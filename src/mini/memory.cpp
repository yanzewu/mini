
#include "memory.h"
#include "ir.h"

using namespace mini;

Address MemorySection::allocate(MemoryObject::Type_t objtype, Size_t dyn_size) {

    // if there is a memory pool, we would 
    // 1) allocate a segment of memory with sizeof(xxxObject) + dyn_size
    // 2) set p->set_dataptr(p+sizeof(xxxObject), dyn_size)
    // 3) cast.

    Address addr = get_free_slot();
    MemoryObject* p = 0;
    switch (objtype)
    {
    case MemoryObject::Type_t::ARRAY: p = new ArrayObject(); break;
    case MemoryObject::Type_t::CLOSURE: p = new ClosureObject(); break;
    case MemoryObject::Type_t::CLASS: p = new ClassObject(); break;
    default:
        break;
    }
    p->set_dataptr(malloc(dyn_size), dyn_size); // may be replaced by custom allocator

    table[addr] = p;

    return addr;
}

Address MemorySection::get_free_slot() const {
    return table.size() + 1;
}
