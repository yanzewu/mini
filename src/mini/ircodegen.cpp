
#include "ircodegen.h"

using namespace mini;

void IRCodeGenerator::process(const std::vector<pAST>& nodes, const SymbolTable& sym_table, IRProgram& irprog, const std::vector<std::string>& filename_table) {

    this->irprog = &irprog;

    // put the filenames
    for (const auto& fn : filename_table) {
        filename_indices.push_back(add_string(fn));
    }
    irprog.source_index = filename_indices[0];

    latest_linenos.resize(filename_table.size(), -1);
    irprog.constant_pool.push_back(new LineNumberTable());
    irprog.line_number_table_index = irprog.constant_pool.size() - 1;

    build_system_type(sym_table);
    ref_addressable = sym_table.find_type("@Addressable");

    SymbolInfo info_main(Location(0, 0, 0));

    // the "<main>" function does not belong to any class; it is a static method.
    irprog.entry_index = push_lambda_env(0, 0, "<main>",
        info_main,
        PrimitiveTypeBuilder("function")("nil")("nil")(sym_table) // the signature of main is automatically generated as function(nil,nil)                
    );

    // build the <main> class
    irprog.global_pool_index = push_class_env("<main>", info_main, type_addr.size() - 1);
    build_system_lib(sym_table);

    for (const auto& node : nodes) {
        if (node->is_expr()) {
            process_expr(std::dynamic_pointer_cast<ExprNode>(node));
        }
        else if (node->get_type() == AST::LET) {
            process_let(node->as<LetNode>(), true);
        }
        else if (node->get_type() == AST::SET) {
            process_set(node->as<SetNode>());
        }
        else if (node->get_type() == AST::INTERFACE) {  // as if a new type is introduced
            struct2layoutaddr(std::static_pointer_cast<StructType>(
                sym_table.find_global_type(
                    node->as<InterfaceNode>()->symbol->get_name())
                ->as<InterfaceTypeMetaData>()->actual_type));
        }
        else if (node->get_type() == AST::CLASS) {
            process_class(node->as<ClassNode>());
        }
        else {
            throw std::runtime_error("Incorrect AST Type");
        }
    }
    pop_class_env();
    emit(ByteCode::na_code(ByteCode::HALT), info_main);    // may support return in main in the future
    pop_lambda_env();

    LineNumberTable* lnt = irprog.fetch_constant(irprog.line_number_table_index)->as<LineNumberTable>();
    std::sort(lnt->line_number_table.begin(), lnt->line_number_table.end());

    this->irprog = nullptr;
}

void IRCodeGenerator::process_expr(const Ptr<ExprNode>& node, const std::string& name) {
    switch (node->get_type())
    {
    case AST::CONSTANT: process_constant(node->as<ConstantNode>()); break;
    case AST::VAR: process_var(node->as<VarNode>()); break;
    case AST::ARRAY: process_array(node->as<ArrayNode>()); break;
    case AST::TUPLE: process_tuple(node->as<TupleNode>()); break;
    case AST::STRUCT: process_struct(node->as<StructNode>()); break;
    case AST::FUNCALL: process_funcall(node->as<FunCallNode>()); break;
    case AST::GETFIELD: process_getfield(node->as<GetFieldNode>(), NULL); break;
    case AST::NEW: process_new(node->as<NewNode>()); break;
    case AST::TYPEAPPL: process_expr(node->as<TypeApplNode>()->lhs); break;
    case AST::LAMBDA: process_lambda(node->as<LambdaNode>(), name); break;
    default:
        throw std::runtime_error("Incorrect AST Type");
    }
}

void IRCodeGenerator::process_constant(const ConstantNode* node) {
    switch (node->value.get_type())
    {
    case Constant::Type_t::NIL: emit({ ByteCode::SHIFT }, node->get_info()); break;
    case Constant::Type_t::CHAR: emit({ ByteCode::CONST, 0, StackElem(std::get<char>(node->value.data)) }, node->get_info()); break;
    case Constant::Type_t::BOOL: emit({ ByteCode::CONST, 0, StackElem(std::get<bool>(node->value.data) ? char(1) : char(0)) }, node->get_info()); break;
    case Constant::Type_t::INT: emit({ ByteCode::CONSTI, 0, StackElem(std::get<int>(node->value.data)) }, node->get_info()); break;
    case Constant::Type_t::FLOAT: emit({ ByteCode::CONSTF, 0, StackElem(std::get<float>(node->value.data)) }, node->get_info()); break;
    case Constant::Type_t::STRING: {
        Size_t sindex = add_string(std::get<std::string>(node->value.data));
        emit(ByteCode::sa_code_a(ByteCode::LOADC, sindex), node->get_info());
        break;
    }
    default:
        throw std::runtime_error("Invalid constant");   // won't reached
        break;
    }
}

