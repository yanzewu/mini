#ifndef MINI_TYPEMETADATA_H
#define MINI_TYPEMETADATA_H

#include "symbol.h"
#include "defines.h"

namespace mini {

    // Metadata of a Type
    class TypeMetaData {
    public:
        enum Type_t {
            PRIMITIVE = 0,
            OBJECT = 1,
            INTERFACE = 2,
            VARIABLE = 3,
        };

        pSymbol symbol;

        TypeMetaData() {}
        TypeMetaData(const pSymbol& symbol, Type_t tp) : symbol(symbol), _type(tp) {}

        // is a predefined type (int, char...)
        bool is_primitive()const { return _type == Type_t::PRIMITIVE;}
        bool is_object()const { return _type == Type_t::OBJECT; }
        bool is_interface()const { return _type == Type_t::INTERFACE; }
        bool is_variable()const { return _type == Type_t::VARIABLE; }

        virtual OutputStream& print(OutputStream& os)const { return os; }
        
        template<typename T>
        const T* as()const {
            static_assert(std::is_base_of<TypeMetaData, T>::value, "Proper type required in casting");
            return static_cast<const T*>(this);
        }
        template<typename T>
        T* as() {
            static_assert(std::is_base_of<TypeMetaData, T>::value, "Proper type required in casting");
            return static_cast<T*>(this);
        }

    protected:
        Type_t _type;
    };

    typedef std::shared_ptr<TypeMetaData> pTypedef;
    typedef TypeMetaData* TypedefRef;
    typedef const TypeMetaData* ConstTypedefRef;

    inline OutputStream& operator<<(OutputStream& os, const TypeMetaData& a) {
        return a.print(os);
    }

    class Type;

    class PrimitiveTypeMetaData : public TypeMetaData {
    public:
        enum Primitive_Type_t {
            TOP = 0,
            NIL = 1,
            BOOL = 2,
            INT = 3,
            CHAR = 4,
            FLOAT = 5,
            LIST = 10,
            FUNCTION = 11,
            TUPLE = 12,
            ARRAY = 13,
            OBJECT = 14,
            STRUCT = 15,
            VARIANT = 16,
            BOTTOM = 20,
        } primitive_type;

        PrimitiveTypeMetaData(const pSymbol& symbol, Primitive_Type_t primitive_type) :
            TypeMetaData(symbol, Type_t::PRIMITIVE), primitive_type(primitive_type) {}

        Primitive_Type_t get_type()const {
            return primitive_type;
        }
        
        size_t min_arg()const {
            switch (primitive_type) {
            case FUNCTION: case TUPLE: case ARRAY: case VARIANT: return 1;
            default: return 0;
            }
        }

        size_t max_arg()const {
            switch (primitive_type){
            case FUNCTION: case TUPLE: case VARIANT: return INT_MAX;
            case ARRAY: return 1;
            default: return 0;
            }
        }

        bool is_nil()const { return primitive_type == NIL; }
        bool is_function()const { return primitive_type == FUNCTION; }
        // Does not accept arguments
        bool is_singlet()const { return primitive_type <= LIST || primitive_type == OBJECT || primitive_type == BOTTOM; }
        // Inherits list
        bool is_list_like()const { return primitive_type >= LIST && primitive_type <= OBJECT; }

    };

    // Describes an object
    class ObjectTypeMetaData: public TypeMetaData {
    public:
        Index_t index = 0;      // Defined index of custom class
        unsigned args_count = 0;

        // (F-omega) constraints on args

        // (F-omega) type-mapping functions on parent
        // std::shared_ptr<Type> parent;

        ObjectTypeMetaData(const pSymbol& psymbol, Type_t tp) :
            TypeMetaData(psymbol, tp) {}

    };

    // Describes an interface
    class InterfaceTypeMetaData : public TypeMetaData {
    public:

        std::shared_ptr<Type> actual_type;

        InterfaceTypeMetaData(const pSymbol& symbol) : TypeMetaData(symbol, TypeMetaData::INTERFACE) {}
        InterfaceTypeMetaData(const pSymbol& symbol, const std::shared_ptr<Type>& actual_type) : 
            TypeMetaData(symbol, TypeMetaData::INTERFACE), actual_type(actual_type){}

    };

    // Describes a type variable
    class TypeVariableMetaData : public TypeMetaData {
    public:
        unsigned arg_id;
        std::shared_ptr<Type> quantifier;

        TypeVariableMetaData(const pSymbol& symbol, unsigned arg_id, const std::shared_ptr<Type>& quantifier) :
            TypeMetaData(symbol, TypeMetaData::VARIABLE), arg_id(arg_id), quantifier(quantifier) {}
    };

}

#endif