#ifndef MINI_CONSTANT_H
#define MINI_CONSTANT_H

#include <variant>
#include <string>

namespace mini {
    struct Constant {
        enum class Type_t {
            NIL = 0,
            BOOL = 1,
            CHAR = 2,
            INT = 3,
            FLOAT = 4,
            STRING = 5
        };

        typedef std::monostate Nil;

        typedef std::variant<Nil, bool, char, int, float, std::string> DataType;
        DataType data;

        Constant() {}
        explicit Constant(bool val) : data(val) {}
        explicit Constant(char val) : data(val) {}
        explicit Constant(int val) : data(val) {}
        explicit Constant(float val) : data(val) {}
        explicit Constant(const std::string& val) : data(val) {}
        explicit Constant(const char* val) : data(std::string(val)) {}

        std::string to_str()const;
        Constant::Type_t get_type()const {
            return (Constant::Type_t)data.index();
        }
        bool operator==(const Constant& rhs)const {
            return this->data == rhs.data;
        }
    };
}

#endif