void IRCodeGenerator::process_var(const VarNode* node) {
    // loadl/loadg
    switch (node->ref->source)
    {
    case VarMetaData::ARG:
        emit(ByteCode::sa_code_a(ByteCode::LOADL, argindex2addr(node->ref->index), get_typebit(node->prog_type)), node->get_info()); break;
    case VarMetaData::BINDING:
        emit(ByteCode::sa_code_a(ByteCode::LOADL, bindindex2addr(node->ref->index), get_typebit(node->prog_type)), node->get_info()); break;
    case VarMetaData::LOCAL:
        emit(ByteCode::sa_code_a(ByteCode::LOADL, localindex2addr(node->ref->index), get_typebit(node->prog_type)), node->get_info()); break;
    case VarMetaData::GLOBAL:
        emit(ByteCode::sa_code_a(ByteCode::LOADG, node->ref->index), node->get_info()); break;
    default:
        break;
    }
}

void IRCodeGenerator::process_array(const ArrayNode* node) {
    // alloc + store
    auto tp = get_typebit(node->prog_type->as<PrimitiveType>()->args[0]);
    emit(ByteCode::consti(node->children.size()), node->get_info());
    emit(ByteCode::na_code(ByteCode::ALLOC, tp), node->get_info());
    for (size_t i = 0; i < node->children.size(); i++) {
        emit(ByteCode::na_code(ByteCode::DUP), node->get_info());
        emit(ByteCode::consti(i), node->get_info());
        process_expr(node->children[i]);
        emit(ByteCode::na_code(ByteCode::STOREI, tp), node->get_info());
    }
}

void IRCodeGenerator::process_tuple(const TupleNode* node) {
    // currently tuple has backend as array(object).
    emit(ByteCode::consti(node->children.size()), node->get_info());
    emit(ByteCode::na_code(ByteCode::ALLOC, 3), node->get_info());
    for (size_t i = 0; i < node->children.size(); i++) {
        emit(ByteCode::na_code(ByteCode::DUP), node->get_info());
        emit(ByteCode::consti(i), node->get_info());
        process_expr(node->children[i]);
        emit(ByteCode::na_code(ByteCode::STOREI, 3), node->get_info());
    }
}

void IRCodeGenerator::process_struct(const StructNode* node) {
    
    auto layout = struct2layoutaddr(std::static_pointer_cast<StructType>(node->prog_type));
    auto info = irprog->fetch_constant(layout)->as<ClassLayout>()->info_index;

    emit(ByteCode::sa_code_a(ByteCode::NEW, layout), node->get_info());
    for (const auto& f : node->children) {
        emit(ByteCode::na_code(ByteCode::DUP), node->get_info());
        Size_t field_offset = lookup_field_index(info, field_ids.at(f.first->get_name()));
        process_expr(f.second);
        emit(ByteCode::sa_code_a(ByteCode::STOREFIELD, field_offset), node->get_info());
    }
}

void IRCodeGenerator::process_funcall(const FunCallNode* node) {
    // call...

    process_expr(node->caller);
    for (const auto& a : node->args) {
        process_expr(a);
    }
    emit(ByteCode::sa_code_i(ByteCode::CALLA, node->args.size()), node->get_info());
    emit(ByteCode::na_code(ByteCode::SWAP), node->get_info());      // remove the function itself
    emit(ByteCode::na_code(ByteCode::POP), node->get_info());
}

void IRCodeGenerator::process_getfield(const GetFieldNode* node, const std::shared_ptr<ExprNode>& assignment_expr) {
    process_expr(node->lhs);
    if (assignment_expr) process_expr(assignment_expr);

    if (node->lhs->prog_type->is_concrete()) {
        // concrete => find class => get field ref 
        Size_t infoaddr = type2infoaddr(node->lhs->prog_type);
        Size_t field_index = lookup_field_index(infoaddr, field_ids.at(node->field->get_name()));
        emit(ByteCode::sa_code_a(assignment_expr ? ByteCode::STOREFIELD : ByteCode::LOADFIELD, field_index), node->get_info());
    }
    else if (node->lhs->prog_type->is_universal_variable()) {
        emit(ByteCode::sa_code_a(assignment_expr ? ByteCode::STOREINTERFACE : ByteCode::LOADINTERFACE, field_ids.at(node->field->get_name())), node->get_info());
    }
    else {
        throw std::runtime_error("Incorrect type in getfield");
    }
}

