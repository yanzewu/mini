#ifndef MINI_TYPE_H
#define MINI_TYPE_H

#include "symbol.h"
#include "defines.h"
#include "typemetadata.h"

#include <vector>
#include <unordered_map>

namespace mini {

    // Type instance
    class Type {
    public:

        // The partial order of types
        enum class Ordering {
            UNCOMPARABLE = 0,
            GREATER = 1,
            LESS = 2,
            EQUAL = 3,
        };

        enum class Type_t {
            PRIMITIVE = 0,
            STRUCT = 1,
            OBJECT = 2,
            VARIABLE = 3,
            UNIVERSAL = 4,
            RECURSIVE = 5,
        } _type;    // this has nothing to do with any typing...

        virtual unsigned kind()const {
            return 0;
        }

        bool is_universal()const { return _type == Type_t::UNIVERSAL; }
        bool is_primitive()const { return _type == Type_t::PRIMITIVE; }
        bool is_struct()const { return _type == Type_t::STRUCT; }
        bool is_object()const { return _type == Type_t::OBJECT; }
        bool is_universal_variable()const { return _type == Type_t::VARIABLE; }
        bool is_recursive()const { return _type == Type_t::RECURSIVE; }
        // is a non universal type with kind *
        bool is_concrete()const { 
            return _type == Type_t::PRIMITIVE || _type == Type_t::STRUCT || _type == Type_t::OBJECT; }
        bool is_kind0()const { return true; }


        bool is_top()const;

        bool is_bottom()const;

        bool qualifies_interface()const;

        // A > B. if RHS has less, use rhs < lhs; otherwise use lhs > rhs
        Ordering partial_compare(const Type* rhs)const {
            if (rhs->has_less()) return _reverse(rhs->less(this));
            else return greater(rhs);
        }

        // A == B
        virtual bool equals(const Type* rhs)const { return false; }

        // A >* B
        virtual bool is_interface_of(const Type* rhs)const { return false; }

        PrimitiveTypeMetaData::Primitive_Type_t erased_primitive_type()const;

        // Provide the reference of Addessable.
        std::shared_ptr<Type> erasure(ConstTypedefRef addressable_ref)const;

        virtual OutputStream& print(OutputStream& os)const { return os; }

        template<typename T>
        const T* as()const {
            static_assert(std::is_base_of<Type, T>::value, "Proper type required in casting");
            return static_cast<const T*>(this);
        }

        // Only for test.
        static Ordering compare(const Type* lhs, const Type* rhs);

    protected:

        Type() {}
        explicit Type(Type_t tp) : _type(tp) {}

        // A > B
        virtual Ordering greater(const Type* rhs)const { return Ordering::UNCOMPARABLE; }

        // A < B (may not implemented)
        virtual Ordering less(const Type* rhs)const { return Ordering::UNCOMPARABLE; }

        // if true then use the compare result of it 
        bool has_less()const {
            return is_bottom() || is_universal_variable();
        }

        // Check if in the same category (primitive,struct,object,variable,universal)
        bool type_matches(const Type* rhs)const { return _type == rhs->_type; }

        // returns true if equal element-wise
        static bool array_equals(const std::vector<std::shared_ptr<Type>>& lhs, const std::vector<std::shared_ptr<Type>>& rhs) {
            return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                [](const std::shared_ptr<Type>& l, const std::shared_ptr<Type>& r) {return l->equals(r.get()); });
        }

