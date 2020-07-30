// Should only be included once.

#ifndef UT_H
#define UT_H

#include <stdexcept>

void m_assert(bool result, const char* desc = 0);
void fail(const char* s, const char* desc = 0);
void summary();

#define REQUIRE(x, d) try {m_assert(x, d);} catch(const std::exception& e) {fail(e.what(), d);}

#endif