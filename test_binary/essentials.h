#ifndef ESSENTIALS_H
#define ESSENTIALS_H

#include <cstdint>
#include <vector>
#include <set>
#include <array>
#include <functional>
#include <iostream>
#include <chrono>

using namespace std;

// ─── Wire type ────────────────────────────────────────────────────────────────
// Binary logic: two values.
//   T_FALSE = 0  (analogous to T_NEG  in the ternary system)
//   T_TRUE  = 1  (analogous to T_POS  in the ternary system)
//
// We keep the enum underlying type as int so that (int)w works everywhere,
// matching the ternary code's casting idiom.
enum wire : int {
    T_FALSE = 0,
    T_TRUE  = 1
};

// ─── Truth-table helpers ──────────────────────────────────────────────────────
// A unary  function over {F,T} has 2 entries  → vector<wire> of size 2.
// A binary function over {F,T} has 4 entries  → vector<wire> of size 4.
//
// Ordering convention (mirrors ternary): row-major, inputs in {F,T} order.
//   unary : [f(F), f(T)]
//   binary: [f(F,F), f(F,T), f(T,F), f(T,T)]

static const wire WIRE_VALS[2] = { T_FALSE, T_TRUE };

// get_vector for a unary function
template<typename UnaryFunc>
vector<wire> get_vector(UnaryFunc f) {
    vector<wire> v(2);
    for (int i = 0; i < 2; i++)
        v[i] = f(WIRE_VALS[i]);
    return v;
}

// get_binary_vector for a binary function
template<typename BinaryFunc>
vector<wire> get_binary_vector(BinaryFunc f) {
    vector<wire> v(4);
    int k = 0;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            v[k++] = f(WIRE_VALS[i], WIRE_VALS[j]);
    return v;
}

// Compose two unary truth-tables through a binary gate Arb:
//   h(x) = Arb(f(x), g(x))
template<typename ArbFunc>
vector<wire> compose(
    const vector<wire>& f,
    const vector<wire>& g,
    ArbFunc Arb)
{
    vector<wire> h(2);
    for (int i = 0; i < 2; i++)
        h[i] = Arb(f[i], g[i]);
    return h;
}

// ─── VectorHash ───────────────────────────────────────────────────────────────
// Same polynomial hash as the ternary version, but base-2.
struct VectorHash {
    size_t operator()(const vector<wire>& v) const {
        size_t h = 0;
        for (auto x : v)
            h = h * 2 + (int)x;
        return h;
    }
};

// ─── Delta predicate ─────────────────────────────────────────────────────────
// In ternary, a "delta" function picks out exactly one value class.
// The binary analogue: a unary function that is a *constant* (picks out one
// value).  These are the two constant unary functions: always-F, always-T.
//
// Alternatively you may want "is_not" — NOT is the only non-trivial bijection.
// We keep is_delta as "is constant unary" to mirror the ternary semantics.
inline bool is_delta(const vector<wire>& v) {
    if (v.size() != 2) return false;
    return v[0] == v[1]; // f(F)==f(T) → constant function
}

#endif // ESSENTIALS_H