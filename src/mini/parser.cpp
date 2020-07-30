
#include "parser.h"

using namespace mini;

Ptr<LetNode> mini::Parser::parse_let_wb() {
    // assuming the let is parsed.

    auto m_node = std::make_shared<LetNode>();
    m_node->set_info(get_last_info());

    match_token(Token::ID);
    m_node->symbol = get_id_inc();
    match_keyword_inc(Keyword::COLON);

    m_node->vtype = parse_type();

    if (test_keyword_inc(EQ)) {
        m_node->expr = parse_expr();
    }

    return m_node;

}

Ptr<SetNode> mini::Parser::parse_set_wb() {
    auto m_node = std::make_shared<SetNode>();
    m_node->set_info(get_last_info());

    m_node->lhs = parse_expr();
    if (m_node->lhs->get_type() != AST::VAR && m_node->lhs->get_type() != AST::GETFIELD) {
        throw_cur_token("set must apply on variable or field");
    }

    match_keyword_inc(Keyword::EQ);
    m_node->expr = parse_expr();

    return m_node;
}

Ptr<TypeNode> mini::Parser::parse_type() {

    auto m_node = std::make_shared<TypeNode>();
    if (test_token(Token::CONSTANT) && std::get<Constant>(cur_token->value).get_type() == Constant::Type_t::NIL) {
        m_node->symbol = std::make_shared<Symbol>("nil", cur_token->get_info());
        m_node->set_info(cur_token->get_info());
        cur_token++;
        return m_node;
    }

    match_token(Token::ID);

    m_node->symbol = get_id_inc();
    m_node->set_info(get_last_info());

    if (test_keyword_inc(Keyword::LBRACKET)) {
        while (1) {
            m_node->args.push_back(parse_type());
            if (test_keyword_inc(Keyword::RBRACKET)) break;
            else if (test_keyword_inc(Keyword::COMMA)) continue;
            else {
                throw_cur_token("`,' or `)' expected");
            }
        }
    }
    return m_node;
}

Ptr<ExprNode> mini::Parser::parse_expr() {

    match_no_end();

    Ptr<ExprNode> prefix;

    switch (cur_token->get_type()) {
    case Token::CONSTANT: {
        prefix = std::make_shared<ConstantNode>(get_token_data_inc<Constant>());
        prefix->set_info(get_last_info());
        return prefix;  // constant cannot perform function call or dot
    }
    case Token::ID: {
        prefix = std::make_shared<VarNode>(get_id_inc());
        prefix->set_info(get_last_info());
        break;
    }
    case Token::KEYWORD: {
        auto keyword = get_token_data_inc<Keyword>();
        switch (keyword)
        {
        case Keyword::BACKSLASH: prefix = parse_lambda_wb(); break;
        case Keyword::LCURLY: prefix = parse_struct_wb(); break;
        case Keyword::LSQBRACKET: prefix = parse_array_wb(); break;
        case Keyword::LBRACKET: {
            if (test_keyword_inc(RBRACKET)) {    // nil
                prefix = std::make_shared<ConstantNode>(Constant());
                prefix->set_info(get_last_info());
                break;
            }
            prefix = parse_expr();
            if (test_keyword_inc(COMMA)) {       // a tuple
                auto m_node = std::make_shared<TupleNode>();
                m_node->children.push_back(prefix);
                m_node->set_info(get_last_info());
                if (!test_keyword_inc(RBRACKET)) {
                    while (1) {
                        m_node->children.push_back(parse_expr());
                        if (test_keyword_inc(COMMA)) {
                            if (test_keyword_inc(RBRACKET)) break;
                        }
                        else if (test_keyword_inc(RBRACKET)) break;
                        else {
                            throw_cur_token("`,' or `)' expected");
                        }
                    }
                }
                std::static_pointer_cast<ExprNode>(m_node).swap(prefix);    // set prefix to be the expression
            }
            else {
                match_keyword_inc(RBRACKET);
            }
            break;
        }

        default:
            cur_token--;
            throw_cur_token("Unkown symbol: " + to_str(keyword));
            break;
        }
    }
    }

    // function call / member
    while (1) {
        if (test_keyword_inc(Keyword::LBRACKET)) {
            auto m_node = std::make_shared<FunCallNode>();
            m_node->set_info(get_last_info());
            m_node->caller = prefix;

            if (!test_keyword_inc(Keyword::RBRACKET)) {
                while (1) {
                    m_node->args.push_back(parse_expr());
                    if (test_keyword_inc(Keyword::COMMA)) continue;
                    else if (test_keyword_inc(Keyword::RBRACKET)) break;
                    else {
                        throw_cur_token("`,' or `)' expected");
                    }
                }
            }
            std::static_pointer_cast<ExprNode>(m_node).swap(prefix);    // set prefix to be the lhs
        }
        else if (test_keyword_inc(Keyword::DOT)) {
            auto m_node = std::make_shared<GetFieldNode>();
            m_node->lhs = prefix;
            m_node->field = get_id_inc();
            std::static_pointer_cast<ExprNode>(m_node).swap(prefix);
        }
        else {
            return prefix;
        }
    }

}

