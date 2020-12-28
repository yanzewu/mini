#ifndef MINI_IR_H
#define MINI_IR_H

#include "bytecode.h"
#include "symbol.h"
#include "type.h"

namespace mini {

    //class BinaryStream;

    typedef uint32_t Size_t;
    typedef int32_t Offset_t;

    class IRProgram;

    class ConstantPoolObject {
    public:

        enum Type_t {
            STRING = 0,
            FUNCTION = 1,
            CLASS_LAYOUT = 2,
            FUNCTION_INFO = 3,
            CLASS_INFO = 4,
            LINE_NUMBER_TABLE = 5,
        };
        Type_t _type;

        Type_t get_type()const { return _type; }

        virtual Size_t size()const { return 0;}    // the size after serialization

      //  virtual void serialize(BinaryStream&)const;

      //  virtual void deserialize(BinaryStream&);

        virtual OutputStream& print(OutputStream& os)const {
            return os;
        }

        template<typename T>
        const T* as()const {
            static_assert(std::is_base_of<ConstantPoolObject, T>::value);
            return static_cast<const T*>(this);
        }

        template<typename T>
        T* as() {
            static_assert(std::is_base_of<ConstantPoolObject, T>::value);
            return static_cast<T*>(this);
        }

    protected:
        explicit ConstantPoolObject(Type_t type) : _type(type) {}
    };

    inline OutputStream& operator<<(OutputStream& os, const ConstantPoolObject& cpo) {
        return cpo.print(os);
    }

    class StringConstant : public ConstantPoolObject {
    public:

        std::string value;

        StringConstant() : ConstantPoolObject(Type_t::STRING) {}
        explicit StringConstant(const std::string& s) : ConstantPoolObject(Type_t::STRING), value(s) {}

        Size_t size()const {
            return value.size() + sizeof(Size_t);
        }

        OutputStream& print(OutputStream& os)const {
            return os << value;
        }
    };

    class Function : public ConstantPoolObject {
    public:

        std::vector<ByteCode> codes;
        Size_t sz_arg;
        Size_t sz_bind;
        Size_t sz_local;
        Size_t info_index;

        Function() : ConstantPoolObject(Type_t::FUNCTION) {}

        OutputStream& print(OutputStream& os, const IRProgram& irprog)const;

        OutputStream& print(OutputStream& os)const {
            return os << "#" << info_index;
        }

        OutputStream& print_code(OutputStream& os, const IRProgram& irprog)const;
    };

    class ClassLayout : public ConstantPoolObject {
    public:

        std::vector<Size_t> offset;
        Size_t info_index;

        ClassLayout() : ConstantPoolObject(ConstantPoolObject::CLASS_LAYOUT) {}
        
        Size_t size()const {
            return offset.size() * sizeof(Size_t) + 2 * sizeof(Size_t);
        }

        OutputStream& print(OutputStream& os)const {
            return os << '#' << info_index;
        }

        OutputStream& print(OutputStream& os, const IRProgram& irprog)const;
    };

    class FunctionInfo : public ConstantPoolObject {
    public:

        Size_t name_index;
        SymbolInfo symbol_info;  // the 'filename' entry will be changed to cp addr
        std::vector<Size_t> arg_index;
        Size_t ret_index;

        FunctionInfo() : ConstantPoolObject(Type_t::FUNCTION_INFO) {}

        Size_t size()const {
            return arg_index.size() * sizeof(Size_t) + 3 * sizeof(Size_t);
        }

        // just print the name
        OutputStream& print_simple(OutputStream& os, const IRProgram& irprog)const;

        OutputStream& print(OutputStream& os, const IRProgram& irprog)const;

        OutputStream& print(OutputStream& os)const {
            os << "name=#" << name_index << ", location=" << symbol_info;
            os << " ,types=(#";
            for (const auto& a : arg_index) os << a << ",#";
            os << ret_index << ")";
            return os;
        }
    };

    class ClassInfo : public ConstantPoolObject {
    public:

        struct FieldInfo {
            Size_t name_index;
            Size_t type_index;
        };

        Size_t name_index;
        SymbolInfo symbol_info;
        std::vector<FieldInfo> field_info;

        ClassInfo() : ConstantPoolObject(Type_t::CLASS_INFO) {}

        // just print the name
        OutputStream& print_simple(OutputStream& os, const IRProgram& irprog)const;

        OutputStream& print(OutputStream& os)const {
            os << "name=#" << name_index << ", location=" << symbol_info;
            os << ", fields=(";
            for (size_t i = 0; i < field_info.size(); i++) {
                if (i > 0) os << ',';
                os << '#' << field_info[i].name_index << ":#" << field_info[i].type_index;
            }
            os << ')';
            return os;
        }
    };


    class LineNumberTable : public ConstantPoolObject {
    public:

       struct LineNumberPair {
            Size_t function_index;
            Size_t pc;
            Size_t line_number;

            bool operator<(const LineNumberPair& other)const {
                return function_index < other.function_index ? true : (function_index > other.function_index ? false : pc < other.pc);
            }
        };
        std::vector<LineNumberPair> line_number_table;

        LineNumberTable() : ConstantPoolObject(ConstantPoolObject::LINE_NUMBER_TABLE) {}

        void add_entry(Size_t function_addr, Size_t pc, Size_t line_number) {
            line_number_table.push_back({ function_addr, pc, line_number });
        }

        const LineNumberPair& query(Size_t function_addr, Size_t pc)const;

        OutputStream& print(OutputStream& os)const {
            return os << "LineNumberTable";
        }
        // print the line number associated with a certain function
        OutputStream& print_line_number(OutputStream& os, Size_t function_addr)const;
    };


    class IRProgram {
    public:

        std::vector<ConstantPoolObject*> constant_pool;
        Size_t source_index;
        Size_t entry_index;
        Size_t global_pool_index;
        Size_t line_number_table_index;

        OutputStream& print_full(OutputStream& os)const;

        OutputStream& print(OutputStream& os, bool with_head = true, bool with_lnt = false)const;

        OutputStream& print_symbolinfo(OutputStream& os, const SymbolInfo& info)const;

        OutputStream& print_string(OutputStream& os, Size_t addr)const {
            return os << *constant_pool[addr]->as<StringConstant>();
        }

        Size_t add_constant(ConstantPoolObject* cpo) {
            constant_pool.push_back(cpo);
            return constant_pool.size() - 1;
        }

        ConstantPoolObject* fetch_constant(Size_t size) {
            return constant_pool[size];
        }
        const ConstantPoolObject* fetch_constant(Size_t size)const {
            return constant_pool[size];
        }

        const std::string& fetch_string(Size_t index)const {
            return fetch_constant(index)->as<StringConstant>()->value;
        }

        const LineNumberTable* line_number_table()const {
            return fetch_constant(line_number_table_index)->as<LineNumberTable>();
        }

        ~IRProgram() {
            for (auto cp : constant_pool) {
                delete cp;
            }
        }

    };
    inline OutputStream& operator<<(OutputStream& os, const IRProgram& a) {
        a.print(os); return os;
    }
}

#endif