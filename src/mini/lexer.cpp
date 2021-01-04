
#include "lexer.h"

using namespace mini;

const std::unordered_map<char, Keyword> Lexer::operator_map = {
    {',', COMMA},
    {':', COLON},
    {';', SEMICOLON},
    {'"', DOUBLEQUOTE},
    {'(', LBRACKET},
    {')', RBRACKET},
    {'<', LANGLE},
    {'>', RANGLE},
    {'{', LCURLY},
    {'}', RCURLY},
    {'[', LSQBRACKET},
    {']', RSQBRACKET},
    {'=', EQ},
    {'\\', BACKSLASH},
    {'.', DOT},
    {'-', DASH},
};

const std::unordered_map<std::string, Keyword> Lexer::keyword_map = {
    {"->", ARROW},
    {"forall", FORALL},
    {"import", IMPORT},
    {"let", LET},
    {"def", DEF},
    {"set", SET},
    {"class", CLASS},
    {"interface", INTERFACE},
    {"implements", IMPLEMENTS},
    {"extends", EXTENDS},
    {"new", NEW},
    {"static", STATIC},
    {"virtual", VIRTUAL},
};

const std::unordered_map<Token::Type_t, std::string> token_backmap = {
    {Token::ID, "id"},
    {Token::KEYWORD, "symbol"},
    {Token::CONSTANT, "constant"},
};

const std::unordered_map<Keyword, std::string> symbol_backmap = {
    {COMMA, ","},
    {COLON, ":"},
    {SEMICOLON, ";"},
    {DOUBLEQUOTE, "\""},
    {LBRACKET, "("},
    {RBRACKET, ")"},
    {LANGLE, "<"},
    {RANGLE, ">"},
    {LCURLY, "{"},
    {RCURLY, "}"},
    {LSQBRACKET, "["},
    {RSQBRACKET, "]"},
    {EQ, "="},
    {BACKSLASH, "\\"},
    {DOT, "."},
    {ARROW, "->"},
    {FORALL, "forall"},
    {IMPORT, "import"},
    {LET, "let"},
    {DEF, "def"},
    {CLASS, "class"},
    {SET, "set"},
    {IMPLEMENTS, "implements"},
    {EXTENDS, "extends"},
    {NEW, "new"},
    {STATIC, "static"},
    {VIRTUAL, "virtual"},
};

const std::string& mini::to_str(Token::Type_t t) {
    return token_backmap.at(t);
}

const std::string& mini::to_str(Keyword s) {
    return symbol_backmap.at(s);
}

std::string Constant::to_str()const {
    switch (get_type())
    {
    case Constant::Type_t::NIL: return "nil";
    case Constant::Type_t::BOOL: return std::get<bool>(data) ? "true" : "false";
    case Constant::Type_t::CHAR: return { std::get<char>(data) };
    case Constant::Type_t::INT: return std::to_string(std::get<int>(data));
    case Constant::Type_t::FLOAT: return std::to_string(std::get<float>(data));
    case Constant::Type_t::STRING: return std::get<std::string>(data);
    default:
        return "";
    }
}

void Lexer::tokenize(const std::string& str, std::vector<Token>& token_buffer, unsigned file_no) {

    state = State::EMPTY;
    lineno = 0;
    linebegin = 0;
    begin = begin_noword;
    srcno = file_no;

    for (pos = 0; pos < str.size();) {

        // whites

        if (str[pos] == ' ' || str[pos] == '\t') {
            reduce_normal(str, token_buffer);
            skip();
        }
        else if (str[pos] == '\n') {
            reduce_normal(str, token_buffer);
            skip();
            lineno++;
            linebegin = pos;
        }
        else if (str[pos] == comment_begin) {
            reduce_normal(str, token_buffer);
            while (pos < str.size() && str[pos] != '\n') skip();
        }
        else if (str[pos] == '\'' || str[pos] == '\"') {    // string
            reduce_normal(str, token_buffer);
            shift();
            match_string(str, token_buffer);
        }
        else if (str[pos] == '-') {
            reduce_normal(str, token_buffer);
            shift();
            state = State::HOLD_DASH;
        }
        else if (str[pos] == '.') {
            if (state == State::HOLD_DASH) {   // special case -.
            }
            else {
                reduce_normal(str, token_buffer);
            }
            shift();
            state = State::HOLD_DOT;
        }
        else if (str[pos] == '>') {
            if (state != State::HOLD_DASH) {
                reduce_and_match_operator(str, token_buffer);
            }
            else {
                pos++;
                reduce_normal(str, token_buffer);
            }
        }
        else if (str[pos] == '+') {
            reduce_normal(str, token_buffer);
            shift();
            match_number(str, token_buffer);
        }
        else if (str[pos] >= '0' && str[pos] <= '9') {
            if (state == State::HOLD_DASH || state == State::HOLD_DOT) {
                match_number(str, token_buffer);
            }
            else if (state == State::HOLD_ID) {
                shift();
            }
            else {
                shift();
                match_number(str, token_buffer);
            }
        }
        else {
            if (reduce_and_match_operator(str, token_buffer));
            else {
                if (state == State::HOLD_DOT || state == State::HOLD_DASH) reduce_normal(str, token_buffer);
                shift();
                state = State::HOLD_ID;
            }
        }
    }
    reduce_normal(str, token_buffer);
}