        static Ordering _reverse(Ordering ordering_) {
            return ordering_ == Ordering::LESS ? Ordering::GREATER : (
                ordering_ == Ordering::GREATER ? Ordering::LESS : ordering_);
        }
    };


    typedef std::shared_ptr<Type> pType;
    typedef Type* TypeRef;
    typedef const Type* ConstTypeRef;

    inline OutputStream& operator<<(OutputStream& os, const Type& a) {
        return a.print(os);
    }

    // Predefined types (top,bottom,nil,bool,char,int,float,list,function,tuple,array,object,variant)
    class PrimitiveType : public Type {
    public:
        using Primitive_Type_t = PrimitiveTypeMetaData::Primitive_Type_t;

        std::vector<pType> args;
        const PrimitiveTypeMetaData* ref;

        explicit PrimitiveType(const PrimitiveTypeMetaData* ref) : Type(Type_t::PRIMITIVE), ref(ref) {}
        PrimitiveType(const PrimitiveTypeMetaData* ref, const std::vector<pType>& args) :
            Type(Type_t::PRIMITIVE), ref(ref), args(args) {}

        // the name of type
        Primitive_Type_t type_name()const {
            return ref->as<PrimitiveTypeMetaData>()->primitive_type;
        }

        bool equals(const Type* rhs)const final {
            return type_matches(rhs) && 
                type_name() == rhs->as<PrimitiveType>()->type_name() &&
                array_equals(this->args, rhs->as<PrimitiveType>()->args);
        }

        bool is_interface_of(const Type* rhs)const final {
            return type_name() == Primitive_Type_t::ADDRESSABLE && greater(rhs) == Ordering::GREATER;
        }

        Ordering greater(const Type* rhs)const final;

        // for BOTTOM only
        Ordering less(const Type* rhs)const final;

        OutputStream& print(OutputStream& os)const final;

        bool is_nil()const { return pref()->is_nil(); }
        bool is_function()const { return pref()->is_function(); }
        bool is_unboxed()const { // char,int,float,array(char)
            return type_name() == Primitive_Type_t::CHAR || type_name() == Primitive_Type_t::INT || type_name() == Primitive_Type_t::FLOAT ||
                type_name() == Primitive_Type_t::NIL || 
                (type_name() == Primitive_Type_t::ARRAY && 
                    args.size() == 1 && args[0]->is_primitive() && args[0]->as<PrimitiveType>()->type_name() == Primitive_Type_t::CHAR);
        }
        bool is_singlet()const {return pref()->is_singlet(); }
        bool is_list_like()const { return pref()->is_list_like();}

    protected:

        const PrimitiveTypeMetaData* pref()const {
            return ref->as<PrimitiveTypeMetaData>();
        }
    };

    class StructType : public Type {
    public:
        
        struct Identifier {
            std::shared_ptr<StructType> val;
            bool operator==(const Identifier& rhs)const {
                return val->equals(rhs.val.get());
            }
            Identifier() {}
            Identifier(const std::shared_ptr<StructType>& val) : val(val) {}
        };

        struct Hasher {
        public:
            size_t operator()(const StructType::Identifier& rhs)const {
                return _hash(rhs.val.get());
            }
            size_t _hash(const Type*)const;
        };

        std::unordered_map<std::string, pType> fields;

        StructType() : Type(Type_t::STRUCT) {}
        explicit StructType(const std::unordered_map<std::string, pType>& fields) : 
            Type(Type_t::STRUCT), fields(fields) {}

        bool equals(const Type* rhs)const override {
            return this->_type == rhs->_type && field_equal(fields, rhs->as<StructType>()->fields);
        }

        bool is_interface_of(const Type* rhs)const override;

        Ordering greater(const Type* rhs)const override {
            if (type_matches(rhs) && fields.size() == rhs->as<StructType>()->fields.size()) {
                // we require the fields have same size so that field_compatible == field_match
                return field_compatible(fields, rhs->as<StructType>()->fields);
            }
            else return Ordering::UNCOMPARABLE;
        }

        // Check if all lhs's field exists in rhs's field. Return EQUAL/GREATOR/UNCOMPARABLE
        static Ordering field_compatible(const std::unordered_map<std::string, pType>&, const std::unordered_map<std::string, pType>&);

        // Check if they are exactly equal
        static bool field_equal(const std::unordered_map<std::string, pType>&, const std::unordered_map<std::string, pType>&);

        OutputStream& print(OutputStream& os)const final;

    };


    class ObjectType : public Type {
    public:

        const ObjectTypeMetaData* ref;
        
        explicit ObjectType(const ObjectTypeMetaData* ref) : Type(Type_t::OBJECT), ref(ref) {}

        bool equals(const Type* rhs)const final {
            return type_matches(rhs) && _index_equal(rhs->as<ObjectType>()->ref);
            // TODO F-omega may need to compare args
        }

        Ordering greater(const Type* rhs)const final {
            if (!type_matches(rhs)) return Ordering::UNCOMPARABLE;
            if (equals(rhs)) return Ordering::EQUAL;
            if (_index_greater(rhs->as<ObjectType>()->ref)) return Ordering::GREATER;
            return Ordering::UNCOMPARABLE;
        }

        OutputStream& print(OutputStream& os)const final;

        pType member_unfold()const {
            return std::make_shared<StructType>(ref->fields);
        }

    protected:
        bool _index_equal(const ObjectTypeMetaData* rhs)const {
            return ref->index == rhs->index;
        }
        bool _index_greater(const ObjectTypeMetaData* rhs)const {
            if (!rhs) return false;
            else if (ref->index == rhs->index) {
                return true;
            }
            else {
                if (!rhs->base->is_object()) return false;
                return _index_greater(rhs->base->as<ObjectType>()->ref);
            }
        }
    };

    class UniversalTypeVariable : public Type {
    public:
        /* The type variable is described by the "nameless representation": (stack_id, arg_id)
        When the argument stack is properly loaded this should uniquely map to a variable.
        */

        unsigned stack_id;      // distance to declaration. For example, <X>.A(<Y>.B(X,Y)) X.stack_id=1,Y.stack_id=0.
        unsigned arg_id;        // declaration order in arguments
        ConstTypeRef quantifier;

        UniversalTypeVariable(unsigned stack_id, unsigned arg_id, ConstTypeRef quantifier) : Type(Type_t::VARIABLE), stack_id(stack_id), arg_id(arg_id), quantifier(quantifier) {}

        // equal: other must also be a type variable. The quantifier ref 
        // is not checked here, since it is already checked in the universal part.
        bool equals(const Type* rhs)const override {
            if (!type_matches(rhs)) return false;

            auto rrhs = rhs->as<UniversalTypeVariable>();
            return rrhs->stack_id == stack_id && rrhs->arg_id == arg_id;
        }

        Ordering less(const Type* rhs)const final {
            /* This is the most conservative way. Alternatively, we can do
            LHS is less than RHS <=> RHS appears in the inheriting chain of LHS for all nonrecursive cases.
            */

            if (equals(rhs)) return Ordering::EQUAL;
            else if (rhs->is_top() || (rhs->is_primitive() && rhs->as<PrimitiveType>()->type_name() == PrimitiveTypeMetaData::ADDRESSABLE)) {   
                                        // In this version the top quantifier is Addressable, which < top
                return Ordering::LESS;
            }
            else {
                return Ordering::UNCOMPARABLE;
            }
        }

        OutputStream& print(OutputStream& os)const final;

    };

    class UniversalType : public Type {
    public:

        std::vector<pType> quantifiers; // cannot be empty.
        pType body;

        UniversalType() : Type(Type_t::UNIVERSAL) {}
        UniversalType(const std::vector<pType>& quantifiers, const pType& body) : 
            Type(Type_t::UNIVERSAL), quantifiers(quantifiers), body(body) {}

        bool equals(const Type* rhs)const final {
            if (!type_matches(rhs)) return false;

            auto rrhs = rhs->as<UniversalType>();
            return array_equals(quantifiers, rrhs->quantifiers) && body->equals(rrhs->body.get());
        }

        Ordering greater(const Type* rhs)const final {
            if (!type_matches(rhs)) return Ordering::UNCOMPARABLE;

            auto rrhs = rhs->as<UniversalType>();
            return array_equals(quantifiers, rrhs->quantifiers) ? body->partial_compare(rrhs->body.get()) : Ordering::UNCOMPARABLE;
            // this is kernel F<, since our interface has no transistivity.
        }

        OutputStream& print(OutputStream& os)const final;

        pType instanitiate(const std::vector<pType>& args, const SymbolInfo& info)const {
            if (args.size() != quantifiers.size()) {
                info.throw_exception(StringAssembler("Too few arguments for type ")(*this)(": expect ")(quantifiers.size())(", got ")(args.size())());
            }
            auto a = args.begin();
            auto q = quantifiers.begin();
            for (; q != quantifiers.end(); q++, a++) {
                if (!(*q)->is_interface_of(a->get())) {
                    info.throw_exception(StringAssembler("Type ")(**q)(" is not an interface of ")(**a)());
                }
            }
            return evaluate(body, args, 0, info);
        }

    private:

        static pType evaluate(const pType& target, const std::vector<pType>& args, unsigned stack_base, const SymbolInfo& info);
    };

    class RecursiveTypeVariable : public Type {
        unsigned stack_id;    /* Recursive stack ID */
        unsigned arg_id;
    };

    class RecursiveType : public Type {
    public:
        size_t nargs;
        pType body;

        RecursiveType() : Type(Type_t::RECURSIVE) {}

        bool equals(const Type* rhs)const {
            if (rhs->is_recursive()){
                return nargs == rhs->as<RecursiveType>()->nargs && body->equals(rhs->as<RecursiveType>()->body.get());
            }
            return false;
        }

        Ordering greator(const Type* rhs);

        pType unfold()const {
            return nullptr;
        }

        OutputStream& print(OutputStream& os)const final;
    };

}

#endif // !TYPE_H
