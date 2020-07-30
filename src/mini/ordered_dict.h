#ifndef ORDERED_DICT_H
#define ORDERED_DICT_H

#include <unordered_map>
#include <vector>

namespace mini {

    template<typename Key, typename Val>
    class OrderedDict {
    public:



    private:

        std::unordered_map<Key, Val*> indexer;
        std::vector<Val> container;
    };

}


#endif