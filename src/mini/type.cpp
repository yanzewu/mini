#include "type.h"
#include "symtable.h"

using namespace mini;

bool Type::equals(const Type& lhs, const Type& rhs) {
    
    if (lhs.ref != rhs.ref && lhs.is_custom_class() || (lhs.ref->get_type() != rhs.ref->get_type())) return false;
    if (lhs.args.size() != rhs.args.size()) return false;
    for (size_t i = 0; i < lhs.args.size(); i++) {
        if (!equals(*lhs.args[i], *rhs.args[i])) return false;
    }

    return true;
}

Type::Ordering Type::compare(const Type& lhs, const Type& rhs) {
    auto ret = partial_compare(lhs, rhs);
    if (ret == Ordering::UNCOMPARABLE) {
        return _reverse(partial_compare(rhs, lhs));
    }
    else {
        return ret;
    }
}

Type::Ordering Type::partial_compare(const Type& lhs, const Type& rhs) {
    if (lhs.is_object()) {
        return rhs.is_object() ? Ordering::EQUAL : Ordering::GREATER;
    }
    else if (rhs.is_bottom()) {
        return lhs.is_bottom() ? Ordering::EQUAL : Ordering::GREATER;
    }
    else if (lhs.is_atomic()) {
        return lhs.ref->get_type() == rhs.ref->get_type() ? Ordering::EQUAL : Ordering::UNCOMPARABLE;
    }
    else if (lhs.is_list()) {
        if (rhs.is_list()) return Ordering::EQUAL;
        else if (rhs.is_atomic()) return Ordering::UNCOMPARABLE;
        else return Ordering::GREATER;
    }
    else if (lhs.is_array()) {
        if (rhs.is_array() && lhs.args.size() == rhs.args.size()) {
            if (rhs.args[0]->is_bottom()) return Ordering::GREATER;     // Special case of array. Will be replaced with RANK-2 type 'any'
            else return partial_compare(*lhs.args[0], *rhs.args[0]);    // here we use inherit semantics (recursive semantics are also possible)
        }
        else {
            return Ordering::UNCOMPARABLE;
        }
    }
    else if (lhs.is_tuple()) {
        if (rhs.is_tuple() && lhs.args.size() == rhs.args.size()) {
            bool is_equal = true;
            for (size_t j = 0; j < lhs.args.size(); j++) {
                auto t = partial_compare(*lhs.args[j], *rhs.args[j]);
                if (t == Ordering::EQUAL) continue;
                else if (t == Ordering::GREATER) is_equal = false;
                else return Ordering::UNCOMPARABLE;
            }
            return is_equal ? Ordering::EQUAL : Ordering::GREATER;
        }
        else {
            return Ordering::UNCOMPARABLE;
        }
    }
    else if (lhs.is_function()) {
        if (rhs.is_function()) {
            if (lhs.args.size() != rhs.args.size()) return Ordering::UNCOMPARABLE;
            for (size_t j = 0; j < lhs.args.size() - 1; j++) if (!lhs.args[j]->equals(*rhs.args[j])) return Ordering::UNCOMPARABLE;
            return partial_compare(*lhs.args.back(), *rhs.args.back());     // covariant of return type
        }
        else {
            return Ordering::UNCOMPARABLE;
        }
    }
    else if (lhs.is_struct()) {
        if (rhs.is_struct() || rhs.is_custom_class()) {
            auto& m_lhs = static_cast<const StructType&>(lhs);
            auto& m_rhs = static_cast<const StructType&>(rhs);
            return StructType::field_compatible(m_lhs, m_rhs);
        }
        else {
            return Ordering::UNCOMPARABLE;
        }
    }

    else if (lhs.is_custom_class()) {
        if (!rhs.is_custom_class()) {
            return Ordering::UNCOMPARABLE;
        }
        else {
            return inherits(lhs, rhs);
        }
    }
    else {
        return equals(lhs, rhs) ? Ordering::EQUAL : Ordering::UNCOMPARABLE;
    }
}

Type::Ordering Type::inherits(const Type& lhs, const Type& rhs) {

    if (lhs.ref == rhs.ref) {
        return equals(lhs, rhs) ? Ordering::EQUAL : Ordering::UNCOMPARABLE;
    }
    else {
        auto m_lhs = lhs;
        auto m_ref = lhs.ref;
        while (m_ref->parent != NULL) {
            m_lhs = m_ref->parent->evaluates_to(m_lhs.args);
            m_ref = m_lhs.ref;

            if (equals(m_lhs, rhs)) return Ordering::GREATER;
        }
        return Ordering::UNCOMPARABLE;
    }
}

Type::Ordering Type::_reverse(Ordering ordering_) {
    return ordering_ == Ordering::LESS ? Ordering::GREATER : (
        ordering_ == Ordering::GREATER ? Ordering::LESS : ordering_);
}



OutputStream& Type::print(OutputStream& os)const {
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

Type::Ordering StructType::field_compatible(const StructType& lhs, const StructType& rhs) {

    for (const auto& key : lhs.fields) {
        if (rhs.fields.count(key.first) && Type::equals(*key.second, *rhs.fields.at(key.first))) {
        }
        else {
            return Ordering::UNCOMPARABLE;
        }
    }
    return lhs.fields.size() == rhs.fields.size() ? Ordering::EQUAL : Ordering::GREATER;
}
