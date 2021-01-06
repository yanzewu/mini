
#include "parser.h"

using namespace mini;

void mini::Parser::parse(const std::vector<Token>& token_buffer, std::vector<Ptr<AST>>& ast_buffer, ErrorManager* error_manager) {

    std::vector<std::pair<TokenIter_t, TokenIter_t>> blocks;

    // scan declaration info.

    TokenIter_t block_begin = token_buffer.begin();
    for (auto it = token_buffer.begin(); it != token_buffer.end();) {
        if (it->get_type() == Token::KEYWORD && std::get<Keyword>(it->value) == Keyword::SEMICOLON) {
            if (it - block_begin > 0) {
                blocks.push_back({ block_begin, it });
            }
            it++;
            block_begin = it;
        }
        else {
            it++;
        }
    }
    if (block_begin != token_buffer.end()) {
        throw ParsingError("Semicolon does not match in the end");
    }

    for (const auto& block : blocks) {
        try {
            ast_buffer.push_back(parse_block(block.first, block.second));
        }
        catch (const ParsingError& e) {
            if (error_manager && !error_manager->has_enough_errors()) {
                error_manager->count_and_print_error(e);
            }
            else {
                throw;
            }
        }
    }

}

Ptr<AST> mini::Parser::parse_block(TokenIter_t token_begin, TokenIter_t token_end) {

    cur_token = token_begin;
    token_bound = token_end;

    Ptr<AST> ret;

    if (test_keyword_inc(Keyword::LET)) {
        ret = parse_let_wb();
    }
    else if (test_keyword_inc(Keyword::SET)) {
        ret = parse_set_wb();
    }
    else if (test_keyword_inc(Keyword::DEF)) {
        ret = parse_func_def_wb();
    }
    else if (test_keyword_inc(Keyword::CLASS)) {
        ret = parse_class_wb();
    }
    else if (test_keyword_inc(Keyword::INTERFACE)) {
        ret = parse_interface_wb();
    }
    else if (test_keyword_inc(Keyword::IMPORT)) {
        ret = parse_import_wb();
    }
    else {
        ret = parse_expr();
    }

    if (cur_token != token_end) {
        throw_cur_token("Unknown trailing sequence");
    }
    return ret;
}

