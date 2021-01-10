#ifndef MINI_AST_H
#define MINI_AST_H

#include "token.h"
#include "value.h"
#include "type.h"
#include "stream.h"

#include <vector>
#include <memory>

namespace mini {

    class AST {
    public:

        enum Type_t {
            NONE,
            CONSTANT,
            VAR,
            TUPLE,
            STRUCT,
            ARRAY,
            LAMBDA,
            FUNCALL,    // term->term
            GETFIELD,
            NEW,
            CASE,
            TYPE,       // type->type
            TYPEAPPL,   // type->term
            LET,
            SET,
            CLASS,      // class definition
            INTERFACE,  // interface definition
            IMPORT
        };

        AST() : _type(Type_t::NONE) {}
        
        // Shortcut for expression checking
        virtual bool is_expr()const {
            return false;
        }

        void print(OutputStream& os)const {
            print(os, 0);
        }

        // Get the node type (not programming type)
        AST::Type_t get_type()const {
            return _type;
        }

        // Set info of the root symbol
        void set_info(const SymbolInfo& info_) {
            info = info_;
        }

        // Get info of the root symbol
        const SymbolInfo& get_info()const {
            return info;
        }

        template<typename T>
        const T* as()const {
            static_assert(std::is_base_of<AST, T>::value, "Proper type required in casting");
            return static_cast<const T*>(this);
        }

        virtual void print(OutputStream&, unsigned indent)const {};
    protected:
        AST::Type_t _type;
        SymbolInfo info;

        explicit AST(AST::Type_t tp) : _type(tp) {}
        AST(AST::Type_t tp, const SymbolInfo& info) : _type(tp), info(info) {}
        
    };

    inline OutputStream& operator<<(OutputStream& os, const AST& a) {
        a.print(os); return os;
    }

    typedef Ptr<AST> pAST;
    
    // get pointer
    template<typename T>
    T* ast_cast(const pAST& node) {
        static_assert(std::is_base_of<AST, T>::value, "AST-derived type required");
        return static_cast<T*>(node.get());
    }

    // get the constant pointer
    template<typename T>
    const T* const_ast_cast(const pAST& node) {
        static_assert(std::is_base_of<AST, T>::value, "AST-derived type required");
        return static_cast<const T*>(node.get());
    }

    // a calculatable expression (might be a single expr, variable, or function call)
    class ExprNode : public AST {

    public:
        pType prog_type;    // the programming type. All expressions must have a type.
        // ExprNode has the ownship of type because these are type instances, not the type defined
        // (consider the Rank 2 types like array)

        ExprNode() : AST() {}
        
        bool is_expr()const {
            return true;
        }

        // get the programming type
        pType& get_prog_type() {
            return prog_type;
        }
        // get programming type as ptr
        ConstTypeRef get_prog_type()const {
            return prog_type.get();
        }
        // set programming type
        void set_prog_type(const pType& tp) {
            this->prog_type = tp;
        }

        virtual void print(OutputStream&, unsigned indent)const {};
    protected:
        explicit ExprNode(AST::Type_t tp) : AST(tp), prog_type(NULL) {}
        ExprNode(AST::Type_t tp, const SymbolInfo& info) : AST(tp, info), prog_type(NULL) {}

        // Try print type; Does nothing if type is not defined.
        void print_type(OutputStream& os)const;
    };

    // constant (other than lambda!)
    class ConstantNode : public ExprNode {
    public:
        Constant value;

        ConstantNode() : ExprNode(AST::Type_t::CONSTANT) {}
        explicit ConstantNode(const Constant& val) : ExprNode(AST::Type_t::CONSTANT), value(val) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    // variable
    class VarNode : public ExprNode {
    public:

        pSymbol symbol;
        ConstVariableRef ref;
        bool skip_attribution = false;
        // AST has the ownship of pSymbol because that's the first place it is stored.

        VarNode() : ExprNode(AST::Type_t::VAR), ref(NULL) {}
        explicit VarNode(const pSymbol& name_) : 
            ExprNode(AST::Type_t::VAR, name_->get_info()), symbol(name_), ref(NULL) {}

        void set_symbol(const pSymbol& symbol) {
            this->symbol = symbol;
        }

        // Set the reference of symbol
        void set_ref(ConstVariableRef ref) {
            this->ref = ref;
        }
        void print(OutputStream& os, unsigned indent)const;

