#ifndef MINI_SYMBOL_H
#define MINI_SYMBOL_H

#include <string>
#include <memory>

#include "constant.h"
#include "errors.h"
#include "stream.h"

namespace mini {

    // Not sure about the storation of real string;
    // TODO: check the use of symbol/string references
    typedef std::string StringRef;

    struct Location {
        unsigned lineno;
        unsigned colno;
        unsigned srcno;

        Location() : lineno(0), colno(0), srcno(-1) {}
        Location(unsigned lineno_, unsigned colno_, unsigned srcno_) :
            lineno(lineno_), colno(colno_), srcno(srcno_) {}
    };

    class SymbolInfo {
    public:

        Location location;
        unsigned section_index;

        SymbolInfo() : section_index(0) {}

        explicit SymbolInfo(const Location& loc) : 
            location(loc), section_index(0) {}

        SymbolInfo(const Location& loc, unsigned section_idx) :
            location(loc), section_index(section_idx) {}

        unsigned get_section_index()const {
            return section_index;
        }

        const Location& get_location()const {
            return location;
        }
        bool is_absolute()const {
            return section_index == -1;
        }

        static SymbolInfo absolute() {
            return SymbolInfo( Location(), -1);
        }
        void throw_exception(const std::string& msg)const {
            throw ParsingError(msg, location.srcno, location.lineno, location.colno);
        }

        void print(OutputStream& os)const {
            if (is_absolute()) os << "absolute";
            else os << "[#" << location.srcno + 1 << ":" << location.lineno + 1 << ":"
                << location.colno + 1 << "]";
        }
    };
    inline OutputStream& operator<<(OutputStream& os, const SymbolInfo& a) {
        a.print(os); return os;
    }

    // The unit of a symbol.
    struct Symbol {
        StringRef name;   // may use other string or even just reference to the code.
        SymbolInfo info;

        // Usually pSymbol is preferred in use; However when StringRef is really a ref then Symbol is good too.

        Symbol() {}
        explicit Symbol(const std::string& name) : 
            name(name), info(SymbolInfo::absolute()) {}
        Symbol(const std::string& name, const SymbolInfo& info) :
            name(name), info(info) {}

        const std::string& get_name()const {
            return name;
        }
        const SymbolInfo& get_info()const {
            return info;
        }

        void print(OutputStream& os)const {
            os << name << "@" << info;
        }

        static std::shared_ptr<Symbol> create_absolute_symbol(const std::string& name) {
            return std::make_shared<Symbol>(name, SymbolInfo::absolute());
        }
    };
    typedef std::shared_ptr<Symbol> pSymbol;
    typedef Symbol* SymbolRef;
    typedef const Symbol* ConstSymbolRef;

    inline OutputStream& operator<<(OutputStream& os, const Symbol& a) {
        a.print(os); return os;
    }
}


#endif // !SYMBOL_H
