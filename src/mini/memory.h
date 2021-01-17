#ifndef MEMORY_H
#define MEMORY_H

#include "ir.h"

#include <unordered_map>
#include <vector>

namespace mini {


    class MemoryObject {
    public:

        enum class Type_t : unsigned char {
            ARRAY = 0,
            CLOSURE = 1,
            CLASS = 2
        };

        Type_t type;
        Size_t size;          // size of data in bytes. Total size should be sizeof(T) + T.size
        Size_t ref_count = 0;     // also metadata for gc
        void* data;


        void set_dataptr(void* ptr, Size_t size) {
            data = ptr;
            this->size = size;
        }

        char fetch(Size_t index)const {
            return static_cast<char*>(data)[index];
        }
        void store(Size_t index, char value) {
            static_cast<char*>(data)[index] = value;
        }
        // index is in bytes!
        StackElem fetch4(Size_t index)const {
            return *reinterpret_cast<StackElem*>(static_cast<char*>(data) + index);
        }
        // index is in bytes!
        void store4(Size_t index, StackElem value) {
            *reinterpret_cast<StackElem*>(static_cast<char*>(data) + index) = value;
        }

        virtual ~MemoryObject() {}

        template<class T>
        const T* as()const {
            static_assert(std::is_base_of<MemoryObject, T>::value);
            return static_cast<const T*>(this);
        }

        template<class T>
        T* as() {
            static_assert(std::is_base_of<MemoryObject, T>::value);
            return static_cast<T*>(this);
        }

    protected:

        MemoryObject(Type_t t) : type(t), size(0), data(nullptr) {}
    };

    class ArrayObject : public MemoryObject {
    public:
        ArrayObject() : MemoryObject(MemoryObject::Type_t::ARRAY) {}
    };

    class ClosureObject : public MemoryObject {
    public:
        ClosureObject() : MemoryObject(MemoryObject::Type_t::CLOSURE) {}

        Size_t function_addr()const {
            return static_cast<Size_t*>(data)[0];
        }
        // in bytes
        Size_t data_size()const {
            return size - 4;
        }
        void set_function_addr(Size_t addr) {
            static_cast<Size_t*>(data)[0] = addr;
        }

        // move data from src with data_size(); will NOT check boundary
        void move_from(StackElem* src) {
            memcpy(static_cast<char*>(data) + 4, src, data_size());
        }
        void move_to(StackElem* dst) {
            memcpy(dst, static_cast<char*>(data) + 4, data_size());
        }

    };

    class ClassObject : public MemoryObject {
    public:
        ClassObject() : MemoryObject(MemoryObject::Type_t::CLASS) {}

        Size_t layout_addr()const {
            return static_cast<Size_t*>(data)[0];
        }
        void set_layout_addr(Size_t addr) {
            static_cast<Size_t*>(data)[0] = addr;
        }
    };

    class MemorySection {
    public:

        std::unordered_map<Address, MemoryObject*> table;

        MemorySection() {}

        MemoryObject* fetch(Address addr) {
            return const_cast<MemoryObject*>(const_cast<const MemorySection*>(this)->fetch(addr));
        }
        const MemoryObject* fetch(Address addr)const {
            if (addr == 0) {
                throw RuntimeError("The memory address is null");
            }
            auto t = table.find(addr);
            if (t == table.end()) {
                throw RuntimeError("Invalid address: " + std::to_string(addr));
            }
            else {
                return t->second;
            }
        }

        Address allocate(MemoryObject::Type_t objtype, Size_t dyn_size);

        Address get_free_slot()const;

        ~MemorySection() {
            for (const auto& t : table) {
                free(t.second->data);
                delete t.second;
            }
        }

    };

}

#endif