Ptr<LetNode> mini::Parser::parse_let_wb(bool allow_auto) {
    // assuming the let is parsed.

    auto m_node = std::make_shared<LetNode>();
    m_node->set_info(get_last_info());

    match_token(Token::ID);
    m_node->symbol = get_id_inc();
    if (allow_auto && test_keyword_inc(Keyword::EQ)) {    // type inference
        m_node->expr = parse_expr();
        m_node->vtype = NULL;
    }
    else {
        match_keyword_inc(Keyword::COLON);
        m_node->vtype = parse_type();
        if (test_keyword_inc(EQ)) {
            m_node->expr = parse_expr();
        }
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
    if (test_keyword_inc(Keyword::FORALL)) {
        m_node->set_info(get_last_info());
        match_keyword_inc(Keyword::LANGLE);
        parse_quantifier_def_wb(m_node->quantifiers);
        match_keyword_inc(Keyword::DOT);
    }
    // special case: nil is simutaneously a type name and constant name
    if (test_token(Token::CONSTANT) && std::get<Constant>(cur_token->value).get_type() == Constant::Type_t::NIL) {
        m_node->symbol = std::make_shared<Symbol>("nil", cur_token->get_info());
        m_node->set_info(cur_token->get_info());
        cur_token++;
        return m_node;
    }

    match_token(Token::ID);

    m_node->symbol = get_id_inc();
    if (m_node->quantifiers.empty()) m_node->set_info(get_last_info());

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
        case Keyword::NEW:
        case Keyword::EXTENDS: {
            auto m_node = std::make_shared<NewNode>();
            m_node->set_info(get_last_info());
            m_node->symbol = get_id_inc();

            if (keyword == Keyword::EXTENDS) {  // distinguish call to base/call to self.
                m_node->self_arg = std::make_shared<VarNode>(std::make_shared<Symbol>("self", get_last_info()));
            }
            
            // if there are quantifier, will add it first so that transforming will be easier.
            if (test_keyword_inc(Keyword::LANGLE)) {
                while (1) {
                    m_node->type_args.push_back(parse_type());
                    if (test_keyword_inc(Keyword::COMMA)) continue;
                    else if (test_keyword_inc(Keyword::RANGLE)) break;
                    else {
                        throw_cur_token("`,' or `>' expected");
                    }
                }
            }
            std::static_pointer_cast<ExprNode>(m_node).swap(prefix);
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
        else if (test_keyword_inc(Keyword::LANGLE)) {
            auto m_node = std::make_shared<TypeApplNode>();
            m_node->set_info(get_last_info());
            m_node->lhs = prefix;

            while (1) {
                m_node->args.push_back(parse_type());
                if (test_keyword_inc(Keyword::COMMA)) continue;
                else if (test_keyword_inc(Keyword::RANGLE)) break;
                else {
                    throw_cur_token("`,' or `>' expected");
                }
            }
            std::static_pointer_cast<ExprNode>(m_node).swap(prefix);
        }
        else if (test_keyword_inc(Keyword::DOT)) {
            auto m_node = std::make_shared<GetFieldNode>();
            m_node->set_info(get_last_info());
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

Ptr<LambdaNode> mini::Parser::parse_lambda_wb(bool is_constructor) {
    auto m_node = std::make_shared<LambdaNode>();
    m_node->set_info(get_last_info());

    if (test_keyword_inc(Keyword::LANGLE)) {
        parse_quantifier_def_wb(m_node->quantifiers);
    }

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
    if (test_keyword(Keyword::EXTENDS)) {   // will leave "extends" to parse_expr
        auto m_extend_supernode = parse_expr();
        m_node->statements.push_back(m_extend_supernode);
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

    if (!is_constructor && test_keyword_inc(Keyword::COLON)) { // greedy match
        m_node->ret_type = parse_type();
    }

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

    if (test_keyword_inc(Keyword::LANGLE)) {
        parse_quantifier_def_wb(m_funnode->quantifiers);
        m_typenode->quantifiers = m_funnode->quantifiers;
    }

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

    if (test_keyword_inc(Keyword::ARROW)) {
        m_funnode->ret_type = parse_type();
        m_typenode->args.push_back(m_funnode->ret_type);
    }
    else {
        m_typenode->args.push_back(NULL);   // to be inferred
    }
    m_node->vtype = m_typenode;

    match_keyword_inc(Keyword::LCURLY);
    parse_statement_list(m_funnode->statements);
    match_keyword_inc(Keyword::RCURLY);
    m_node->expr = m_funnode;

    return m_node;
}

void mini::Parser::parse_quantifier_def_wb(std::vector<std::pair<pSymbol, Ptr<TypeNode>>>& args) {
    while (1) {
        auto arg_name = get_id_inc();
        Ptr<TypeNode> arg_type;
        if (test_keyword_inc(Keyword::IMPLEMENTS)) {
            arg_type = parse_type();
        }
        else {
            arg_type = NULL;
        }
        args.push_back({ arg_name, arg_type });

        if (test_keyword_inc(Keyword::RANGLE)) break;
        else if (test_keyword_inc(Keyword::COMMA)) continue;
        else {
            throw_cur_token("`,' or `>' expected");
        }
    }
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

    auto m_node = std::make_shared<ClassNode>();
    m_node->set_info(get_last_info());
    m_node->symbol = get_id_inc();

    while (1) {
        if (test_keyword_inc(EXTENDS)) {
            if (m_node->base) get_last_info().throw_exception("Base class is already defined");
            m_node->base = parse_type();
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
            if (test_keyword_inc(Keyword::NEW)) { // constructor
                if (m_node->constructor) m_node->constructor->get_info().throw_exception("Constructor redefinition");
                else m_node->constructor = parse_lambda_wb(true);
            }
            else {
                ClassNode::ClassMemberMeta m_meta;
                if (test_keyword_inc(Keyword::VIRTUAL)) m_meta.is_virtual = true;
                auto m_member = parse_let_field_wb();
                m_node->members.push_back({ m_member, m_meta });
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
    auto m_node = std::make_shared<InterfaceNode>();
    m_node->set_info(get_last_info());
    m_node->symbol = get_id_inc();
    if (test_keyword_inc(Keyword::EXTENDS)) {
        do {
            m_node->parents.push_back(get_id_inc());
        } while (test_keyword_inc(Keyword::COMMA));
    }

    match_keyword_inc(Keyword::LCURLY);
    if (!test_keyword(Keyword::RCURLY)) {
        while (1) {
            m_node->add_member(parse_let_field_wb());

            if (test_keyword_inc(Keyword::COMMA)) continue;
            else if (test_keyword_inc(Keyword::RCURLY)) break;
            else {
                throw_cur_token("`,' or `}' expected");
            }
        }
    }

    return m_node;
}

Ptr<LetNode> mini::Parser::parse_let_field_wb()
{
    auto m_member = std::make_shared<LetNode>();
    m_member->symbol = get_id_inc();
    m_member->set_info(get_last_info());
    match_keyword_inc(Keyword::COLON);
    m_member->vtype = parse_type();
    return m_member;
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