Ptr<StructNode> mini::Parser::parse_struct_wb() {
    auto m_node = std::make_shared<StructNode>();
    m_node->set_info(get_last_info());
    if (test_keyword_inc(Keyword::RCURLY)) {
        return m_node;
    }
    while (1) {
        match_token(Token::ID);
        auto name = get_id_inc();
        match_keyword_inc(Keyword::EQ);
        m_node->children.push_back({ name, parse_expr() });

        if (test_keyword_inc(Keyword::COMMA)) continue;
        else if (test_keyword_inc(Keyword::RCURLY)) break;
        else {
            throw_cur_token("`,' or `}' expected");
        }
    }

    return m_node;
}

Ptr<ArrayNode> mini::Parser::parse_array_wb() {
    auto m_node = std::make_shared<ArrayNode>();
    m_node->set_info(get_last_info());
    if (!test_keyword_inc(Keyword::RSQBRACKET)) {
        while (1) {
            m_node->children.push_back(parse_expr());
            if (test_keyword_inc(Keyword::COMMA)) continue;
            else if (test_keyword_inc(Keyword::RSQBRACKET)) break;
            else {
                throw_cur_token("`,' or `]' expected");
            }
        }
    }

    return m_node;
}

Ptr<LambdaNode> mini::Parser::parse_lambda_wb() {
    auto m_node = std::make_shared<LambdaNode>();
    m_node->set_info(get_last_info());

    if (test_token(Token::ID)) {
        auto name = get_id_inc();
        match_keyword_inc(Keyword::COLON);
        auto type = parse_type();
        m_node->args.push_back({ name, type });
    }
    else {
        match_keyword_inc(Keyword::LBRACKET);
        if (!test_keyword_inc(Keyword::RBRACKET)) {
            while (1) {
                auto name = get_id_inc();
                match_keyword_inc(Keyword::COLON);
                auto type = parse_type();
                m_node->args.push_back({ name, type });

                if (test_keyword_inc(Keyword::COMMA)) continue;
                else if (test_keyword_inc(Keyword::RBRACKET)) break;
                else {
                    throw_cur_token("`,' or `)' expected");
                }
            }
        }
    }
    match_keyword_inc(Keyword::ARROW);
    // do resolve ambiguity with struct here
    if (test_keyword_inc(Keyword::LCURLY)) {
        // maybe a struct? look ahead
        bool is_struct = false;

        if (test_token(Token::ID)) {
            cur_token++;
            if (test_keyword(Keyword::EQ)) {
                is_struct = true;
            }
            cur_token--;
        }
        else if (test_keyword(Keyword::RCURLY)) {
            is_struct = true;
        }

        if (is_struct) {
            m_node->statements.push_back(parse_struct_wb());
        }
        else {
            parse_statement_list(m_node->statements);
            match_keyword_inc(Keyword::RCURLY);
        }
    }
    else {
        m_node->statements.push_back(parse_expr());
    }
    match_keyword_inc(Keyword::COLON);
    m_node->ret_type = parse_type();

    return m_node;
}

