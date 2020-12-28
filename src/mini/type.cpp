#include "type.h"
#include "symtable.h"

using namespace mini;


Type::Ordering Type::compare(const Type* lhs, const Type* rhs) {
    auto ret = lhs->partial_compare(rhs);
    if (ret == Ordering::UNCOMPARABLE) {
        return _reverse(rhs->partial_compare(lhs));
    }
    else {
        return ret;
    }
}

bool Type::is_top()const {
    return _type == Type_t::PRIMITIVE && as<PrimitiveType>()->type_name() == PrimitiveTypeMetaData::TOP;
}

bool Type::is_bottom()const {
    return _type == Type_t::PRIMITIVE && as<PrimitiveType>()->type_name() == PrimitiveTypeMetaData::BOTTOM;
}

bool Type::qualifies_interface() const {
    return is_struct();
}

pType Type::erasure()const {
    switch (_type)
    {
    case Type::Type_t::PRIMITIVE: {
        //std::vector<pType> newargs;
        //for (const auto& a : as<PrimitiveType>()->args) newargs.push_back(a->erasure());
        return std::make_shared<PrimitiveType>(as<PrimitiveType>()->ref);   // NOTE we put no args since we treat them as F-omega part.
        // This does not have problem since they must all (not) be Addressable, and nothing related with type variables (since they only can be
        // Addressable too)
    }
    case Type::Type_t::STRUCT: {
        std::unordered_map<std::string, pType> newfields;
        for (const auto& f : as<StructType>()->fields) newfields.insert({ f.first, f.second->erasure() });
        return std::make_shared<StructType>(newfields);
    }
    case Type::Type_t::OBJECT: {
        // TODO object
        return std::make_shared<ObjectType>(as<ObjectType>()->ref);
    }
    case Type::Type_t::VARIABLE: {
        return std::make_shared<StructType>();  // will be an empty struct. Alternatively, the quantifier can be used, but seems unncessary here.
    }
    case Type::Type_t::UNIVERSAL: {
        return as<UniversalType>()->body->erasure();
    }
    default:
        throw std::runtime_error("Unrecognized type instance");
    }
}


PrimitiveTypeMetaData::Primitive_Type_t Type::erased_primitive_type()const {
    return is_universal() ? as<UniversalType>()->body->erased_primitive_type() :
        (is_primitive() ? as<PrimitiveType>()->type_name() : PrimitiveTypeMetaData::LIST);
}

Type::Ordering PrimitiveType::greater(const Type* rhs)const {
    
    // top: Greater than any kind 0 types.
    if (is_top()) {
        return rhs->is_kind0() ?
            (equals(rhs) ? Ordering::EQUAL : Ordering::GREATER) : 
            Ordering::UNCOMPARABLE;
    }
    // list: Greater than any nonatomic concrete types
    else if (type_name() == Primitive_Type_t::LIST) {
        if (!rhs->is_concrete()) return Ordering::UNCOMPARABLE;
        if (!rhs->is_primitive()) return Ordering::GREATER;

        auto rrhs = rhs->as<PrimitiveType>();
        if (rrhs->type_name() == Primitive_Type_t::LIST) Ordering::EQUAL;
        else return rrhs->is_list_like() ? Ordering::GREATER : Ordering::UNCOMPARABLE;
    }

    if (!type_matches(rhs)) return Ordering::UNCOMPARABLE;
    auto rrhs = rhs->as<PrimitiveType>();

    if (type_name() != rrhs->type_name()) return Ordering::UNCOMPARABLE;
    
    // function is covariant of return type and invariant of argument types.
    if (type_name() == Primitive_Type_t::FUNCTION) {
        if (args.size() != rrhs->args.size()) return Ordering::UNCOMPARABLE;
        for (size_t j = 0; j < args.size() - 1; j++) if (!args[j]->equals(rrhs->args[j].get())) return Ordering::UNCOMPARABLE;
        return args.back()->partial_compare(rrhs->args.back().get());
    }
    else {
        if (args.size() != rrhs->args.size()) return Ordering::UNCOMPARABLE;
        bool is_equal = true;
        for (size_t j = 0; j < args.size(); j++) {
            auto t = args[j]->partial_compare(rrhs->args[j].get());
            if (t == Ordering::EQUAL) continue;
            else if (t == Ordering::GREATER) is_equal = false;
            else return Ordering::UNCOMPARABLE;
        }
        return is_equal ? Ordering::EQUAL : Ordering::GREATER;
    }
}