void IRCodeGenerator::process_new(const NewNode* node) {
    // as a funcall.

    // Load the new_A: A->(...->A)
    emit(ByteCode::sa_code_a(ByteCode::LOADG, node->constructor_ref->index), node->get_info());
    if (node->self_arg) {
        process_var(node->self_arg.get());
    }
    else {
        auto layout = type_addr[node->type_ref->index];
        emit(ByteCode::sa_code_a(ByteCode::NEW, layout), node->get_info());
    }
    emit(ByteCode::sa_code_i(ByteCode::CALLA, 1), node->get_info());
    emit(ByteCode::na_code(ByteCode::SWAP), node->get_info());      // remove the function itself
    emit(ByteCode::na_code(ByteCode::POP), node->get_info());
}

void IRCodeGenerator::process_let(const LetNode* node, bool create_field) {
    // storel/storeg
    // note: let always create a new variable with/without symbol conflict
    // the checking should be done in attributor.

    // TODO static variables (which should be stored in global table)
    if (create_field) {
        add_field(node->symbol->get_name(), node->vtype->prog_type);
    }

    if (node->expr) {
        process_expr(node->expr, node->symbol->get_name());
    }

    switch (node->ref->source)
    {
    case VarMetaData::LOCAL:
        if (node->expr) emit(ByteCode::sa_code_a(ByteCode::STOREL, localindex2addr(node->ref->index), get_typebit(node->ref->prog_type)), node->get_info());
        cur_function()->sz_local++;
        break;
    case VarMetaData::GLOBAL:
        if (node->expr) emit(ByteCode::sa_code_a(ByteCode::STOREG, node->ref->index), node->get_info());
        break;
    default:
        throw std::runtime_error("Incorrect source in let");
    }
}

void IRCodeGenerator::process_set(const SetNode* node) {
    // storel/storeg/storefield

    // set variable
    if (node->lhs->get_type() == AST::VAR) {
        auto lhs = node->lhs->as<VarNode>();
        process_expr(node->expr, lhs->symbol->get_name());
        switch (lhs->ref->source)
        {
        case VarMetaData::ARG:  // set to argument is allowed; However it won't modify the external one (except for pointer)
            emit(ByteCode::sa_code_a(ByteCode::STOREL, argindex2addr(lhs->ref->index), get_typebit(lhs->ref->prog_type)), node->get_info());
            break;
        case VarMetaData::LOCAL:
            emit(ByteCode::sa_code_a(ByteCode::STOREL, localindex2addr(lhs->ref->index), get_typebit(lhs->ref->prog_type)), node->get_info());
            break;
        case VarMetaData::GLOBAL:
            emit(ByteCode::sa_code_a(ByteCode::STOREG, lhs->ref->index), node->get_info());
            break;
        default:
            throw std::runtime_error("Incorrect source in set");
        }
    }
    else {  // set field
        auto lhs = node->lhs->as<GetFieldNode>();
        process_getfield(lhs, node->expr);
    }

}

void IRCodeGenerator::process_lambda(const LambdaNode* node, const std::string& name) {
    // findex is for constant pool
    Size_t findex = push_lambda_env(node->args.size(), node->bindings.size(), name.empty() ? "<lambda>" : name,
        node->get_info(), node->prog_type);

    for (const auto& s : node->statements) {
        if (s->is_expr()) {
            process_expr(std::static_pointer_cast<ExprNode>(s));
        }
        else if (s->get_type() == AST::LET) {
            process_let(const_ast_cast<LetNode>(s), false);
        }
        else if (s->get_type() == AST::SET) {
            process_set(const_ast_cast<SetNode>(s));
        }
        else {
            throw std::runtime_error("Incorrect AST Type");
        }
    }

    auto& ret_type = (node->prog_type->is_primitive() ? node->prog_type : node->prog_type->as<UniversalType>()->body )->as<PrimitiveType>()->args.back();
    if (ret_type->is_primitive() && ret_type->as<PrimitiveType>()->is_nil()) {   // return nil (type checking is done in attributor
        emit(ByteCode::na_code(ByteCode::RETN), node->get_info());
    }
    else {
        emit(ByteCode::na_code(ByteCode::RET, get_typebit(ret_type)), node->get_info());
    }


    pop_lambda_env();

    // evaluate the binding variables. Note: binding variables are not changed
    // even if they are modified externally. Use boxes to avoid such problem.
    for (const auto& ref : node->bindings) {
        switch (ref->source) {
        case VarMetaData::LOCAL:
            emit(ByteCode::sa_code_a(ByteCode::LOADL, localindex2addr(ref->index), get_typebit(ref->prog_type)), node->get_info()); break;
        case VarMetaData::BINDING:
            emit(ByteCode::sa_code_a(ByteCode::LOADL, bindindex2addr(ref->index), get_typebit(ref->prog_type)), node->get_info()); break;
        case VarMetaData::ARG:
            emit(ByteCode::sa_code_a(ByteCode::LOADL, argindex2addr(ref->index), get_typebit(ref->prog_type)), node->get_info()); break;
        default:
            throw std::runtime_error("Incorrect binding variable source");
        }
    }
    emit(ByteCode::sa_code_a(ByteCode::NEWCLOSURE, findex), node->get_info());
}

