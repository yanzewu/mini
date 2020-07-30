#ifndef ATTRIBUTOR_H
#define ATTRIBUTOR_H

#include "ast.h"
#include "symtable.h"
#include "type.h"
#include "errors.h"
#include "builtin.h"

namespace mini {

    // Semantic analysis for AST
    class Attributor {

    public:

        Attributor() {}

        void process(const std::vector<pAST>& nodes, SymbolTable& sym_table) {
            this->symbol_table = &sym_table;
            for (const auto& node : nodes) {
                if (node->is_expr()) {
                    process_expr(std::static_pointer_cast<ExprNode>(node));
                }
                else if (node->get_type() == AST::LET) {
                    process_let(ast_cast<LetNode>(node));
                }
                else if (node->get_type() == AST::SET) {
                    process_set(ast_cast<SetNode>(node));
                }
                else if (node->get_type() == AST::CLASS) {
                    process_class(ast_cast<ClassNode>(node));
                }
                else {
                    throw std::runtime_error("Incorrect AST type");
                }
            }
            this->symbol_table = nullptr;
        }

        void process_type(const Ptr<TypeNode>& m_node) {
            TypedefRef ref = symbol_table->find_type(m_node->symbol->name, m_node->get_info());
            if (!ref) {
                m_node->get_info().throw_exception("Undefined type: " + m_node->symbol->name);
            }
            else if (m_node->args.size() < ref->min_arg || m_node->args.size() > ref->max_arg) {
                m_node->get_info().throw_exception("Incorrect type parameter number");
            }
            else {
                std::vector<pType> type_args;
                for (const auto& a : m_node->args) {
                    process_type(a);
                    type_args.push_back(a->prog_type);
                }
                m_node->prog_type = std::make_shared<Type>(ref, type_args);
            }
        }

        void process_expr(const Ptr<ExprNode>& m_node) {
            switch (m_node->get_type())
            {
            case AST::CONSTANT: process_constant(ast_cast<ConstantNode>(m_node)); break;
            case AST::VAR: process_var(ast_cast<VarNode>(m_node)); break;
            case AST::TUPLE: process_tuple(ast_cast<TupleNode>(m_node)); break;
            case AST::STRUCT: process_struct(ast_cast<StructNode>(m_node)); break;
            case AST::ARRAY: process_array(ast_cast<ArrayNode>(m_node)); break;
            case AST::FUNCALL: process_funcall(ast_cast<FunCallNode>(m_node)); break;
            case AST::GETFIELD: process_getfield(ast_cast<GetFieldNode>(m_node)); break;
            case AST::LAMBDA: process_lambda(ast_cast<LambdaNode>(m_node)); break;
            default:
                break;
            }
        }

