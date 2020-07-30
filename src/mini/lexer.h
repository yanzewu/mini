#ifndef MINI_LEXER_H
#define MINI_LEXER_H

#include "token.h"
#include "errors.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace mini {

    class Lexer {
    private:

        static const std::unordered_map<char, Keyword> operator_map;
        static const std::unordered_map<std::string, Keyword> keyword_map;

        static const char comment_begin = '#';
        static const unsigned begin_noword = -1;

        enum class State {
            EMPTY,
            HOLD_ID,
            HOLD_DOT,
            HOLD_DASH,
        };
        State state;

        unsigned pos;           // position of current char
        unsigned lineno;        // index of line
        unsigned linebegin;     // position of current line beginning
        unsigned begin;         // position of current token beginning
        unsigned srcno;         // index of source file

    public:

        // LALR
        void tokenize(const std::string& str, std::vector<Token>& token_buffer, unsigned file_no = 0);

    private:

        // pos++, begin does not change (but if begin == none, set begin = pos)
        void shift();

        // pos++
        void skip();

        // do the normal reduction for [begin,pos). pos unchanged; begin->none; state->empty
        void reduce_normal(const std::string& str, std::vector<Token>& token_buffer);

        // if matched, set pos->pos+1, begin->none; state->empty; otherwise remain unchanged. Returns true if matched.
        bool reduce_and_match_operator(const std::string& str, std::vector<Token>& token_buffer);

        // begin, pos must be placed on two quotes. They won't change.
        void reduce_str_or_char(const std::string& s, std::vector<Token>& token_buffer);

        // match number greedy. begin must be placed at the first character to match.
        // pos->next char of number; begin->none; state->empty. Throws if not matched.
        void match_number(const std::string& str, std::vector<Token>& token_buffer);

        // begin must be placed at the quote. pos->next char of quote; begin->none; state->empty
        void match_string(const std::string& str, std::vector<Token>& token_buffer);

        // Build the SymbolInfo instance for current position
        SymbolInfo cur_info()const;

    };

}

#endif

