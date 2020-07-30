
#ifndef MINI_TOKEN_H
#define MINI_TOKEN_H

#include "symbol.h"
#include "constant.h"

#include <string>
#include <variant>

namespace mini {

    
    // represents a symbol
    enum Keyword {
        COMMA = 0,
        COLON = 1,
        SEMICOLON = 2,
        DOUBLEQUOTE = 4,
        LBRACKET = 10,
        RBRACKET = 11,
        LANGLE = 12,
        RANGLE = 13,
        LCURLY = 14,
        RCURLY = 15,
        LSQBRACKET = 16,
        RSQBRACKET = 17,
        EQ = 20,
        ARROW = 21,
        BACKSLASH = 22,
        DOT = 23,
        DASH = 24,

        IMPORT = 100,
        LET = 101,
        DEF = 102,
        SET = 103,
        CLASS = 104,
        INTERFACE = 105,

        IMPLEMENTS = 110,
        EXTENDS = 111,
        STATIC = 113

    };

    struct Token {

        enum Type_t {
            KEYWORD = 0,
            ID = 1,
            CONSTANT = 2
        };

        typedef std::variant<Keyword, StringRef, Constant> Value_t;
        Value_t value;
        SymbolInfo info;

        Token() {}
        Token(const SymbolInfo& info) : info(info) {}
        Token(const Constant& c, const SymbolInfo& info) : value(c), info(info) {}
        Token(const Keyword& c, const SymbolInfo& info) : value(c), info(info) {}
        Token(const StringRef& s, const SymbolInfo& info) : value(s), info(info) {}

        Token::Type_t get_type()const {
            return (Token::Type_t)value.index();
        }
        SymbolInfo get_info()const {
            return info;
        }
    };


    const std::string& to_str(Token::Type_t);
    const std::string& to_str(Keyword);

}
#endif