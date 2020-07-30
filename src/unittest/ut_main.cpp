
#include "../mini/mini.h"
#include "ut.h"

#include "test_lexer.h"
#include "test_parser.h"
#include "test_typing.h"

unsigned tested_cases = 0;
unsigned correct_cases = 0;

void m_assert(bool result, const char* desc) {
    tested_cases++;
    if (result) {
        correct_cases++;
        printf("Test case %d: Passed\n", tested_cases);
    }
    else {
        if (desc) {
            printf("Test case %d: Failed. Content: %s\n", tested_cases, desc);
        }
        else {
            printf("Test case %d: Failed\n", tested_cases);
        }
    }
}

void fail(const char* s, const char* desc) {
    tested_cases++;
    if (desc) {
        printf("Test case %d: Error: %s; Content: %s\n", tested_cases, s, desc);
    }
    else {
        printf("Test case %d: Error: %s\n", tested_cases, s);
    }
    
}

void summary() {
    printf("Total %d cases, %d passed (%.2f%%), %d failed (%.2f%%)\n", tested_cases, correct_cases,
        100.0*correct_cases/tested_cases, tested_cases-correct_cases, 100.0-100.0*correct_cases/tested_cases);
    tested_cases = 0;
    correct_cases = 0;
}

int main(int argc, char** argv) {
    test_lexer();
    test_parser();
    test_typing();
    return 0;
}