// do the normal reduction for [begin,pos). pos unchanged; begin->none; state->empty
void Lexer::reduce_normal(const std::string& str, std::vector<Token>& token_buffer) {
    if (state == State::EMPTY) return;
    else if (state == State::HOLD_DASH) {
        if (begin == pos - 2 && str[begin + 1] == '>') {
            token_buffer.push_back(Token(Keyword::ARROW, cur_info()));
        }
        else {
            token_buffer.push_back(Token(Keyword::DASH, cur_info()));
        }
    }
    else if (state == State::HOLD_DOT) {
        if (begin == pos - 2 && str[begin] == '-') {
            token_buffer.push_back(Token(Keyword::DASH, cur_info()));
            begin++;
        }
        token_buffer.push_back(Token(Keyword::DOT, cur_info()));
    }
    else {
        if (begin == begin_noword) throw std::runtime_error("begin not set");

        std::string frag = str.substr(begin, pos - begin);
        Token token(cur_info());

        if (frag == "nil") {
            token.value = Constant();
        }
        else if (frag == "true" || frag == "false") {
            token.value = Constant(frag == "true" ? true : false);
        }
        else {
            auto kwd = keyword_map.find(frag);
            if (kwd == keyword_map.end()) {
                token.value = frag;
            }
            else {
                token.value = kwd->second;
            }
        }
        token_buffer.push_back(token);
    }
    begin = begin_noword;
    state = State::EMPTY;
}

// if matched, set pos->pos+1, begin->none; state->empty; otherwise remain unchanged. Returns true if matched.
bool Lexer::reduce_and_match_operator(const std::string& str, std::vector<Token>& token_buffer) {
    auto sym = operator_map.find(str[pos]);
    if (sym != operator_map.end()) {
        reduce_normal(str, token_buffer);  // reduce the potential token holded first
        begin = pos;
        token_buffer.push_back(Token(sym->second, cur_info()));
        pos++;
        begin = begin_noword;
        state = State::EMPTY;
        return true;
    }
    else {
        return false;
    }
}

// begin, pos must be placed on two quotes. They won't change.
void Lexer::reduce_str_or_char(const std::string& s, std::vector<Token>& token_buffer) {

#define MIN(a, b) (a) < (b) ? (a) : (b)

    std::string builder;
    unsigned i = begin + 1, j = s.find_first_of('\\', i);
    j = MIN(j, pos);

    for (; i < pos;) {
        builder += s.substr(i, j - i);
        i = j;
        if (s[j] == '\\') {
            switch (s[j + 1]) {
            case 'b': builder.push_back('\b'); break;
            case 'f': builder.push_back('\f'); break;
            case 'n': builder.push_back('\n'); break;
            case 'r': builder.push_back('\r'); break;
            case 't': builder.push_back('\t'); break;
            case 'v': builder.push_back('\v'); break;
            case '\\': builder.push_back('\\'); break;
            case '\'': builder.push_back('\''); break;
            case '"': builder.push_back('"'); break;
            case '?': builder.push_back('\?'); break;
            default:
                cur_info().throw_exception("Unknown escape sequence");
            }
            i = j + 2;
            j = s.find_first_of('\\', i);
            j = MIN(j, pos);
        }
    }
#undef MIN
    bool is_string = (s[begin] == '\"');

    if (!is_string && builder.size() > 1) cur_info().throw_exception("Only one character is allowed in char");
    if (!is_string && builder.size() == 0) cur_info().throw_exception("Char should contain at least one character");

    if (is_string) token_buffer.push_back(Token(Constant(builder), cur_info()));
    else token_buffer.push_back(Token(Constant(builder[0]), cur_info()));
}


void Lexer::match_number(const std::string& str, std::vector<Token>& token_buffer) {
    char* iend, * fend;

    int ibuffer = std::strtol(&str[begin], &iend, 10);
    float fbuffer = std::strtof(&str[begin], &fend);

    // nothing matches
    if (iend == &str[begin] && fend == &str[begin]) {
        cur_info().throw_exception("Invalid number: " + str[begin]);
    }
    else if (fend > iend) {
        token_buffer.push_back(Token(Constant(fbuffer), cur_info()));
        pos = begin + (fend - &str[begin]);
    }
    else if (fend == iend) {
        token_buffer.push_back(Token(Constant(ibuffer), cur_info()));
        pos = begin + (fend - &str[begin]);
    }
    else {
        throw std::runtime_error("fend < iend");    // this should not happen...
    }
    begin = begin_noword;
    state = State::EMPTY;
}

void Lexer::match_string(const std::string& str, std::vector<Token>& token_buffer) {
    char quote = str[begin];
    pos = begin + 1;
    while (pos < str.size()) {
        if (str[pos] == '\\') {
            pos++;
            if (pos < str.size()) pos++;
        }
        else if (str[pos] == quote) {
            break;
        }
        else {
            pos++;
        }

    }
    if (pos == str.size()) {
        cur_info().throw_exception("Quote not match");
    }
    else {
        reduce_str_or_char(str, token_buffer);
        pos++;
        begin = begin_noword;
        state = State::EMPTY;
    }
}

void Lexer::shift() {
    if (begin == begin_noword) begin = pos;
    pos++;
}

void Lexer::skip() {
    pos++;
}

SymbolInfo Lexer::cur_info()const {
    return SymbolInfo({ lineno, begin - linebegin, srcno });
}