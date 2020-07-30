
#include <array>

#include "../mini/mini.h"
#include "ut.h"


struct LexerTestCase {
    std::string input;
    std::vector<mini::Token::Value_t> answer;
};


#define MC(x) mini::Constant(x)
#define MCS(x) mini::Constant(std::string(x))

std::array<LexerTestCase, 5> lexer_test_cases = {
    LexerTestCase{"a b   c\td\ne f#Invisible##\n\ng", {"a", "b", "c", "d", "e", "f", "g"}},
    LexerTestCase{".12 12.34 12. .1e+10 -.4 -.1e-2 -1e 123 -34 e23 e2-3 e2-b e2-.4 e3.4 1e3.4 .1e.b .e3.4 1..2 1...2",
    {MC(0.12f), MC(12.34f), MC(12.0f), MC(0.1e+10f), MC(-0.4f), MC(-0.1e-2f), MC(-1), "e", MC(123), MC(-34),
    "e23", "e2", MC(-3), "e2", mini::Keyword::DASH, "b", "e2", MC(-0.4f), "e3", MC(0.4f), MC(1e3f), MC(0.4f),
    MC(0.1f), "e", mini::Keyword::DOT, "b", mini::Keyword::DOT, "e3", MC(0.4f), MC(1.0f), MC(0.2f), MC(1.0f),
    mini::Keyword::DOT, MC(0.2f)
    }},
    // the string is '1' '\"' '"' ' ' "" "\'\"\\""\'" abc"123"+45
    LexerTestCase{"'1' '\\\"' '\"' ' ' \"\" \"\\t\" \"\\'\\\"\\\\\"\"\\'\" abc\"123\"+45", {
        MC('1'), MC('"'), MC('"'), MC(' '), MCS(""), MCS("\t"), MCS("\'\"\\"), MCS("'"), "abc", MCS("123"), MC(45)
    }},
    LexerTestCase{"50.a a.b.c.0 (abc).de (abc).23 (abc).-23 (abc)-.23 (12.34).cd (abc).(def)", {
        MC(50.0f), "a", "a", mini::Keyword::DOT, "b", mini::Keyword::DOT, "c", MC(0.0f), mini::Keyword::LBRACKET, "abc",
        mini::Keyword::RBRACKET, mini::Keyword::DOT, "de", mini::Keyword::LBRACKET, "abc", mini::Keyword::RBRACKET,
        MC(0.23f), mini::Keyword::LBRACKET, "abc", mini::Keyword::RBRACKET, mini::Keyword::DOT, MC(-23),
        mini::Keyword::LBRACKET, "abc", mini::Keyword::RBRACKET, MC(-0.23f), mini::Keyword::LBRACKET, MC(12.34f),
        mini::Keyword::RBRACKET, mini::Keyword::DOT, "cd", mini::Keyword::LBRACKET, "abc", mini::Keyword::RBRACKET,
        mini::Keyword::DOT, mini::Keyword::LBRACKET, mini::Keyword::DEF, mini::Keyword::RBRACKET,
    }},
    LexerTestCase{"a->b -> -->> ab-23 --5 -+5", {"a", mini::Keyword::ARROW, "b", mini::Keyword::ARROW, mini::Keyword::DASH,
    mini::Keyword::ARROW, mini::Keyword::RANGLE, "ab", MC(-23), mini::Keyword::DASH, MC(-5), mini::Keyword::DASH, MC(5)}}
};

#undef MC
#undef MCS

void test_lexer() {

    mini::Lexer lexer;
    std::vector<mini::Token> result;

    for (const auto& t : lexer_test_cases) {
        lexer.tokenize(t.input, result);
        for (size_t i = 0; i < std::min(result.size(), t.answer.size()); i++) {
            REQUIRE(result[i].value == t.answer[i], std::to_string(i).c_str());
        }
        REQUIRE(result.size() == t.answer.size(), (std::to_string(result.size()) + "!=" + std::to_string(t.answer.size())).c_str());
        result.clear();
    }

    summary();
}