void IRCodeGenerator::process_class(const ClassNode* node) {
    auto rref = node->ref->as<ObjectTypeMetaData>();
    push_class_env(node->symbol->get_name(), node->get_info(), rref->index);
    for (const auto& name : rref->field_names) {
        add_field(name, rref->fields[name]);
    }
    pop_class_env();
    
    // constuctor: a global function
    add_field(SymbolTable::constructor_name(node->symbol->name), node->constructor->prog_type);
    process_lambda(node->constructor.get(), SymbolTable::constructor_name(node->symbol->get_name()));
    emit(ByteCode::sa_code_a(ByteCode::STOREG, node->constructor_ref->index), node->get_info());
}

void IRCodeGenerator::build_system_lib(const SymbolTable& symbol_table) {
    for (const auto& f : BuiltinSymbolGenerator::builtin_function_info) {
        auto prog_type = symbol_table.find_var(f.name)->prog_type;
        Size_t nargs = prog_type->is_primitive() ? prog_type->as<PrimitiveType>()->args.size() - 1
            : prog_type->as<UniversalType>()->body->as<PrimitiveType>()->args.size() - 1;
        auto gindex = add_field(f.name, prog_type);
        auto findex = push_lambda_env(nargs, 0, f.name, SymbolInfo::absolute(), prog_type);
        cur_function()->codes = f.codes;
        irprog->constant_pool[irprog->line_number_table_index]->as<LineNumberTable>()->add_entry(
            function_stack.back(), 0, 0
        );
        pop_lambda_env();
        emit(ByteCode::sa_code_a(ByteCode::NEWCLOSURE, findex), SymbolInfo(Location(0, 0, 0)));
        emit(ByteCode::sa_code_a(ByteCode::STOREG, gindex), SymbolInfo(Location(0, 0, 0)));
    }
}

void IRCodeGenerator::build_system_type(const SymbolTable& symbol_table) {
    for (const auto& sti : BuiltinSymbolGenerator::builtin_type_info) {
        auto ci = new ClassInfo();
        auto cindex = irprog->add_constant(ci);

        ci->name_index = add_string(sti.name);
        ci->symbol_info = SymbolInfo::absolute();
        type_addr[BuiltinSymbolGenerator::builtin_type_index(symbol_table.find_type(sti.name)->as<PrimitiveTypeMetaData>()->get_type())] = cindex;
    }
}

Size_t IRCodeGenerator::push_lambda_env(Size_t narg, Size_t nbind, const StringRef& name, const SymbolInfo& info, const pType& type) {

    Function* f = new Function();
    Size_t findex = irprog->add_constant(f);
    function_stack.push_back(findex);

    f->sz_arg = narg;
    f->sz_bind = nbind;
    f->sz_local = 0;

    FunctionInfo* fi = new FunctionInfo();
    f->info_index = irprog->add_constant(fi);
    fi->name_index = add_string(name);
    fi->symbol_info = info;

    const auto& funtype = type->is_universal() ? type->as<UniversalType>()->body : type;
    // funtype cannot be nested UT -- since the syntax does not allow it.

    if (!info.is_absolute()) fi->symbol_info.location = translate_location(info.location);
    for (unsigned i = 0; i < funtype->as<PrimitiveType>()->args.size() - 1; i++) {
        fi->arg_index.push_back(type2infoaddr(funtype->as<PrimitiveType>()->args[i]));
    }
    fi->ret_index = type2infoaddr(funtype->as<PrimitiveType>()->args.back());

    return findex;
}

Size_t IRCodeGenerator::push_class_env(const StringRef& name, const SymbolInfo& info, Index_t type_index) {

    ClassLayout* cl = new ClassLayout();
    cl->offset.push_back(4);        // initial offset for classinfo.

    Size_t cindex = irprog->add_constant(cl);
    class_stack.push_back(cindex);
    type_addr[type_index] = cindex;

    ClassInfo* ci = new ClassInfo();
    cl->info_index = irprog->add_constant(ci);

    ci->name_index = add_string(name);
    ci->symbol_info = info;

    return cindex;
}