        // Make an attributed variable node.
        static Ptr<VarNode> make_attributed(ConstVariableRef ref, const SymbolInfo& info) {
            auto r = std::make_shared<VarNode>();
            r->set_symbol(std::make_shared<Symbol>(ref->symbol->get_name(), info));
            r->set_ref(ref);
            r->set_info(info);
            r->skip_attribution = true;
            return r;
        }
    };

    class TupleNode : public ExprNode {
    public:

        std::vector<Ptr<ExprNode>> children;

        TupleNode() : ExprNode(AST::Type_t::TUPLE) {}

        void print(OutputStream& os, unsigned indent)const;
        
    };

    class ArrayNode : public ExprNode {
    public:

        std::vector<Ptr<ExprNode>> children;

        ArrayNode() : ExprNode(AST::Type_t::ARRAY) {}

        void print(OutputStream& os, unsigned indent)const;

    };

    class StructNode : public ExprNode {
    public:

        std::vector<std::pair<pSymbol, Ptr<ExprNode>>> children;

        std::vector<pFieldMetaData> field_meta;

        StructNode() : ExprNode(AST::Type_t::STRUCT) {

        }
        void print(OutputStream& os, unsigned indent)const;
    };

    // function call
    class FunCallNode : public ExprNode {
    public:

        Ptr<ExprNode> caller;
        std::vector<Ptr<ExprNode>> args;
        bool is_constructor = false;

        FunCallNode() : ExprNode(AST::Type_t::FUNCALL) {}
        FunCallNode(const SymbolInfo& info, const Ptr<ExprNode>& caller, const std::vector<Ptr<ExprNode>>& args) : 
            ExprNode(AST::Type_t::FUNCALL, info), caller(caller), args(args) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    // get member
    class GetFieldNode : public ExprNode {
    public:

        Ptr<ExprNode> lhs;
        pSymbol field;

        GetFieldNode() : ExprNode(AST::Type_t::GETFIELD), field(NULL) {}
        GetFieldNode(const SymbolInfo& info, const Ptr<ExprNode>& lhs, const pSymbol& field) :
            ExprNode(AST::Type_t::GETFIELD, info), lhs(lhs), field(field) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    // type and aggregation type
    class TypeNode : public AST {
    public:

        /* Represents two kinds of TypeNodes:
            Type constructor node: psymbol(args)
            Universal type node: forall<quantifiers>.child
        */

        pSymbol symbol;
        std::vector<Ptr<TypeNode>> args;
        std::vector<std::pair<pSymbol, Ptr<TypeNode>>> quantifiers;

        pType prog_type;
        bool skip_attribution = false;

        TypeNode() : AST(AST::Type_t::TYPE), prog_type(NULL) {}

        // non-universal type only
        explicit TypeNode(const pSymbol& symbol) : AST(AST::TYPE), symbol(symbol) {
            this->set_info(symbol->get_info());
        }

        bool is_expr()const {
            return false;
        }
        void print(OutputStream& os, unsigned indent)const;

        // Make an attributed type node
        static Ptr<TypeNode> make_attributed(pType prog_type, const SymbolInfo& info) {
            auto r = std::make_shared<TypeNode>();
            r->prog_type = prog_type;
            r->info = info;
            r->skip_attribution = true;
            return r;
        }
    };

    // new X
    class NewNode : public ExprNode {
    public:
        // new parses like a variable but translates like a function call.
        pSymbol symbol;
        Ptr<VarNode> self_arg;                  // for extends. Represents the self parameter in extends A => new_A(self)
        std::vector<Ptr<TypeNode>> type_args;   // type arguments.

        VariableRef constructor_ref = NULL;            // ref to constructor function A->(...->A)
        const ObjectTypeMetaData* type_ref = NULL;     // ref to the object type

