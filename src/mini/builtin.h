#ifndef MINI_BUILTIN_H
#define MINI_BUILTIN_H

#include "value.h"
#include "symtable.h"
#include "type.h"
#include "typeaux.h"
#include "bytecode.h"

#include <algorithm>

namespace mini {

    class BuiltinSymbolGenerator {
    public:

        // Represents a built-in type
        struct BuiltinTypeInfo {
            PrimitiveTypeMetaData::Primitive_Type_t type;
            std::string name;
        };

        struct BuiltinFunctionInfo {
            std::string name;
            const TypeBuilder* builder;
            std::vector<ByteCode> codes;

            pType get_prog_type(SymbolTable& symbol_table)const {
                return builder->build(symbol_table);
            }
        };
        
        static const char builtin_prefix = '@';
        static std::vector<BuiltinTypeInfo> builtin_type_info;
        static std::vector<BuiltinFunctionInfo> builtin_function_info;

        // Generate the builtin types into symbol table. Must be called before doing actual works
        static void generate_builtin_types(SymbolTable& symbol_table) {
            unsigned cnt = 0;
            for (const auto& sti : builtin_type_info) {
                auto r = std::make_shared<PrimitiveTypeMetaData>(
                    std::make_shared<Symbol>(sti.name), sti.type);
                symbol_table.insert_primitive_type(r);
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

        // Get the index of builtin types
        static Index_t builtin_type_index(PrimitiveTypeMetaData::Primitive_Type_t primitive_type) {
            return std::find_if(builtin_type_info.begin(), builtin_type_info.end(), 
                [primitive_type](const BuiltinTypeInfo& bti) { return bti.type == primitive_type; }) - builtin_type_info.begin();
        }
    };

}

#endif