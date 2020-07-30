#ifndef MINI_BUILTIN_H
#define MINI_BUILTIN_H

#include "value.h"
#include "symtable.h"
#include "type.h"
#include "bytecode.h"

namespace mini {

    // build type quickly from string
    class BuiltinTypeBuilder {
    public:

        BuiltinTypeBuilder() {}
        explicit BuiltinTypeBuilder(const std::string& str) : s(str) {}

        // add an argument
        BuiltinTypeBuilder& operator()(const std::string& arg) {
            args.emplace_back(arg); return *this;
        }
        BuiltinTypeBuilder& operator()(const BuiltinTypeBuilder& arg) {
            args.push_back(arg); return *this;
        }
        // add a field
        BuiltinTypeBuilder& operator()(const std::string& field, const std::string& val) {
            fields.push_back({ field, BuiltinTypeBuilder(val) }); return *this;
        }
        BuiltinTypeBuilder& operator()(const std::string& field, const BuiltinTypeBuilder& val) {
            fields.push_back({ field, val }); return *this;
        }

        pType operator()()const;
        pType operator()(const SymbolTable&)const;

        std::string s;
        std::vector<BuiltinTypeBuilder> args;
        std::vector<std::pair<std::string, BuiltinTypeBuilder>> fields;
    };

    class BuiltinSymbolGenerator {
    public:

        struct BuiltinTypeInfo {
            TypeMetaData::Type_t type;
            std::string name;
            unsigned int min_arg;
            unsigned int max_arg;
        };

        struct BuiltinFunctionInfo {
            std::string name;
            std::vector<BuiltinTypeBuilder> arg_types;
            std::vector<ByteCode> codes;

            pType get_prog_type(const SymbolTable& symbol_table)const {
                auto tb = BuiltinTypeBuilder("function");
                for (const auto& a : arg_types) {
                    tb(a);
                }
                return tb(symbol_table);
            }
        };
        
        static const char builtin_prefix = '@';
        static std::vector<BuiltinTypeInfo> builtin_type_info;
        static std::vector<BuiltinFunctionInfo> builtin_function_info;

        // Generate the builtin types into symbol table. Must be called before doing actual works
        static void generate_builtin_types(SymbolTable& symbol_table) {
            unsigned cnt = 0;
            for (const auto& sti : builtin_type_info) {
                auto r = std::make_shared<TypeMetaData>(
                    std::make_shared<Symbol>(sti.name), sti.type);
                r->index = cnt++;
                r->min_arg = sti.min_arg;
                r->max_arg = sti.max_arg;
                symbol_table.insert_type(r);
            }
        }
        // Generate the builtin functions to the symbol table.
        static void generate_builtin_functions(SymbolTable& symbol_table) {
            for (const auto& f : builtin_function_info) {
                symbol_table.insert_var(std::make_shared<Symbol>(f.name), VarMetaData::GLOBAL, f.get_prog_type(symbol_table));
            }
        }

        // Just syntactically (i.e. begin with @)!
        static bool is_builtin_symbol(ConstSymbolRef symbol) {
            return !symbol->get_name().empty() && symbol->get_name()[0] == builtin_prefix;
        }
    };

}

#endif