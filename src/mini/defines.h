#ifndef DEFINES_H
#define DEFINES_H

#include <memory>

namespace mini {

    template <typename T>
    using Ptr = std::shared_ptr<T>;

    typedef uint32_t Index_t;
}

#endif