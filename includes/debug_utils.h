#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <stdio.h>
#include <vector>
#include "essentials.h"
#include <string>
#include <array>
#include <stdexcept>

using namespace std;

static inline void print_trit(wire t) {
    switch (t) {
        case T_NEG:
            printf("-");
            break;
        case T_ZERO:
            printf("0");
            break;
        case T_POS:
            printf("+");
            break;
        default:
            printf("INVALID(%u)", t);
            break;
    }
}


static inline void print_vector(const vector<wire>& v){
    printf("[ ");
    for(auto w : v){
        print_trit(w);
        printf(" ");
    }
    printf("]");
}

template<typename Gate>
vector<wire> get_vector(Gate f)
{
    wire vals[3] = {T_NEG, T_ZERO, T_POS};

    if constexpr (is_invocable_v<Gate, wire, wire>) {
        vector<wire> v(9);
        int k = 0;

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                v[k++] = f(vals[i], vals[j]);

        return v;
    }
    else if constexpr (is_invocable_v<Gate, wire>) {
        vector<wire> v(3);

        for(int i = 0; i < 3; i++)
            v[i] = f(vals[i]);

        return v;
    }
    else {
        static_assert(sizeof(Gate) == 0,
                      "Gate must accept (wire) or (wire,wire)");
    }
}

static inline void print_matrix(vector<wire> gate){
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            print_trit(gate[3*i + j]);
            printf(" ");
        }
        printf("\n");
    }
}

vector<wire> compose(const vector<wire>& f,
                     const vector<wire>& g,
                     vector<wire> Arb)
{
    size_t n = f.size();
    vector<wire> h(n);
    for(int i = 0; i < n; i++){
        h[i] = Arb[3*f[i] + g[i]];
    }
    return h;
}


bool is_delta(const vector<wire>& v){
    return
        (v == vector<wire>{T_POS,  T_NEG,  T_NEG}) ||
        (v == vector<wire>{T_NEG,  T_POS,  T_NEG}) ||
        (v == vector<wire>{T_NEG,  T_NEG,  T_POS});
}

bool is_in_list(const vector<wire>& v, const vector<vector<wire>>& list){
    for(const auto& item : list)
        if(v == item)
            return true;
    return false;
}


auto string_to_gate(const string& str) {
    if (str.size() != 9)
        throw invalid_argument("Gate string must have length 9");

    array<wire, 9> table;

    for (size_t i = 0; i < 9; i++) {
        switch (str[i]) {
            case '-':
                table[i] = T_NEG;
                break;
            case '0':
                table[i] = T_ZERO;
                break;
            case '+':
                table[i] = T_POS;
                break;
            default:
                throw invalid_argument("Invalid character in gate string");
        }
    }

    return [table](wire a, wire b) {
        return table[a * 3 + b];
    };
}


#endif // DEBUG_UTILS_H