
#include "attributor.h"
#include "mini.h"

using namespace mini;

void Attributor::process(const std::vector<pAST>& nodes, SymbolTable& sym_table, ErrorManager* error_manager) {
    this->symbol_table = &sym_table;

    for (const auto& node : nodes) {
        try {
            process_node(node);
        }
        catch (const ParsingError& e) {
            if (error_manager && !error_manager->has_enough_errors()) {
                error_manager->count_and_print_error(e);
                while (symbol_table->cur_scope() != 0) {
                    symbol_table->pop_scope();
                }
                binding_cache.clear();
            }
            else {
                throw;
            }
        }
    }
    this->symbol_table = nullptr;
}

void Attributor::process_node(const pAST& node) {
    if (node->is_expr()) {
        process_expr(std::static_pointer_cast<ExprNode>(node));
    }
    else if (node->get_type() == AST::LET) {
        process_let(ast_cast<LetNode>(node));
    }
    else if (node->get_type() == AST::SET) {
        process_set(ast_cast<SetNode>(node));
    }
    else if (node->get_type() == AST::INTERFACE) {
        process_interface(ast_cast<InterfaceNode>(node));
    }
    else if (node->get_type() == AST::CLASS) {
        process_class(ast_cast<ClassNode>(node));
    }
    else {
        throw std::runtime_error("Incorrect AST type");
    }
}

// Process a single statement inside lambda.

void Attributor::process_statement(const pAST& node) {
    if (node->is_expr()) {
        process_expr(std::static_pointer_cast<ExprNode>(node));
    }
    else if (node->get_type() == AST::LET) {
        process_let(ast_cast<LetNode>(node));
    }
    else if (node->get_type() == AST::SET) {
        process_set(ast_cast<SetNode>(node));
    }
    else {
        throw std::runtime_error("Incorrect AST type");
    }
}

// turn a constant to a boxed one.
Ptr<ExprNode> Attributor::make_boxed_constant_node(const Ptr<ConstantNode>& node) {
    auto m = std::make_shared<FunCallNode>();
    auto m_lhs = std::make_shared<NewNode>();
    m->set_info(node->get_info());
    m_lhs->set_info(node->get_info());
    m->caller = m_lhs;
    
    switch (node->value.get_type()) {
    case Constant::Type_t::BOOL:{
        // After Enum is introduced, lower case true/false will be no longer valid. 
        // True and False will be two enum flags (also variables) -- equivalent to .index.eq(1) and .index.eq(0)
        if (node->value == Constant(true)){
            return VarNode::make_attributed(symbol_table->find_var("True"), node->get_info());
        }
        else{
            return VarNode::make_attributed(symbol_table->find_var("False"), node->get_info());
        }
        break;
    }
    case Constant::Type_t::CHAR: {
        m_lhs->symbol = std::make_shared<Symbol>("Char", node->get_info());
        m->args.push_back(node);
        break;
    }
    case Constant::Type_t::INT: {
        m_lhs->symbol = std::make_shared<Symbol>("Int", node->get_info());
        m->args.push_back(node);
        break;
    }
    case Constant::Type_t::FLOAT: {
        m_lhs->symbol = std::make_shared<Symbol>("Float", node->get_info());
        m->args.push_back(node);
        break;
    }
    case Constant::Type_t::STRING: {
        m_lhs->symbol = std::make_shared<Symbol>("String", node->get_info());
        m->args.push_back(node);
        break;
    }
    default: node->get_info().throw_exception("Unrecognized constant");
    }
    return m;
}

void Attributor::process_tuple(TupleNode* m_node) {

    if (m_node->children.size() == 0) {
        m_node->get_info().throw_exception("Tuple cannot be empty");
    }

    std::vector<Ptr<Type>> type_args;
    for (auto& c : m_node->children) {
        process_expr(c);
        type_args.push_back(c->prog_type);
    }
    m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("tuple"), type_args);
}

void Attributor::process_struct(StructNode* m_node) {

    std::unordered_map<std::string, pType> fields;

    for (auto& c : m_node->children) {
        process_expr(c.second);
        auto r = fields.insert({ c.first->name, c.second->get_prog_type() });
        if (!r.second) {
            c.first->get_info().throw_exception("Field redefinition: " + c.first->get_name());
        }

    }
    if (m_node->children.size() == 0) {
        m_node->prog_type = std::make_shared<StructType>();
    }
    else {
        m_node->prog_type = std::make_shared<StructType>(fields);
    }
}

