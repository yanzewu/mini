
#include "attributor.h"
#include "mini.h"

using namespace mini;

void Attributor::process_case_branch(std::vector<CaseNode::Case>::iterator m_case, std::vector<CaseNode::Case>::iterator m_case_end, VariableRef dummy_var, std::vector<pAST>& statements) {

    const auto& condition_info = m_case->condition->get_info();
    Ptr<ExprNode> condition_expr = NULL;
    Ptr<ExprNode> dummy_var_node = std::make_shared<VarNode>(
        std::make_shared<Symbol>(dummy_var->symbol->get_name(), m_case->condition->get_info())
        );  // This cannot be attributed, since it may be binded.

    switch (m_case->condition->get_type())
    {
    case AST::Type_t::VAR: {        // let var = dummy_var
        auto m = std::make_shared<LetNode>(
            condition_info,
            m_case->condition->as<VarNode>()->symbol,
            nullptr,
            dummy_var_node
            );
        statements.push_back(m);
        break;
    }
    case AST::Type_t::CONSTANT: {   // dummy_var.equals(constant)
        auto m_gf = std::make_shared<GetFieldNode>(
            condition_info,
            dummy_var_node,
            std::make_shared<Symbol>("equals", condition_info)
            );
        condition_expr = std::make_shared<FunCallNode>(
            condition_info,
            m_gf,
            std::vector<Ptr<ExprNode>>{m_case->condition});
        break;
    }
    case AST::Type_t::TUPLE: {      // structure binding
        // if dummy_var is not tuple => fail
        auto tuple_type = dummy_var->prog_type->as<PrimitiveType>();
        if (tuple_type->args.size() != m_case->condition->as<TupleNode>()->children.size()) {
            condition_info.throw_exception(
                StringAssembler("Number of elements not match: Expect ")(tuple_type->args.size())(", got ")(m_case->condition->as<TupleNode>()->children.size())());
        }
        for (size_t i = 0; i < tuple_type->args.size(); i++) {
            auto& child = m_case->condition->as<TupleNode>()->children[i];
            /*          get
                    <Type>
                (dummy_var, i)
            */
            Ptr<ExprNode> get_node = std::make_shared<FunCallNode>(
                child->get_info(),
                std::make_shared<TypeApplNode>(
                    condition_info,
                    VarNode::make_attributed(symbol_table->find_var("@get"), condition_info),
                    std::vector<Ptr<TypeNode>>{TypeNode::make_attributed(tuple_type->args[i], condition_info)}
            ),
                std::vector<Ptr<ExprNode>>{
                dummy_var_node,
                    std::make_shared<ConstantNode>(Constant(int(i)))
            }
            );

            if (child->get_type() == AST::Type_t::VAR) {    // let child = get<TYPE>(dummy_var, i);
                auto m = std::make_shared<LetNode>(
                    child->get_info(),
                    child->as<VarNode>()->symbol,
                    nullptr,
                    get_node
                    );
                statements.push_back(m);
            }
            else if (child->get_type() == AST::Type_t::CONSTANT) {  // condition_expr = and(condition_expr, child.equals(get<TYPE>(dummy_var, i)))
                auto m_gf = std::make_shared<GetFieldNode>(
                    child->get_info(),
                    child,
                    std::make_shared<Symbol>("equals", condition_info)
                    );
                auto m = std::make_shared<FunCallNode>(
                    child->get_info(),
                    VarNode::make_attributed(symbol_table->find_var("and"), child->get_info()),
                    std::vector<Ptr<ExprNode>>{
                    condition_expr,
                        get_node
                });
                auto t = std::static_pointer_cast<ExprNode>(m);
                condition_expr.swap(t);
            }
            else {
                child->get_info().throw_exception("Unrecognized structure binding");
            }
        }
        break;
    }
    default:
        condition_info.throw_exception("Invalid case expression. Must be id/constant/tuple");
    }

    if (m_case->guard && !condition_expr) {
        condition_expr = m_case->guard;
    }
    else if (m_case->guard) {    // sel(condition_expr, \->_case.guard, \->False)()
        auto cond_call_caller = std::make_shared<TypeApplNode>();
        cond_call_caller->lhs = VarNode::make_attributed(symbol_table->find_var("sel"), m_case->guard->get_info());
        cond_call_caller->args.push_back(TypeNode::make_attributed(
            PrimitiveTypeBuilder("function")(ObjectTypeBuilder("Bool")).build(*symbol_table),
            m_case->guard->get_info()
        ));

        auto cond_lhs = std::make_shared<LambdaNode>(m_case->guard->get_info(), std::vector<pAST>{ m_case->guard });
        auto cond_rhs = std::make_shared<LambdaNode>(m_case->guard->get_info(), std::vector<pAST>{
            VarNode::make_attributed(symbol_table->find_var("False"), m_case->guard->get_info())
        });

        auto cond_call = std::make_shared<FunCallNode>(
            m_case->guard->get_info(),
            cond_call_caller,
            std::vector<Ptr<ExprNode>>{condition_expr, cond_lhs, cond_rhs});
        auto t = std::static_pointer_cast<ExprNode>(cond_call);
        condition_expr.swap(t);
    }

    if (condition_expr) {
        /* final_expr =
                       sel
           +sel     <function(dumm_var_type)>
           +lhs   (condition_expr, \->expr, \->{rhs_statements } )
            ()
        */
        auto sel_function_type = std::make_shared<PrimitiveType>(
            symbol_table->find_type("function"),
            std::vector<pType>{dummy_var->prog_type});

        auto sel_lhs = std::make_shared<TypeApplNode>(
            condition_info,
            VarNode::make_attributed(symbol_table->find_var("sel"), condition_info),
            std::vector<Ptr<TypeNode>>{TypeNode::make_attributed(sel_function_type, condition_info)}
        );

        auto br_lhs = std::make_shared<LambdaNode>(condition_info, std::vector<pAST>{m_case->expr});
        auto br_rhs = std::make_shared<LambdaNode>();
        br_rhs->set_info(condition_info);
        if (m_case + 1 == m_case_end) {    // undefined()
            auto m_node = std::make_shared<FunCallNode>(
                m_case->expr->get_info(),
                VarNode::make_attributed(symbol_table->find_var("undefined"), m_case->expr->get_info()),
                std::vector<Ptr<ExprNode>>{}
            );
            br_rhs->statements.push_back(m_node);
        }
        else {
            process_case_branch(m_case + 1, m_case_end, dummy_var, br_rhs->statements);
        }
        auto sel = std::make_shared<FunCallNode>(
            condition_info,
            sel_lhs,
            std::vector<Ptr<ExprNode>>{condition_expr, br_lhs, br_rhs});
        statements.push_back(std::make_shared<FunCallNode>(
            condition_info,
            sel,
            std::vector<Ptr<ExprNode>>{}));
    }
    else {  // then that's it -- it always matched.
        statements.push_back(m_case->expr);
        if (++m_case != m_case_end) {
            condition_info.throw_exception("Cases will not be matched");
        }
    }
}