// for BOTTOM only
Type::Ordering PrimitiveType::less(const Type* rhs)const {
    if (!is_bottom()) throw std::runtime_error("bottom required");
    if (rhs->is_universal_variable()) {
        return (rhs->as<UniversalTypeVariable>()->quantifier == NULL ||
            rhs->as<UniversalTypeVariable>()->quantifier->is_kind0()) ?
            Ordering::LESS : Ordering::UNCOMPARABLE;
    }
    return rhs->is_kind0() ? Ordering::LESS : Ordering::UNCOMPARABLE;
}

Type::Ordering StructType::field_compatible(const std::unordered_map<std::string, pType>& lhs_fields, const std::unordered_map<std::string, pType>& rhs_fields) {

    bool is_equal = true;

    for (const auto& key : lhs_fields) {
        auto rit = rhs_fields.find(key.first);
        if (rit == rhs_fields.end()) return Ordering::UNCOMPARABLE;
        auto r = key.second->partial_compare(rit->second.get());
        if (r == Ordering::GREATER) is_equal = false;
        else if (r == Ordering::EQUAL);
        else return Ordering::UNCOMPARABLE;
    }
    return (is_equal && lhs_fields.size() == rhs_fields.size()) ? Ordering::EQUAL : Ordering::GREATER;
}

bool StructType::field_equal(const std::unordered_map<std::string, pType>& lhs_fields, const std::unordered_map<std::string, pType>& rhs_fields)
{
    if (lhs_fields.size() != rhs_fields.size()) return false;
    for (const auto& key : lhs_fields) {
        auto rit = rhs_fields.find(key.first);
        if (rit == rhs_fields.end()) return false;
        if (!key.second->equals(rit->second.get())) return false;
    }
    return true;
}

// only works in 64 bit. This is CityHash
size_t hash_combine(size_t h, size_t v) {
    const std::size_t kMul = 0x9ddfea08eb382d69ULL;
    size_t a = (v ^ h) * kMul;
    a ^= (a >> 47);
    size_t b = (h ^ a) * kMul;
    b ^= (b >> 47);
    return b * kMul;
}

size_t StructType::Hasher::_hash(const Type* t)const {
    switch (t->_type)
    {
    case Type::Type_t::PRIMITIVE: {
        return size_t(t->as<PrimitiveType>()->type_name());
    }
    case Type::Type_t::OBJECT: {
        return size_t(t->as<ObjectType>()->ref->as<ObjectTypeMetaData>()->index);
    }
    case Type::Type_t::STRUCT: {
        size_t r = 1;
        for (const auto& f : t->as<StructType>()->fields) {
            r = hash_combine(r, std::hash<std::string>()(f.first));
            r = hash_combine(r, _hash(f.second.get()));
        }
        return r;
    }
    case Type::Type_t::VARIABLE: {
        return size_t(PrimitiveType::Primitive_Type_t::LIST);
    }
    case Type::Type_t::UNIVERSAL: {
        return _hash(t->as<UniversalType>()->body.get());
    }
    default:
        return 0;
    }
}

Type::Ordering ObjectType::is_base_of(const ObjectType* rhs)const {

    if (ref == rhs->ref) {
        return equals(rhs) ? Ordering::EQUAL : Ordering::UNCOMPARABLE;
    }
    else {
        /*
        auto m_rhs = rhs;
        while (m_rhs->ref->parent != NULL) {
            
            // TODO m_rhs => parent

            if (equals(m_rhs)) return Ordering::GREATER;
        }*/

        return Ordering::UNCOMPARABLE;
    }
}