void Attributor::process_array(ArrayNode* m_node) {

    pType maximum_type;

    if (m_node->children.empty()) {
        maximum_type = std::make_shared<PrimitiveType>(symbol_table->find_type("bottom"));
    }
    else {  // type inference
        process_expr(m_node->children[0]);
        maximum_type = m_node->children[0]->prog_type;

        for (size_t i = 1; i < m_node->children.size(); i++) {
            process_expr(m_node->children[i]);
            make_maximum_type(maximum_type, m_node->children[i]->prog_type, m_node->get_info());
        }
    }
    m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("array"), std::vector<pType>({ maximum_type }));
}

void Attributor::process_getfield(GetFieldNode* m_node) {
    process_expr(m_node->lhs);
    auto lhs_type = const_cast<ConstTypeRef>(m_node->lhs->get_prog_type().get());
    const std::unordered_map<std::string, pType>* fields = nullptr;

    while (lhs_type->is_universal_variable() && lhs_type->as<UniversalTypeVariable>()->quantifier) {
        lhs_type = lhs_type->as<UniversalTypeVariable>()->quantifier;
    }

    if (lhs_type->is_struct()) {
        fields = &(lhs_type->as<StructType>()->fields);
    }
    else if (lhs_type->is_object()) {
        fields = &(lhs_type->as<ObjectType>()->ref->fields);
    }
    else {
        m_node->lhs->get_info().throw_exception("Struct/class required");
    }

    auto f = fields->find(m_node->field->get_name());
    if (f == fields->end()) {
        m_node->field->get_info().throw_exception(StringAssembler("Type '")(*lhs_type)("' does not have field '")(m_node->field->get_name())("'")());
    }
    m_node->set_prog_type(f->second);
}

// let lhs = rhs;
pAST make_let_node(const pSymbol& lhs, const Ptr<ExprNode>& rhs) {
    auto m = std::make_shared<LetNode>(lhs->get_info(), lhs, nullptr, rhs);
    return m;
}

// lhs.eq(rhs)
Ptr<ExprNode> make_equal_node(const Ptr<ExprNode>& lhs, const Ptr<ExprNode>& rhs) {
    auto m_gf = std::make_shared<GetFieldNode>(
        lhs->get_info(),
        lhs,
        std::make_shared<Symbol>("eq", lhs->get_info())
        );
    auto m = std::make_shared<FunCallNode>(
        lhs->get_info(),
        m_gf,
        std::vector<Ptr<ExprNode>>{rhs});
    return m;
}

// name(args)
Ptr<ExprNode> make_builtin_funcall_node(const std::string& name, const SymbolInfo& info, const std::initializer_list<Ptr<ExprNode>>&& args, const SymbolTable* symbol_table) {
    auto m = std::make_shared<FunCallNode>(
        info,
        VarNode::make_attributed(symbol_table->find_var(name), info),
        std::vector<Ptr<ExprNode>>{args}
    );
    return m;
}

// name<type>(args)
Ptr<ExprNode> make_builtin_funcall_node(const std::string& name, const SymbolInfo& info, const pType& prog_type, const std::initializer_list<Ptr<ExprNode>>&& args, const SymbolTable* symbol_table) {
    auto m_typeappl = std::make_shared<TypeApplNode>(
        info,
        VarNode::make_attributed(symbol_table->find_var(name), info),
        std::vector<Ptr<TypeNode>>{TypeNode::make_attributed(prog_type, info)}
    );

    auto m = std::make_shared<FunCallNode>(
        info,
        m_typeappl,
        std::vector<Ptr<ExprNode>>{args});
    return m;
}


