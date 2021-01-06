#ifndef MINI_VM_H
#define MINI_VM_H

#include "ir.h"
#include "memory.h"

namespace mini {


    class Stack {
    public:

        static const Size_t max_size = 2097152; // TODO changeable stack size

        Stack() : sp(0), bp(0), _storage(1, StackElem()) {}

        // sp <-- sp+size
        void grow(Size_t size) {
            sp += size;
            if (_storage.size() < sp + 1) {
                _storage.resize(sp + 1);
            }
            if (size >= max_size) {
                throw RuntimeError("Stack overflow");
            }
        }
        void shrink(Size_t size) {
            sp -= size;
        }
        // *sp <-- val; sp++
        void push(const StackElem& val) {
            _storage[sp] = val;
            grow(1);
        }
        // *sp = bp; sp++; 
        void push_bp() {
            push(bp);
            bp = sp;
        }
        // sp = bp-1; bp = *sp
        void pop_bp() {
            sp = bp - 1;
            bp = _storage[sp].aarg;
        }

        // stack[sp + offset] no boundary check
        const StackElem& sp_offset(Offset_t offset)const {
            return _storage[sp + offset];
        }
        StackElem& sp_offset(Offset_t offset) {
            return _storage[sp + offset];
        }
        const StackElem& bp_offset(Offset_t offset)const {
            return _storage[bp + offset];
        }
        StackElem& bp_offset(Offset_t offset) {
            return _storage[bp + offset];
        }

        StackElem pop() {
            return _storage[--sp];
        }
        // sp_offset(-1)
        const StackElem& top()const {
            return _storage[sp - 1];
        }
        StackElem& top() {
            return _storage[sp - 1];
        }
        // sp_offset(-2)
        const StackElem& top2()const {
            return _storage[sp - 2];
        }
        StackElem& top2() {
            return _storage[sp - 2];
        }

        Size_t sp;
        Size_t bp;
    private:

        std::vector<StackElem> _storage;
    };

    class VM {
    public:

        void load(const IRProgram& irprog) {
            this->irprog = &irprog;
            call(this->irprog->entry_index);
            build_field_indices_map();
            allocate_class(this->irprog->global_pool_index);
            global_addr = stack.pop().aarg;
        }

        void run() {
            while (!terminate_flag) {

                try {
                    execute(fetch());
                }
                catch (const RuntimeError& e) {
                    handle_error(e);
                    terminate_flag = true;
                    error_flag = 1;
                }
            }
        }

        const ByteCode& fetch() {
            if (pc < cur_function->codes.size()) {
                return cur_function->codes[pc++];
            }
            else {
                throw RuntimeError("Function end without return");
            }
        }

        void execute(const ByteCode& code);

        void handle_error(const RuntimeError& e);

        void build_field_indices_map();

        // local varible -> stack top
        void load_local(Size_t index);

        void store_local(Size_t index, StackElem value);

        // fetch value at certain index
        void load_index(Address addr, Size_t index, Size_t typebit);

        void store_index(Address addr, Size_t index, StackElem value, Size_t typebit);

        void load_field(Address addr, Size_t field_index);

        void store_field(Address addr, Size_t field_index, StackElem value);

        void load_interface(Address addr, Size_t field_id);

        void store_interface(Address addr, Size_t field_id, StackElem value);

        void _get_class_and_field(Address addr, Size_t field_index, ClassObject*& cobj, Size_t& field_offset, Size_t& sz_field);

        void _get_interface_class_and_field(Address addr, Size_t field_index, ClassObject*& cobj, Size_t& field_offset, Size_t& sz_field);

        void load_constant(Size_t index);

        void allocate_array(Size_t size, Size_t typebit);
        
        void allocate_class(Size_t index);

        void allocate_closure(Size_t index);

        void call_closure(Address addr);

        void call_native(int index);

        void call(Size_t index);

        void ret(bool has_value);

        void runtime_assert(bool value, const char* msg) {
            if (!value) {
                throw RuntimeError(msg);
            }
        }

        int error_flag = 0;

        int open_file(const std::string& name, const std::string& mode);

        void close_file(int fd);

        std::fstream& get_file(int fd);

        void _fetch_string(Address addr, std::string& buf);

        Address _store_string(const std::string& buf);

    private:

        bool terminate_flag = false;

        Size_t pc = 0;
        Size_t pc_func = 0;     // two components of pc
        const Function* cur_function = nullptr;

        Address global_addr;         // global pool address (in heap)
        const IRProgram* irprog;
        Stack stack;
        MemorySection heap;

        struct FieldKey { 
            Size_t info_index, field_id;  
            size_t to_key()const { return (size_t(info_index) << 32) + field_id; }
        };
        struct FieldLocation { Size_t field_index, is_global; };

        std::unordered_map<uint64_t, FieldLocation> field_indices;
        std::unordered_map<int, std::fstream> file_descriptors;
        int current_fd = 3;
    };

}

#endif