#ifndef MINI_TYPE_H
#define MINI_TYPE_H

#include "symbol.h"
#include "defines.h"

#include <vector>
#include <unordered_map>

namespace mini {
    
    class Type;

    // instance of a specific type
    class TypeMetaData {
    public:
        enum Type_t {
            OBJECT = 0,
            NIL = 1,
            BOOL = 2,
            INT = 3,
            CHAR = 4,
            FLOAT = 5,
            LIST = 10,
            FUNCTION = 11,
            TUPLE = 12,
            ARRAY = 13,
            STRUCT = 14,
            CUSTOM = 15,
            BOTTOM = 20,
        };

    public:

        pSymbol symbol;
        Index_t index = 0;      // Defined index of custom class
        unsigned min_arg = 0;
        unsigned max_arg = 0;
        
        // (RANK-2) constraints on args

        // (RANK-2) type-mapping functions on parent
        std::shared_ptr<Type> parent;

        TypeMetaData(const pSymbol& symbol_, Type_t tp) : symbol(symbol_), _type(tp) {}

        TypeMetaData::Type_t get_type()const {
            return _type;
        }

        OutputStream& print(OutputStream& os)const {
            os << "[" << *symbol << "]";
            if (_type == TypeMetaData::CUSTOM) {
                os << " " << index;
            }
            return os;
        }

    protected:
        TypeMetaData::Type_t _type;

    };
    typedef std::shared_ptr<TypeMetaData> pTypedef;
    typedef TypeMetaData* TypedefRef;
    typedef const TypeMetaData* ConstTypedefRef;

    inline OutputStream& operator<<(OutputStream& os, const TypeMetaData& a) {
        return a.print(os);
    }

    // Type instance
    class Type {
    public:

        // Notice this is different from TypeMetaData (or Typedef)
        // the latter is a symbol and can be referred from the symbol table,
        // and should be managed centrally (use reference/pointer);
        // Type is only an instance and is designed to be arbitarily copyable.

        // The partial order of types
        enum class Ordering {
            UNCOMPARABLE = 0,
            GREATER = 1,
            LESS = 2,
            EQUAL = 3,
        };

        ConstTypedefRef ref;
        std::vector<std::shared_ptr<Type>> args;
        
        Type() : ref(NULL) {}
        explicit Type(const TypeMetaData* ref_) : ref(ref_) {}

        Type(ConstTypedefRef ref_, const std::vector<std::shared_ptr<Type>>& args_) : ref(ref_) {
            // (RANK-2) check if args satisfy constraints, if any
            args = args_;
        }

        bool is_object()const { return ref->get_type() == TypeMetaData::OBJECT;}
        bool is_nil()const { return ref->get_type() == TypeMetaData::NIL;}
        bool is_bottom()const {return ref->get_type() == TypeMetaData::BOTTOM;}
        bool is_atomic()const {return ref->get_type() >= TypeMetaData::NIL && ref->get_type() <= TypeMetaData::FLOAT;}
        bool is_list()const {return ref->get_type() == TypeMetaData::LIST;}
        bool is_array()const {return ref->get_type() == TypeMetaData::ARRAY;}
        bool is_tuple()const {return ref->get_type() == TypeMetaData::TUPLE;}
        bool is_function()const {return ref->get_type() == TypeMetaData::FUNCTION;}
        bool is_struct()const {return ref->get_type() == TypeMetaData::STRUCT;}
        bool is_custom_class()const {return ref->get_type() == TypeMetaData::CUSTOM;}
        
        // return a new instance with arguments filled. Raise error if constraints are not safisfied.
        Type evaluates_to(const std::vector<std::shared_ptr<Type>>& args_)const {
            // ! This needs to be implemented to support Rank-2 types

            return Type();
        }
        
        // does NOT work for structs
        bool equals(const Type& rhs)const {
            return equals(*this, rhs);
        }

        // does NOT work for structs
        bool equals(const Type* rhs)const {
            return equals(*this, *rhs);
        }

        // only checks lhs >= rhs
        Ordering partial_compare(const Type& rhs)const {
            return partial_compare(*this, rhs);
        }

        // only checks lhs >= rhs
        Ordering partial_compare(const Type* rhs)const {
            return partial_compare(*this, *rhs);
        }

        Ordering compare(const Type& rhs)const {
            return compare(*this, rhs);
        }

        Ordering compare(const Type* rhs)const {
            return compare(*this, *rhs);
        }

        OutputStream& print(OutputStream&)const;

        // does NOT work for structs, use field_compatible/partial_compare instead.
        static bool equals(const Type& lhs, const Type& rhs);

        // Returns EQUAL/GREATOR/LESS/UNCOMPARABLE
        static Ordering compare(const Type& lhs, const Type& rhs);

        // Check if lhs >= rhs. Returns EQUAL/GREATOR/UNCOMPARABLE
        static Ordering partial_compare(const Type& lhs, const Type& rhs);

        // Check if lhs inherits rhs. return EQUAL/GREATOR/UNCOMPARABLE
        // does NOT work for intrinsic types -- use partial_compare instead.
        static Ordering inherits(const Type& lhs, const Type& rhs);
 
    protected:

        static Ordering _reverse(Ordering ordering_);
    };

    typedef std::shared_ptr<Type> pType;
    typedef Type* TypeRef;
    typedef const Type* ConstTypeRef;

    inline OutputStream& operator<<(OutputStream& os, const Type& a) {
        return a.print(os);
    }

    class StructType : public Type {
    public:

        std::unordered_map<std::string, pType> fields;

        StructType() : Type() {}
        StructType(ConstTypedefRef ref) : Type(ref) {}
        StructType(ConstTypedefRef ref, const std::unordered_map<std::string, pType>& fields) :
            Type(ref), fields(fields) {}
        StructType(ConstTypedefRef ref, const std::unordered_map<std::string, pType>& fields, const std::vector<pType>& args) :
            Type(ref, args), fields(fields) {}

        // lhs >= rhs
        Ordering field_compatible(const StructType& rhs)const {
            return field_compatible(*this, rhs);
        }

        // Check if all lhs's field exists in rhs's field. Return EQUAL/GREATOR/UNCOMPARABLE
        static Ordering field_compatible(const StructType& lhs, const StructType& rhs);
    };

    class ClassType : public StructType {
    public:



    };


}

#endif // !TYPE_H