pType UniversalType::evaluate(const pType& target, const std::vector<pType>& args, unsigned stack_base, const SymbolInfo& info)
{
    // This is basically induction in STLC
    // will see if a more elegant version is needed
    // Algorithm:
    // stack_base is the current stack id - the stack id of arguments
    // is_primitive()/is_composite() => copy it, iterate arguments (if necessary)
    // is_variable() => when stack_id == stack_base, change it to args[i],
    //      otherwise continue;
    // is_universal() =>  stack_base+1

    switch (target->_type) {
    case Type::Type_t::PRIMITIVE: {
        std::vector<pType> new_args;
        for (const auto& t : target->as<PrimitiveType>()->args) {
            new_args.push_back(evaluate(t, args, stack_base, info));
        }
        return std::make_shared<PrimitiveType>(target->as<PrimitiveType>()->ref, new_args);
    }
    case Type::Type_t::OBJECT: {
        // TODO implement ObjectType
        return NULL;
    }
    case Type::Type_t::STRUCT: {
        std::unordered_map<std::string, pType> new_fields;
        for (const auto& k : target->as<StructType>()->fields) {
            new_fields.insert({ k.first, evaluate(k.second, args, stack_base, info) });
        }
        return std::make_shared<StructType>(new_fields);
    }
    case Type::Type_t::UNIVERSAL: {
        /* we allow nested UT in principle (e.g. forall<X>.forall<Y>.A(X,Y) )
        however I forbid writing them directly -- since it will be redundant and greatly increases the 
        parsing complexity. Such writing is generally discouraged. I'll see if a "merge" process is necessary.
        */

        auto tt = target->as<UniversalType>();
        return std::make_shared<UniversalType>(tt->quantifiers, 
            evaluate(tt->body, args, stack_base + 1, info));
    }
    case Type::Type_t::VARIABLE: {
        auto tt = target->as<UniversalTypeVariable>();
        if (tt->stack_id == stack_base) {
            return args[tt->arg_id];    // Note here we just use the pointer; 
        }
        else {
            return target;
            // NOTE if use other pointer than shared_ptr,
            // tt->quantifier may break -- use the newly generated quantifier instead
        }
    }
    default: {
        throw std::runtime_error("Unrecognized type instance");
    }
    }
}

OutputStream& PrimitiveType::print(OutputStream& os)const {
    os << ref->symbol->get_name();
    if (args.size() > 0) {
        os << "(";
        for (size_t i = 0; i < args.size() - 1; i++) {
            os << *args[i] << ",";
        }
        os << *args.back() << ")";
    }
    return os;
}

OutputStream& StructType::print(OutputStream& os)const {
    os << '{';
    auto k = fields.begin();
    if (k == fields.end()) return os << '}';
    os << k->first << '=' << *k->second;
    k++;
    for (; k != fields.end(); k++) {
        os << ',' << k->first << '=' << *k->second;
    }
    return os << '}';
}

OutputStream& ObjectType::print(OutputStream& os)const {
    return os << ref->symbol->get_name();
}

OutputStream& UniversalTypeVariable::print(OutputStream& os)const {
    return os << "$[" << stack_id << ',' << arg_id << ']';
}

OutputStream& UniversalType::print(OutputStream& os)const {
    os << "forall<";
    if (quantifiers.size() > 0) {
        os << "$0";
        if (!quantifiers[0]->is_top()) os << " implements " << *quantifiers[0];
        for (size_t i = 0; i < quantifiers.size() - 1; i++) {
            os << ",$" << i;
            if (!quantifiers[i]->is_top()) os << " implements " << *quantifiers[i];
        }
    }
    return os << ">." << *body;
}
