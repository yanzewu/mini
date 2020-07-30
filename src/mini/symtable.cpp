#include "symtable.h"
#include "dependency.h"

using namespace mini;


VariableRef LocalVarTable::insert(const std::string& name, pVariable var) {
    auto it = names.find(name);
    if (it == names.end()) {
        switch (var->source)
        {
        case VarMetaData::Source::GLOBAL:
        case VarMetaData::Source::LOCAL: locals.push_back(var); break;
        case VarMetaData::Source::ARG: args.push_back(var); break;
        case VarMetaData::Source::BINDING: bindings.push_back(var); break;
        default:break;
        }
        names.insert({ name, var.get() });
        return var.get();
    }
    else {
        var->symbol->get_info().throw_exception("Variable " + var->symbol->name + " is already defined");
        return NULL;
    }
}

VariableRef LocalVarTable::find(const std::string& name) {
    auto ret = names.find(name);
    return ret != names.end() ? ret->second : NULL;
}

VariableRef LocalVarTable::find(Index_t index, VarMetaData::Source source) {
    switch (source)
    {
    case VarMetaData::Source::GLOBAL:
    case VarMetaData::Source::LOCAL: return locals[index].get();
    case VarMetaData::Source::ARG: return args[index].get();
    case VarMetaData::Source::BINDING: return bindings[index].get();
    default: return NULL;
    }
}

Index_t LocalVarTable::next_index(VarMetaData::Source source) const {
    switch (source)
    {
    case VarMetaData::Source::GLOBAL:
    case VarMetaData::Source::LOCAL: return locals.size();
    case VarMetaData::Source::ARG: return args.size();
    case VarMetaData::Source::BINDING: return bindings.size();
    default: return 0;
    }
}


VariableRef SymbolTable::find_var(const std::string & name) {

    for (auto vt_iter = var_table_stack.rbegin(); vt_iter != var_table_stack.rend(); vt_iter++) {
        auto ret = var_table_storage[*vt_iter].find(name);
        if (ret) return ret;
    }

    return NULL;
}

VariableRef SymbolTable::find_var(const std::string& name, const SymbolInfo& info) {

    for (auto vt_iter = var_table_stack.rbegin(); vt_iter != var_table_stack.rend(); vt_iter++) {
        auto ret = var_table_storage[*vt_iter].find(name);
        if (ret && dependency_ref->is_defined(info, ret->symbol->info)) return ret;
    }

    return NULL;
}

TypedefRef SymbolTable::find_type(const std::string & name, const SymbolInfo & info) {

    auto ret = type_table.find(name);

    if (ret == type_table.end() || !dependency_ref->is_defined(info, ret->second->symbol->info)) {
        return NULL;
    }
    else {
        return ret->second.get();
    }

}

VariableRef SymbolTable::insert_var(const pSymbol& symbol, VarMetaData::Source source, const pType& prog_type) {

    if (source == VarMetaData::Source::GLOBAL && cur_scope() != 0) {
        throw std::runtime_error("Try to insert global var into local scope");
    }

    // The ownship of VarMetaData is always the symbol table
    auto p_varsymbol = std::make_shared<VarMetaData>(
        symbol, cur_scope(), var_table_storage[cur_scope()].next_index(source), source);
    p_varsymbol->prog_type = prog_type;
    return var_table_storage[cur_scope()].insert(symbol->get_name(), p_varsymbol);
}

