#ifndef MINI_PARSER_H
#define MINI_PARSER_H

#include "token.h"
#include "ast.h"

#include <vector>


namespace mini {

    class Parser {

    private:

        typedef std::vector<Token>::const_iterator TokenIter_t;

        TokenIter_t cur_token;
        TokenIter_t token_bound;

    public:
        
        void parse(const std::vector<Token>& token_buffer, std::vector<Ptr<AST>>& ast_buffer, ErrorManager* error_manager = nullptr);

    private:

        Ptr<AST> parse_block(TokenIter_t token_begin, TokenIter_t token_end);

        // 'wb' stands for without boundary: the leftest word is assume parsed.

        Ptr<LetNode> parse_let_wb(bool allow_expr=true);

        Ptr<LetNode> parse_let_field_wb();

        Ptr<SetNode> parse_set_wb();

        Ptr<TypeNode> parse_type();

        Ptr<ExprNode> parse_expr();

        Ptr<StructNode> parse_struct_wb();

        Ptr<ArrayNode> parse_array_wb();

        Ptr<LambdaNode> parse_lambda_wb(bool is_constructor=false);

        Ptr<CaseNode> parse_case_wb();

        Ptr<LetNode> parse_func_def_wb();

        void parse_quantifier_def_wb(std::vector <std::pair<pSymbol, Ptr<TypeNode>>>& args);

        void parse_statement_list(std::vector<Ptr<AST>>& statements);

        Ptr<ClassNode> parse_class_wb();

        Ptr<InterfaceNode> parse_interface_wb();

        Ptr<ImportNode> parse_import_wb();

        // if Token does not match, raise error
        void match_token(Token::Type_t tp) {
            match_no_end();
            if (cur_token->get_type() != tp) {
                throw_cur_token("Expect " + to_str(tp) + ", got " + to_str(cur_token->get_type()));
            }
        }

        // return if Token with certain type matches
        bool test_token(Token::Type_t tp) {
            return cur_token != token_bound && cur_token->get_type() == tp;
        }

        // return if keyword matches
        bool test_keyword(Keyword keyword) {
            return test_token(Token::KEYWORD) && std::get<Keyword>(cur_token->value) == keyword;
        }

        // if Token matches, increase current position and return ture; otherwise return false
        bool test_token_inc(Token::Type_t tp) {
            if (test_token(tp)) {
                cur_token++;
                return true;
            }
            else {
                return false;
            }
        }

        // if Keyword matches, increase current position and return true; otherwise return false
        bool test_keyword_inc(Keyword keyword) {
            if (test_keyword(keyword)) {
                cur_token++;
                return true;
            }
            else {
                return false;
            }
        }

        // if Token matches, increase current position; otherwise raise error
        void match_inc(Token::Type_t tp) {
            match_token(tp);
            cur_token++;
        }

        // if Symbol matches, increase current position; otherwise raise error
        void match_keyword_inc(Keyword keyword) {
            match_token(Token::KEYWORD);
            if (std::get<Keyword>(cur_token->value) != keyword) {
                throw_cur_token("Expect " + to_str(keyword));
            }
            cur_token++;
        }

        // throw an error if the block end is met
        void match_no_end() {
            if (cur_token == token_bound) {
                (cur_token-1)->get_info().throw_exception("Incomplete expression");
            }
        }

        // get the data at current token and increase position by one. Will not check boundary.
        template<typename T>
        const T& get_token_data_inc() {
            return std::get<T>((cur_token++)->value);
        }

        // get the ID data at current token and increase position by one. Will not check boundary.
        pSymbol get_id_inc() {
            pSymbol ret = std::make_shared<Symbol>(std::get<StringRef>(cur_token->value), cur_token->get_info());
            cur_token++;
            return ret;
        }

        // get the TokenInfo at the token just passed. Will not check boundary.
        SymbolInfo get_last_info()const {
            return (cur_token - 1)->get_info();
        }

        // throw an error with current token
        void throw_cur_token(const std::string& msg) {
            match_no_end();
            cur_token->get_info().throw_exception(msg);
        }


    };

}

#endif