
#ifndef MINI_DEPENDENCY_H
#define MINI_DEPENDENCY_H

#include "symbol.h"

#include <vector>
#include <unordered_set>

namespace mini {

    // Check dependency of variable definition
    class DependencyChecker {
    public:

        std::vector<std::unordered_set<unsigned>> graph;

        // add a node
        void insert(unsigned id) {
            graph.push_back({ id });
        }

        // register a new dependency from src->dst
        void connect(unsigned src, unsigned dst) {
            graph[src].insert(dst);
        }

        // cache the dependency of each node. This must be called before is_defined().
        void spread_dependency() {

            std::vector<char> seen(graph.size(), 0);

            for (unsigned i = 0; i < graph.size(); i++) {
                std::fill(seen.begin(), seen.end(), 0);
                seen[i] = 1;
                _walk(i, i, seen);
            }
        }

        void _walk(unsigned idx, unsigned src, std::vector<char>& seen) {
            for (const auto i : graph[idx]) {
                if (!seen[i]) {
                    graph[src].insert(i);
                    seen[i] = 1;
                    _walk(i, src, seen);
                }
            }
        }

        // check if symbol 'query' is defined before 'location' (exclude)
        bool is_defined(const SymbolInfo& location, const SymbolInfo& query)const {
            if (query.is_absolute()) return true;

            auto cur_loc = location.get_location();
            auto query_loc = query.get_location();

            if (cur_loc.srcno == query_loc.srcno) {
                return cur_loc.lineno == query_loc.lineno ?
                    (cur_loc.colno > query_loc.colno) :
                    (cur_loc.lineno > query_loc.lineno);
            }
            else {
                return graph[cur_loc.srcno].find(query_loc.srcno) != graph[cur_loc.srcno].end();
            }
        }
    };

}

#endif