Ptr<LambdaNode> Attributor::process_case_branch(const CaseNode::Case& m_case, const pSymbol& arg_name, const pSymbol& previous_branch_name, const pType& arg_type, bool is_last_one) {

    const auto& condition_info = m_case.condition->get_info();

    auto m_node = std::make_shared<LambdaNode>();
    m_node->set_info(condition_info);
    Ptr<ExprNode> condition_expr = nullptr;
    Ptr<ExprNode> arg_var_node = std::make_shared<VarNode>(
        std::make_shared<Symbol>(arg_name->get_name(), m_case.condition->get_info())
        );  // This cannot be attributed, since it may be binded.

    switch (m_case.condition->get_type())
    {
    case AST::Type_t::VAR: {        // let var = lhs_var
        m_node->statements.push_back(make_let_node(m_case.condition->as<VarNode>()->symbol, arg_var_node));
        break;
    }
    case AST::Type_t::CONSTANT: {   // lhs_var.eq(constant)
        condition_expr = make_equal_node(arg_var_node, m_case.condition);
        break;
    }
    case AST::Type_t::TUPLE: {      // structure binding
        if (!arg_type->is_primitive() || arg_type->as<PrimitiveType>()->type_name() != PrimitiveTypeMetaData::TUPLE) {
            m_case.condition->get_info().throw_exception(StringAssembler("Not a tuple type: ")(*arg_type)());
        }

        auto tuple_type = arg_type->as<PrimitiveType>();
        if (tuple_type->args.size() != m_case.condition->as<TupleNode>()->children.size()) {
            condition_info.throw_exception(
                StringAssembler("Number of elements not match: Expect ")(tuple_type->args.size())(", got ")(m_case.condition->as<TupleNode>()->children.size())());
        }

        for (int i = 0; i < tuple_type->args.size(); i++) {
            auto& child = m_case.condition->as<TupleNode>()->children[i];
            auto get_node = make_builtin_funcall_node("@get", child->get_info(), tuple_type->args[i], 
                { arg_var_node, std::make_shared<ConstantNode>(Constant(i), child->get_info())}, symbol_table);  // get<TYPE>(lhs_var, i)

            if (child->get_type() == AST::Type_t::VAR) {    // let child = get<TYPE>(lhs_var, i);
                m_node->statements.push_back(make_let_node(child->as<VarNode>()->symbol, get_node));
            }
            else if (child->get_type() == AST::Type_t::CONSTANT) {  // condition_expr = and(condition_expr, get<TYPE>(lhs_var, i).eq(child))
                auto equal_node = make_equal_node(get_node, child);
                if (condition_expr) {
                    auto m = make_builtin_funcall_node("and", child->get_info(), { condition_expr, equal_node}, symbol_table);
                    condition_expr.swap(m);
                }
                else {
                    condition_expr = equal_node;
                }
            }
            else {
                child->get_info().throw_exception("Unrecognized structure binding");
            }
        }
        break;
    }
    case AST::Type_t::STRUCT: {
        // we don't check the type of getfield. will leave that to getfield.
        for (const auto& [m_field, child] : m_case.condition->as<StructNode>()->children) {
            auto get_node = std::make_shared<GetFieldNode>(condition_info, arg_var_node, m_field);   // lhs_var.m_field
            if (child->get_type() == AST::Type_t::VAR) {  // let child = lhs_var.m_field
                m_node->statements.push_back(make_let_node(child->as<VarNode>()->symbol, get_node));
            }
            else if (child->get_type() == AST::Type_t::CONSTANT) {  // condition_expr = and(condition_expr, lhs_var.m_field.eq(child))
                auto equal_node = make_equal_node(get_node, child);
                if (condition_expr) {
                    auto m = make_builtin_funcall_node("and", child->get_info(), { condition_expr, equal_node }, symbol_table);
                    condition_expr.swap(m);
                }
                else {
                    condition_expr = equal_node;
                }
            }
            else {
                child->get_info().throw_exception("Unrecognized structure binding");
            }
        }
        break;
    }
    default:
        condition_info.throw_exception("Invalid case expression. Must be id/constant/tuple/struct");
    }

    if (m_case.guard && !condition_expr) {
        condition_expr = m_case.guard;
    }
    else if (m_case.guard) {    // sel(condition_expr, \->_case.guard, \->False)()
        auto cond_lhs = std::make_shared<LambdaNode>(m_case.guard->get_info(), std::vector<pAST>{ m_case.guard });
        auto cond_rhs = std::make_shared<LambdaNode>(m_case.guard->get_info(), std::vector<pAST>{
            VarNode::make_attributed(symbol_table->find_var("False"), m_case.guard->get_info())
        });
        auto sel_call = make_builtin_funcall_node("sel", m_case.guard->get_info(), {condition_expr, cond_lhs, cond_rhs}, symbol_table);
        Ptr<ExprNode> sel = std::make_shared<FunCallNode>(m_case.guard->get_info(), sel_call, std::vector<Ptr<ExprNode>>{});
        condition_expr.swap(sel);
    }

    if (condition_expr) {
        /* final_expr = sel(condition_expr, \->expr, next_case )() */
        auto br_lhs = std::make_shared<LambdaNode>(condition_info, std::vector<pAST>{m_case.expr});
        auto br_rhs = std::make_shared<VarNode>(previous_branch_name);
        br_rhs->set_info(condition_info);
        auto sel_call = make_builtin_funcall_node("sel", condition_info, { condition_expr, br_lhs, br_rhs }, symbol_table);
        m_node->statements.push_back(std::make_shared<FunCallNode>(condition_info, sel_call, std::vector<Ptr<ExprNode>>{}));
    }
    else {  // then that's it -- it always matched.
        m_node->statements.push_back(m_case.expr);
        if (!is_last_one) {
            condition_info.throw_exception("Next cases will be unreachable");
        }
    }
    return m_node;
}