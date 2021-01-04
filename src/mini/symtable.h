#ifndef MINI_SYMTABLE_H
#define MINI_SYMTABLE_H

#include "value.h"

#include <unordered_map>
#include <memory>

namespace mini {


    class DependencyChecker;

    // Variable table of a closure
    class LocalVarTable {
    public:

        std::vector<pVariable> args;
        std::vector<pVariable> bindings;
        std::vector<pVariable> locals;
        std::unordered_map<std::string, VariableRef> names;    // We don't allow same symbol for args and locals

        // Throws if failed.
        VariableRef insert(const std::string& name, pVariable var);

        // Find a variable by symbol. Returns NULL if not found
        VariableRef find(const std::string& name);

        // Find a variable by index.
        VariableRef find(Index_t index, VarMetaData::Source source);

        Index_t next_index(VarMetaData::Source source)const;

        size_t size()const {
            return names.size();
        }

        OutputStream& print(OutputStream& os) const {
            for (const auto& v : args) { os << *v << "\n"; }
            for (const auto& v : bindings) { os << *v << "\n"; }
            for (const auto& v : locals) { os << *v << "\n"; }
            return os;
        }
    };
    inline OutputStream& operator<<(OutputStream& os, const LocalVarTable& a) {
        return a.print(os);
    }

    class SymbolTable {
    public:

        void initialize(const DependencyChecker* dependency_ref_) {
            var_table_storage.resize(1);
            var_table_stack.push_back(0);
            dependency_ref = dependency_ref_;
        }

        // find a variable in all scopes without checking visibility
        VariableRef find_var(const std::string&);

        // find a variable in all scopes defined before current location
        VariableRef find_var(const std::string&, const SymbolInfo&);

        // find a variable in all scopes defined before current location, specified in ref->cur_info()
        VariableRef find_var(ConstSymbolRef ref) {
            return find_var(ref->get_name(), ref->get_info());
        }

        // find a primitive type
        const PrimitiveTypeMetaData* find_type(const std::string& name)const {
            auto r = type_table.find(name)->second.get();
            return r->is_primitive() ? r->as<PrimitiveTypeMetaData>() : NULL;
        }

        // find a nonprimitive type. Use with care, since it won't check location
        ConstTypedefRef find_global_type(const std::string& name)const {
            return type_table.find(name)->second.get();
        }

        // find a type or type variable. returns (ref, relative stack distance)
        std::pair<TypedefRef, unsigned> find_type(const std::string& name, const SymbolInfo&);

        // insert a variable into the top scope. Throws if already defined.
        VariableRef insert_var(const pSymbol& symbol, VarMetaData::Source source, const pType& prog_type);

        // Caution: Will add reference to typemeta.
        TypedefRef insert_primitive_type(const pTypedef& typemeta) {
            auto r = type_table.insert({ typemeta->symbol->get_name(), typemeta});
            if (!r.second) {
                typemeta->symbol->get_info().throw_exception("Type " + typemeta->symbol->get_name() + " is already defined");
            }
            return r.first->second.get();
        }

        // insert a named type. Throws if already defined.
        TypedefRef insert_object_type(const pSymbol& symbol, TypeMetaData::Type_t category = TypeMetaData::OBJECT) {
            auto p_typesymbol = std::make_shared<ObjectTypeMetaData>(symbol, category);
            p_typesymbol->index = type_table.size();
            auto r = type_table.insert({ symbol->get_name(), p_typesymbol });
            if (!r.second) {
                symbol->get_info().throw_exception("Type " + symbol->get_name() + " is already defined");
            }
            return r.first->second.get();
        }

        // insert a named interface type. Throws if already defined.
        TypedefRef insert_interface_type(const pSymbol& symbol) {
            auto p_typesymbol = std::make_shared<InterfaceTypeMetaData>(symbol);
            auto r = type_table.insert({ symbol->get_name(), p_typesymbol });
            if (!r.second) {
                symbol->get_info().throw_exception("Type " + symbol->get_name() + " is already defined");
            }
            return r.first->second.get();
        }

        // insert a type variable
        TypedefRef insert_type_var(const pSymbol& symbol, unsigned arg_id, const pType& quantifier) {
            auto p_typesymbol = std::make_shared<TypeVariableMetaData>(symbol, arg_id, quantifier);
            auto r = type_var_table_storage.back().insert({ symbol->get_name(), p_typesymbol });
            if (!r.second) {
                symbol->get_info().throw_exception("Type " + symbol->get_name() + " is already defined");
            }
            return r.first->second.get();
        }

        // insert a constructor into global table with name converted; change the corresponding typemetadata
        VariableRef insert_constructor(const pSymbol& symbol, ObjectTypeMetaData* otmd, const pType& prog_type) {
            auto p_varsymbol = std::make_shared<VarMetaData>(
                symbol, cur_scope(), var_table_storage[cur_scope()].next_index(VarMetaData::Source::GLOBAL), VarMetaData::Source::GLOBAL);
            p_varsymbol->prog_type = prog_type;
            return var_table_storage[cur_scope()].insert(constructor_name(symbol->get_name()), p_varsymbol);
        }

        // Create a new local scope. Return the scope id.
        Index_t create_scope() {
            var_table_storage.resize(var_table_storage.size() + 1);
            return var_table_storage.size() - 1;
        }

        // push a scope into the stack top
        void push_scope(Index_t scope) {
            if (scope >= var_table_storage.size()) {
                throw std::runtime_error("Scope out of size");
            }
            var_table_stack.push_back(scope);
        }

        // pop the scope at stack top
        void pop_scope() {
            var_table_stack.pop_back();
        }

        // return the number of current scope
        Index_t cur_scope()const {
            return var_table_stack.back();
        }

        void push_type_scope() {
            type_var_table_storage.resize(type_var_table_storage.size() + 1);
        }

        void pop_type_scope() {
            type_var_table_storage.pop_back();
        }

        Index_t next_var_index(VarMetaData::Source source)const {
            return var_table_storage[var_table_stack.back()].next_index(source);
        }

        // just the size of the current local varible table
        size_t var_table_size()const {
            return var_table_storage[var_table_stack.back()].size();
        }

        size_t type_table_size()const {
            return type_table.size();
        }

        OutputStream& print(OutputStream& os)const {
            for (size_t i = 0; i < var_table_storage.size(); i++) {
                os << "Scope " << i << ":\n" << var_table_storage[i];
            }
            return os;
        }

        static std::string constructor_name(const std::string& type_name) {
            return "new " + type_name;
        }

    private:

        typedef std::unordered_map<std::string, pTypedef> LocalTypeTable;
        typedef std::unordered_map<std::string, std::shared_ptr<TypeVariableMetaData>> LocalTypeVariableTable;

        std::vector<LocalVarTable> var_table_storage;
        LocalTypeTable type_table;
        std::vector<LocalTypeVariableTable> type_var_table_storage;
        // typevar table will be destroyed after used since it will be evaluated immediately

        std::vector<unsigned> var_table_stack;

        const DependencyChecker* dependency_ref;
    };
    inline OutputStream& operator<<(OutputStream& os, const SymbolTable& a) {
        a.print(os); return os;
    }
}

#endif