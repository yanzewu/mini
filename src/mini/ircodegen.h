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
         
        void process_struct(const StructNode* node);

        void process_funcall(const FunCallNode* node);

        void process_getfield(const GetFieldNode* node, const std::shared_ptr<ExprNode>& assignment_expr);

        void process_new(const NewNode* node);

        void process_let(const LetNode* node, bool create_field);

        void process_set(const SetNode* node);

        void process_lambda(const LambdaNode* node, const std::string& name);

        void process_class(const ClassNode* node);

        // fill the system library codes to predefined closures.
        void build_system_lib(const SymbolTable& symbol_table);

        void build_system_type(const SymbolTable& symbol_table);

    private:

        // get the layout address of a struct type
        Size_t struct2layoutaddr(const std::shared_ptr<StructType>& st) {
            const auto& [addr, notfound] = struct_addr.insert({ 
                StructType::Identifier(std::static_pointer_cast<StructType>(st->erasure(ref_addressable))), Size_t(0) });
            if (!notfound) return addr->second;

            // Register a new struct type.

            ClassLayout* cl = new ClassLayout();
            cl->offset.push_back(4);        // initial offset for classinfo.

            Size_t cindex = irprog->add_constant(cl);
            class_stack.push_back(cindex);
            addr->second = cindex;

            ClassInfo* ci = new ClassInfo();
            cl->info_index = irprog->add_constant(ci);

            ci->name_index = add_string("struct#" + std::to_string(++struct_count));
            ci->symbol_info = SymbolInfo::absolute();

            for (const auto& m : addr->first.val->fields) {
                add_field(m.first, m.second);
            }
            pop_class_env();
            return cindex;
        }

        Size_t lookup_field_index(Size_t info_index, Size_t field_id) const {
            return field_indices.at( (size_t(info_index) << 32) + field_id );
        }
        void insert_field_offset(Size_t info_index, Size_t field_id, Size_t field_index) {
            field_indices.insert({ (size_t(info_index) << 32) + field_id, field_index });
        }

        uint16_t get_typebit(const pType& tp)const {
            switch (tp->erased_primitive_type())
            {
            case PrimitiveTypeMetaData::NIL:
            case PrimitiveTypeMetaData::BOOL:
            case PrimitiveTypeMetaData::CHAR: return 0;
            case PrimitiveTypeMetaData::INT: return 1;
            case PrimitiveTypeMetaData::FLOAT: return 2;
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
        Size_t type2infoaddr(const pType& tr) {

            /* Type erasure, but only one layer */

            // for a system type, we just store the info addr (since there is no class)
            switch (tr->_type)
            {
            case Type::Type_t::PRIMITIVE: 
                return type_addr.at(BuiltinSymbolGenerator::builtin_type_index(tr->as<PrimitiveType>()->type_name()));
            case Type::Type_t::STRUCT:
                return irprog->fetch_constant(
                    struct2layoutaddr(std::static_pointer_cast<StructType>(tr))
                )->as<ClassLayout>()->info_index;
            case Type::Type_t::OBJECT:
                return irprog->fetch_constant(
                    type_addr.at(tr->as<ObjectType>()->ref->index)
                )->as<ClassLayout>()->info_index;
            case Type::Type_t::VARIABLE: 
                return type_addr.at(BuiltinSymbolGenerator::builtin_type_index(tr->erased_primitive_type()));
            case Type::Type_t::UNIVERSAL: return type2infoaddr(tr->as<UniversalType>()->body);
            default:
                throw std::runtime_error("Unsupported type");
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
            auto r = field_ids.find(name);  // ensure the uniqueness of field string
            if (r == field_ids.end()) {
                r = field_ids.insert({ name, add_string(name) }).first;
            }
            cur_class()->offset.push_back(cur_class()->offset.back() + 4);

            auto field_index = cur_class()->offset.size() - 2;
            auto& field_info = cur_class_info()->field_info;
            field_info.push_back({ r->second, type2infoaddr(type) });
            insert_field_offset(cur_class()->info_index, r->second, Size_t(field_index));
            return field_index;
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
        std::unordered_map<Size_t, Size_t> type_addr;           // address of typeinfo/classlayout in constant pool for primitive/object types, keyed by id
        std::vector<Offset_t> latest_linenos;   // latest line numbers in each file, keyed by filename id
        std::unordered_map<std::string, Size_t> field_ids;      // global field id for interfaces
        std::unordered_map<uint64_t, Size_t> field_indices;     // field offsets map, keyed by info_id:field_id
        std::unordered_map<StructType::Identifier, Size_t, StructType::Hasher> struct_addr;      // address of classlayout for struct types
        size_t struct_count = 0;
        ConstTypedefRef ref_addressable;
    };

}

#endif