        NewNode() : ExprNode(AST::Type_t::NEW), self_arg(NULL) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    class TypeApplNode : public ExprNode {
    public:
        Ptr<ExprNode> lhs;
        std::vector<Ptr<TypeNode>> args;

        TypeApplNode() : ExprNode(AST::Type_t::TYPEAPPL), lhs(NULL) {}
        TypeApplNode(const SymbolInfo& info, const Ptr<ExprNode>& lhs, const std::vector<Ptr<TypeNode>>& args) :
            ExprNode(AST::Type_t::TYPEAPPL, info), lhs(lhs), args(args) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    // lambda instance
    class LambdaNode : public ExprNode {
    public:

        Ptr<TypeNode> ret_type; // return type (null if omitted)
        std::vector<std::pair<pSymbol, Ptr<TypeNode>>> quantifiers;
        std::vector<std::pair<pSymbol, Ptr<TypeNode>>> args;
        std::vector<Ptr<AST>> statements;

        std::vector<VariableRef> bindings;

        LambdaNode() : ExprNode(AST::Type_t::LAMBDA) {}

        // Make a lambda node without arguments & quantifiers
        LambdaNode(const SymbolInfo& info, const std::vector<pAST>& statements) : 
            ExprNode(AST::Type_t::LAMBDA, info),  statements(statements) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    class CommandNode : public AST {
    public:

        CommandNode() {}

        bool is_expr()const {
            return false;
        }
        void print(OutputStream& os, unsigned indent)const {}

    protected:
        explicit CommandNode(AST::Type_t tp) : AST(tp) {}
        CommandNode(AST::Type_t tp, const SymbolInfo& info) : AST(tp, info) {}
    };

    class LetNode : public CommandNode {

    public:

        pSymbol symbol;
        Ptr<TypeNode> vtype;
        Ptr<ExprNode> expr;  // might be nothing

        VariableRef ref;

        bool has_init = false;

        LetNode() : CommandNode(AST::Type_t::LET), expr(NULL), ref(NULL) {}
        LetNode(const SymbolInfo& info, const pSymbol& symbol, const Ptr<TypeNode>& vtype, const Ptr<ExprNode>& expr) :
            CommandNode(AST::Type_t::LET, info), vtype(vtype), expr(expr), ref(NULL) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    class SetNode : public CommandNode {
    public:

        Ptr<ExprNode> lhs;
        Ptr<ExprNode> expr;

        SetNode() : CommandNode(AST::Type_t::SET), lhs(NULL), expr(NULL) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    // pattern matching & cases
    class CaseNode : public ExprNode {
    public:
        struct Case {
            Ptr<ExprNode> condition;    // VarNode/ConstantNode/TupleNode
            Ptr<ExprNode> guard = NULL; //
            Ptr<ExprNode> expr;         // Cannot be Null
        };

        Ptr<ExprNode> lhs;
        std::vector<Case> cases;

        std::vector<Ptr<AST>> statements;

        CaseNode() : ExprNode(AST::Type_t::CASE) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    class ClassNode : public CommandNode {
    public:

        // metadata here (extends, implements)
        // template metadata here

        struct ClassMemberMeta {
            bool is_static;
            bool is_virtual;
        };

        pSymbol symbol;
        Ptr<TypeNode> base;
        std::vector<Ptr<TypeNode>> interfaces;
        std::vector<std::pair<Ptr<LetNode>, ClassMemberMeta>> members;
        Ptr<LambdaNode> constructor;

        TypedefRef ref = NULL;
        VariableRef constructor_ref = NULL;

        ClassNode() : CommandNode(AST::CLASS), base(NULL), constructor(NULL) {}

        void print(OutputStream& os, unsigned indent)const;
    };

    class InterfaceNode : public CommandNode {
    public:
        pSymbol symbol;
        std::vector<Ptr<LetNode>> members;
        std::vector<pSymbol> parents;

        InterfaceNode() : CommandNode(AST::Type_t::INTERFACE) {}

        void add_member(const Ptr<LetNode>& m) { members.push_back(m); }

        void print(OutputStream& os, unsigned indent)const;

    };

    class ImportNode : public CommandNode {
    public:

        std::vector<pSymbol> symbols;

        ImportNode() : CommandNode(AST::Type_t::IMPORT) {}

        void add_symbol(const pSymbol& symbol) {
            symbols.push_back(symbol);
        }

        // assemble the filename
        std::string get_filename()const {
            if (symbols.size() == 0) return "";

            std::string buffer = symbols[0]->get_name();
            for (size_t i = 1; i < symbols.size(); i++) {
                buffer += "/" + symbols[i]->get_name();
            }
            return buffer;
        }

        void print(OutputStream& os, unsigned indent)const;
    };
}

#endif