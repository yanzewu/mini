#ifndef MINI_IRCODEGEN_H
#define MINI_IRCODEGEN_H

#include "ast.h"
#include "ir.h"
#include "builtin.h"
#include "constant.h"

#include <vector>
#include <algorithm>

namespace mini {

    class IRCodeGenerator {
    public:

        IRCodeGenerator() {}

        void process(const std::vector<pAST>& nodes, const SymbolTable& sym_table, IRProgram& irprog, const std::vector<std::string>& filename_table);

        void process_expr(const Ptr<ExprNode>& node, const std::string& name = "");

        void process_constant(const ConstantNode* node);

        void process_var(const VarNode* node);

        void process_array(const ArrayNode* node);

        void process_tuple(const TupleNode* node);

        void process_funcall(const FunCallNode* node);

        void process_getfield(const GetFieldNode* node);

        void process_let(const LetNode* node, bool create_field);

        void process_set(const SetNode* node);

        void process_lambda(const LambdaNode* node, const std::string& name);

        void process_class(const ClassNode* node);

        // fill the system library codes to predefined closures.
        void build_system_lib(const SymbolTable& symbol_table);

        void build_system_type(const SymbolTable& symbol_table);

    private:

        uint16_t get_typebit(const pType& tp)const {
            switch (tp->ref->get_type())
            {
            case TypeMetaData::NIL:
            case TypeMetaData::BOOL:
            case TypeMetaData::CHAR: return 0;
            case TypeMetaData::INT: return 1;
            case TypeMetaData::FLOAT: return 2;
            default:
                return 3;
            }
        }
        Size_t localindex2addr(unsigned index)const {
            return cur_function()->sz_arg + cur_function()->sz_bind + index;
        }
        Size_t bindindex2addr(unsigned index)const {
            return cur_function()->sz_arg + index;
        }
        Size_t argindex2addr(unsigned index)const {
            return index;
        }
        // get the typedef
        Size_t type2infoaddr(ConstTypedefRef tr)const {

            // for a system type, we just store the info addr (since there is no class)
            if (tr->get_type() != TypeMetaData::CUSTOM && tr->get_type() != TypeMetaData::STRUCT) {
                return type_addr[tr->index];
            }
            else {
                return irprog->fetch_constant(type_addr[tr->index])->as<ClassLayout>()->info_index;
            }
        }
        Location translate_location(const Location& loc)const {
            return Location(loc.lineno, loc.colno, filename_indices[loc.srcno]);
        }

        // output a bytecode at info
        void emit(const ByteCode& b, const SymbolInfo& info) {
            cur_function()->codes.push_back(b);

            // add a new entry in lnt: new function; no info exist (treat as -1); lineno increased
            if (info.location.lineno > latest_linenos[info.location.srcno] || cur_function()->codes.size() == 1) {
                irprog->fetch_constant(irprog->line_number_table_index)->as<LineNumberTable>()->add_entry(
                    function_stack.back(), cur_function()->codes.size() - 1, info.location.lineno
                );
                latest_linenos[info.location.srcno] = info.location.lineno;
            }
        }

        const Function* cur_function()const {
            return irprog->constant_pool[function_stack.back()]->as<Function>();
        }
        Function* cur_function() {
            return irprog->constant_pool[function_stack.back()]->as<Function>();
        }
        ClassLayout* cur_class() {
            return irprog->constant_pool[class_stack.back()]->as<ClassLayout>();
        }
        ClassInfo* cur_class_info() {
            return irprog->constant_pool[cur_class()->info_index]->as<ClassInfo>();
        }
        Size_t add_string(const std::string& s) {
            irprog->constant_pool.push_back(new StringConstant(s));
            return irprog->constant_pool.size() - 1;
        }
        // add a field with certain type; return the field index
        Size_t add_field(const StringRef& name, const pType& type) {
            cur_class()->offset.push_back(cur_class()->offset.back() + 4);
            
            cur_class_info()->field_info.push_back({add_string(name), type2infoaddr(type->ref)});
            return cur_class()->offset.size() - 2;
        }
        
        Size_t push_lambda_env(Size_t narg, Size_t nbind, const StringRef& name, const SymbolInfo& info, const pType& type);
        
        Size_t push_class_env(const StringRef& name, const SymbolInfo& info, Index_t type_index);
        
        void pop_lambda_env() {
            function_stack.pop_back();
        }
        void pop_class_env() {
            class_stack.pop_back();
        }

        IRProgram* irprog = NULL;
        std::vector<Size_t> function_stack;     // stack of currently processed function id
        std::vector<Size_t> class_stack;        // stack of currently processed class id
        std::vector<Size_t> filename_indices;   // address of filename in constant pool, keyed by id
        std::vector<Size_t> type_addr;          // address of typeinfo in constant pool, keyed by id
        std::vector<Offset_t> latest_linenos;   // latest line numbers in each file, keyed by filename id
    };

}

#endif