Ptr<LetNode> mini::Parser::parse_func_def_wb() {
    auto m_node = std::make_shared<LetNode>();
    auto m_funnode = std::make_shared<LambdaNode>();
    auto m_typenode = std::make_shared<TypeNode>();
    m_typenode->symbol = std::make_shared<Symbol>("function", cur_token->get_info());

    match_token(Token::ID);

    m_node->symbol = get_id_inc();
    m_node->set_info(get_last_info());
    m_funnode->set_info(get_last_info());
    m_typenode->set_info(get_last_info());

    match_keyword_inc(Keyword::LBRACKET);
    if (!test_keyword_inc(Keyword::RBRACKET)) {
        while (1) {
            match_token(Token::ID);
            auto name = get_id_inc();
            match_keyword_inc(Keyword::COLON);
            auto type = parse_type();
            m_funnode->args.push_back({ name, type });
            m_typenode->args.push_back(type);

            if (test_keyword_inc(Keyword::COMMA)) continue;
            else if (test_keyword_inc(Keyword::RBRACKET)) break;
            else {
                throw_cur_token("`,' or `)' expected");
            }
        }
    }
    match_keyword_inc(Keyword::ARROW);
    m_funnode->ret_type = parse_type();
    m_typenode->args.push_back(m_funnode->ret_type);
    m_node->vtype = m_typenode;

    match_keyword_inc(Keyword::LCURLY);
    parse_statement_list(m_funnode->statements);
    match_keyword_inc(Keyword::RCURLY);
    m_node->expr = m_funnode;

    return m_node;
}

void mini::Parser::parse_statement_list(std::vector<Ptr<AST>>& statements) {
    while (1) {
        if (test_keyword_inc(Keyword::LET)) {
            statements.push_back(parse_let_wb());
        }
        else if (test_keyword_inc(Keyword::SET)) {
            statements.push_back(parse_set_wb());
        }
        else if (test_keyword_inc(Keyword::DEF)) {
            statements.push_back(parse_func_def_wb());
        }
        else {
            statements.push_back(parse_expr());
        }
        if (!test_keyword_inc(Keyword::COMMA)) break;
    }
}

Ptr<ClassNode> mini::Parser::parse_class_wb() {
    match_token(Token::ID);

    auto m_node = std::make_shared<ClassNode>(get_id_inc());
    m_node->set_info(get_last_info());

    while (1) {
        if (test_keyword_inc(EXTENDS)) {
            m_node->parents.push_back(parse_type());
        }
        else if (test_keyword_inc(IMPLEMENTS)) {
            m_node->interfaces.push_back(parse_type());
        }
        else {
            break;
        }
    }

    match_keyword_inc(Keyword::LCURLY);
    if (!test_keyword_inc(Keyword::RCURLY)) {
        while (1) {
            ClassNode::ClassMemberMeta m_meta;
            if (test_keyword_inc(Keyword::DEF)) {   // def must be static
                m_meta.is_static = true;
                m_node->members.push_back({
                    parse_func_def_wb(),
                    m_meta
                    });
            }
            else {
                if (test_keyword_inc(Keyword::STATIC)) {
                    m_meta.is_static = true;
                }
                else {
                    m_meta.is_static = false;
                }
                m_node->members.push_back({ parse_let_wb(), m_meta });
            }

            if (test_keyword_inc(Keyword::COMMA)) continue;
            else if (test_keyword_inc(Keyword::RCURLY)) break;
            else {
                throw_cur_token("`,' or `}' expected");
            }
        }
    }
    return m_node;
}

Ptr<InterfaceNode> mini::Parser::parse_interface_wb() {
    auto m_node = std::make_shared<InterfaceNode>(get_id_inc());
    match_keyword_inc(Keyword::LCURLY);
    if (!test_keyword(Keyword::RCURLY)) {
        while (1) {
            auto m_member = std::make_shared<LetNode>();
            m_member->symbol = get_id_inc();
            m_member->set_info(get_last_info());
            match_keyword_inc(Keyword::COLON);
            m_member->vtype = parse_type();
            m_node->add_member(m_member);

            if (test_keyword_inc(Keyword::COMMA)) continue;
            else if (test_keyword_inc(Keyword::RCURLY)) break;
            else {
                throw_cur_token("`,' or `}' expected");
            }
        }
    }

    return m_node;
}

Ptr<ImportNode> mini::Parser::parse_import_wb() {
    auto m_node = std::make_shared<ImportNode>();
    m_node->set_info(get_last_info());
    m_node->add_symbol(get_id_inc());
    while (test_keyword_inc(Keyword::DOT)) {
        m_node->add_symbol(get_id_inc());
    }
    return m_node;
}