        void process_constant(ConstantNode* m_node) {
            switch (m_node->value.get_type())
            {
            case Constant::Type_t::NIL: m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("nil")); break;
            case Constant::Type_t::BOOL: m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("bool")); break;
            case Constant::Type_t::CHAR: m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("char")); break;
            case Constant::Type_t::INT: m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("int")); break;
            case Constant::Type_t::FLOAT: m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("float")); break;
            case Constant::Type_t::STRING: m_node->prog_type = BuiltinTypeBuilder("array")("char")(*symbol_table); break;
            default:
                break;
            }
        }

        void process_var(VarNode* m_node) {
            VariableRef ref = symbol_table->find_var(m_node->symbol.get());
            if (!ref) {
                m_node->get_info().throw_exception("Undefined variable: " + m_node->symbol->name);
            }
            else {
                m_node->prog_type = ref->prog_type;
                
                if (!ref->has_assigned) {
                //    node->cur_info().throw_exception("Variable " + node->symbol->symbol + "has been used before assignment");
                    // not sure if an exception is necessary
                }

                // In a closure, we record the appearance of a local variable outside current scope since
                // it may have side-effects. Calling f[bindings](args) is equivalent of calling
                // f_naked(bindings)(args) where f_naked(bindings) returns f with all binding variables filled.
                // For a static language, what we store is f_naked. Another advantage is that we can allocate these
                // bound variables separately (usually useful for language without GC)
                if (ref->scope != 0 && ref->scope != symbol_table->cur_scope() && binding_cache.size() > 0) {
                    auto r = binding_cache.back().find(ref);
                    if (r == binding_cache.back().end()) {
                        // create new ref assign to r
                        auto new_binding_var = symbol_table->insert_var(
                            ref->symbol, VarMetaData::Source::BINDING, ref->prog_type);
                        binding_cache.back().insert({ ref, new_binding_var });
                        m_node->set_ref(new_binding_var);
                    }
                    else {  // already referenced as binding variable
                        m_node->set_ref(r->second);
                    }
                }
                else {  // local/global/arg
                    m_node->set_ref(ref);
                }
            }
        }

        void process_tuple(TupleNode* m_node) {

            if (m_node->children.size() == 0) {
                m_node->get_info().throw_exception("Tuple cannot be empty");
            }

            std::vector<Ptr<Type>> type_args;
            for (auto& c : m_node->children) {
                process_expr(c);
                type_args.push_back(c->prog_type);
            }
            m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("tuple"), type_args);
        }

        void process_struct(StructNode* m_node) {

            if (m_node->children.size() == 0) {
                m_node->set_prog_type(std::make_shared<Type>(symbol_table->find_type("object")));
                return;
            }

            std::unordered_map<std::string, pType> fields;

            for (auto& c : m_node->children) {
                process_expr(c.second);
                auto r = fields.insert({ c.first->name, c.second->get_prog_type() });
                if (!r.second) {
                    c.first->get_info().throw_exception("Field redefinition: " + c.first->get_name());
                }

            }
            if (m_node->children.size() == 0) {     // empty struct is equivalent to object
                m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("object"));
            }
            else {
                m_node->prog_type = std::make_shared<StructType>(symbol_table->find_type("struct"), fields);
            }
        }

        void process_array(ArrayNode* m_node) {

            // TODO: array is a recursive type so it has a complex type rule. this should be addressed in future

            pType maximum_type = std::make_shared<Type>(symbol_table->find_type("bottom"));

            if (m_node->children.empty()) {
            }
            else {
                process_expr(m_node->children[0]);
                maximum_type = m_node->children[0]->prog_type;

                for (size_t i = 1; i < m_node->children.size(); i++) {
                    process_expr(m_node->children[i]);
                    auto result = Type::compare(*maximum_type, *(m_node->children[i]->prog_type));
                    switch (result)
                    {
                    case Type::Ordering::UNCOMPARABLE: {
                        m_node->get_info().throw_exception("Type not compatible");  // TODO this should be the super types of both, ends up with object
                        break;
                    }
                    case Type::Ordering::GREATER:
                    case Type::Ordering::EQUAL:
                        break;
                    case Type::Ordering::LESS: maximum_type = m_node->children[i]->prog_type;
                        break;
                    default:
                        break;
                    }
                }
            }
            m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("array"), std::vector<pType>({ maximum_type }));
        }

        void process_funcall(FunCallNode* m_node) {

            process_expr(m_node->caller);
            const auto& func_type = m_node->caller->prog_type;
            if (func_type->args.size() - 1 != m_node->args.size()) {
                StringOutputStream soutput;
                soutput << "Expect " << func_type->args.size() - 1 << " args, got " << m_node->args.size();
                m_node->get_info().throw_exception(soutput.buffer);
            }
            for (size_t i = 0; i < m_node->args.size(); i++) {
                process_expr(m_node->args[i]);
                match_type(func_type->args[i], m_node->args[i]->prog_type, m_node->args[i]->get_info());
            }
            m_node->prog_type = func_type->args.back();

        }

        void process_getfield(GetFieldNode* m_node) {
            process_expr(m_node->lhs);
            auto lhs_type = m_node->lhs->get_prog_type().get();
            if (lhs_type->is_struct() || lhs_type->is_custom_class()) {     // has field
                auto& fields = static_cast<StructType*>(lhs_type)->fields;
                auto f = fields.find(m_node->field->get_name());
                if (f == fields.end()) {
                    StringOutputStream soutput;
                    soutput << "Class '" << lhs_type->ref->symbol->get_name() << "' does not have field '" << m_node->field->get_name() << "'";
                    m_node->field->get_info().throw_exception(soutput.buffer);
                }
                m_node->set_prog_type(f->second);
            }
        }

        void process_lambda(LambdaNode* m_node) {


            std::vector<pType> type_args;

            // build function types and local variables
            symbol_table->push_scope(symbol_table->create_scope());
            for (const auto& v : m_node->args) {
                process_type(v.second);
                type_args.push_back(v.second->prog_type);
                symbol_table->insert_var(v.first, VarMetaData::Source::ARG, v.second->prog_type);
            }
            process_type(m_node->ret_type);
            type_args.push_back(m_node->ret_type->prog_type);
            binding_cache.push_back({});

            for (auto& s : m_node->statements) {
                if (s->is_expr()) {
                    Ptr<ExprNode> st = std::dynamic_pointer_cast<ExprNode>(s);
                    process_expr(st);
                }
                else if (s->get_type() == AST::LET) {
                    process_let(ast_cast<LetNode>(s));
                }
                else if (s->get_type() == AST::SET) {
                    process_set(ast_cast<SetNode>(s));
                }
                else {
                    throw std::runtime_error("Incorrect AST type");
                }
            }
            // moving external bindings to the lambda
            m_node->bindings.resize(binding_cache.back().size());
            for (const auto& bce : binding_cache.back()) {
                m_node->bindings[bce.second->index] = bce.first;
            }

            binding_cache.pop_back();
            symbol_table->pop_scope();

            // match the type of last statement
            // the full monad feature will be improved in the future -- where the last statement should be 'return x'
            const auto& s_last = m_node->statements.back();
            if (s_last->is_expr()) {
                match_type(type_args.back(), s_last->as<ExprNode>()->prog_type, s_last->get_info());
            }
            else if (m_node->ret_type->prog_type->is_nil()) {       // nil is allowed to be implicit
            }
            else {
                s_last->get_info().throw_exception("Statements inside function return non-nil must end with expression");
            }

            m_node->prog_type = std::make_shared<Type>(symbol_table->find_type("function"), type_args);

        }

        void process_let(LetNode* m_node) {

            process_type(m_node->vtype);

            if (m_node->expr) {
                process_expr(m_node->expr);
                match_type_in_decl(m_node->vtype->prog_type, m_node->expr->prog_type, m_node->get_info());
            }

            if (symbol_table->cur_scope() != 0) {
                m_node->ref = symbol_table->insert_var(m_node->symbol, VarMetaData::LOCAL, m_node->vtype->prog_type);
            }
            else {
                m_node->ref = symbol_table->insert_var(m_node->symbol, VarMetaData::GLOBAL, m_node->vtype->prog_type);
            }

            // usually we need an initialization; unless it's a system lib function
            m_node->ref->has_assigned = (m_node->expr != NULL);
        }

        void process_set(SetNode* m_node) {

            if (m_node->lhs->get_type() == AST::VAR) {
                auto lhs = ast_cast<VarNode>(m_node->lhs);
                VariableRef ref = symbol_table->find_var(lhs->symbol.get());
                if (!ref) {
                    m_node->get_info().throw_exception("Undefined variable: " + lhs->symbol->get_name());
                }
                else {
                    if (ref->scope == 0 && BuiltinSymbolGenerator::is_builtin_symbol(lhs->symbol.get())) {
                        m_node->get_info().throw_exception("Redefinition of system symbols are not allowed");
                        // 'let' does not need this because there is redefinition check.
                    }

                    ref->has_assigned = true;
                    lhs->ref = ref;
                    process_expr(m_node->expr);

                    match_type(lhs->ref->prog_type, m_node->expr->prog_type, m_node->get_info());

                    // ! Assignment to external (i.e bind) variable is not allowed. (It can be allowed by making a new
                    // variable with same symbol, like in Python, but just too confusing)
                    if (ref->scope != 0 && ref->scope != symbol_table->cur_scope()) {
                        m_node->get_info().throw_exception("Cannot assign external variable inside closure");
                    }
                }
            }
            else if (m_node->lhs->get_type() == AST::GETFIELD) {
                auto lhs = ast_cast<GetFieldNode>(m_node->lhs);
                process_getfield(lhs);
                process_expr(m_node->expr);
                match_type(lhs->prog_type, m_node->expr->prog_type, m_node->get_info());
            }
            else {
                throw std::runtime_error("Incorrect AST Type");
            }

            
        }

        void process_class(ClassNode* m_node) {


            // resolve inheritance and implementation first.

            for (const auto child : m_node->members) {

                // get field info, add into class type information (exclude static)
                // if it's a variable, also do the same type check as global var.
                // if it's a function, do the same type check and lambda function as global function.

            }

            // remove class AST from list.
        }

        // If lhs >= rhs return 1, if lhs < rhs return 0. Raises exception if not comparable.
        unsigned match_type(const pType& lhs, const pType& rhs, const SymbolInfo& info) {

            auto result = Type::partial_compare(*lhs, *rhs);
            switch (result)
            {
            case Type::Ordering::UNCOMPARABLE: {
                StringOutputStream soutput;
                soutput << "Uncomparable type: " << *lhs << " and " << *rhs;
                info.throw_exception(soutput.buffer);
                return 0;
            }
            case Type::Ordering::GREATER: return 1;
            case Type::Ordering::EQUAL: return 0;
            default:
                return 0;
            }
        }

        // similar as match_type, except allowing contravariant assign with struct => class
        unsigned match_type_in_decl(const pType& lhs, const pType& rhs, const SymbolInfo& info) {
            if (lhs->is_custom_class() && rhs->is_struct()) {
                auto m_lhs = static_cast<const StructType*>(lhs.get());
                auto m_rhs = static_cast<const StructType*>(rhs.get());
                auto a = m_lhs->field_compatible(*m_rhs);
                if (a == Type::Ordering::EQUAL || a == Type::Ordering::GREATER) {
                    return 2;
                }
                else {
                    info.throw_exception("All fields in RHS must match");
                    return 0;
                }
            }
            else {
                return match_type(lhs, rhs, info);
            }
        }

    private:

        SymbolTable* symbol_table;

        std::vector<std::unordered_map<VariableRef, VariableRef>> binding_cache;   // cache of bindings that need to be recorded
        // This is the stack of binding cache; For each element, it is a dict {external_ref:binding_ref}
    };

}

#endif