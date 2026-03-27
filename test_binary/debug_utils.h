#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "essentials.h"
#include <cstdio>

// ─── Print helpers ────────────────────────────────────────────────────────────

inline char wire_to_char(wire w) {
    switch (w) {
        case T_FALSE: return '0';
        case T_TRUE:  return '1';
        default:      return '?';
    }
}

// Print a unary truth-table  [f(0), f(1)]
inline void print_vector(const vector<wire>& v) {
    printf("[ ");
    for (auto w : v)
        printf("%c ", wire_to_char(w));
    printf("]\n");
}

// Print a binary truth-table as a 2×2 matrix:
//
//        0   1
//   0 [  .   . ]
//   1 [  .   . ]
//
template<typename BinaryFunc>
void print_matrix(BinaryFunc f) {
    printf("     0   1\n");
    for (int i = 0; i < 2; i++) {
        printf("  %d [", i);
        for (int j = 0; j < 2; j++)
            printf("  %c", wire_to_char(f(WIRE_VALS[i], WIRE_VALS[j])));
        printf(" ]\n");
    }
}

#endif // DEBUG_UTILS_H