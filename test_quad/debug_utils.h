#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "essentials.h"
#include <cstdio>

inline char wire_to_char(wire w) {
    switch (w) {
        case W0: return '0';
        case W1: return '1';
        case W2: return '2';
        case W3: return '3';
        default: return '?';
    }
}

inline void print_vector(const vector<wire>& v) {
    printf("[ ");
    for (auto w : v)
        printf("%c ", wire_to_char(w));
    printf("]\n");
}

template<typename BinaryFunc>
void print_matrix(BinaryFunc f) {
    printf("     ");
    for (int j = 0; j < BASE; j++) printf(" %d", j);
    printf("\n");
    for (int i = 0; i < BASE; i++) {
        printf("  %d [", i);
        for (int j = 0; j < BASE; j++)
            printf("  %c", wire_to_char(f(WIRE_VALS[i], WIRE_VALS[j])));
        printf(" ]\n");
    }
}

#endif // DEBUG_UTILS_H
