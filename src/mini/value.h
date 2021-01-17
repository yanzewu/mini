#ifndef VALUE_H
#define VALUE_H

#include "defines.h"
#include "symbol.h"
#include "type.h"

namespace mini {

    class VarMetaData {
    public:

        enum Source {
            GLOBAL = 0,
            ARG = 1,
            BINDING = 2,
            LOCAL = 3,
        };

        pSymbol symbol;     // underlying symbol reference
        Index_t scope;     // where the variable defined
        Index_t index;      // definition index in that scope:source
        Source source;      // where the variable comes from (local/global/arg/binding)
        pType prog_type;    // attributed type
        bool has_assigned = false;

        VarMetaData(const pSymbol& symbol, Index_t scope, Index_t index, Source source) :
            symbol(symbol), scope(scope), index(index), source(source), prog_type(nullptr) {

        }

        void print(OutputStream& os)const {
            os << "[" << *symbol << "] ";
            
            switch (source)
            {
            case mini::VarMetaData::LOCAL: os << "local#"; break;
            case mini::VarMetaData::GLOBAL: os << "global#"; break;
            case mini::VarMetaData::ARG: os << "arg#"; break;
            case mini::VarMetaData::BINDING: os << "binding#"; break;
            default:
                break;
            }
            os << index << " " << *prog_type;
        }
    };

    inline OutputStream& operator<<(OutputStream& os, const VarMetaData& a) {
        a.print(os); return os;
    }

    typedef std::shared_ptr<VarMetaData> pVariable;
    typedef VarMetaData* VariableRef;
    typedef const VarMetaData* ConstVariableRef;

    struct FieldMetaData {
        unsigned index;
    };

    typedef std::shared_ptr<FieldMetaData> pFieldMetaData;
    typedef FieldMetaData* FieldRef;
    typedef const FieldMetaData* ConstFieldRef;

}

#endif