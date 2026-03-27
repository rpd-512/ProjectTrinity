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
// Quaternary logic: four values.
//   W0 = 0  (analogous to T_NEG   in the ternary system)
//   W1 = 1  (analogous to T_ZERO  in the ternary system)
//   W2 = 2
//   W3 = 3  (analogous to T_POS   in the ternary system)
enum wire : int {
    W0 = 0,
    W1 = 1,
    W2 = 2,
    W3 = 3
};

static constexpr int BASE = 4;          // k
static constexpr int BASE2 = 16;        // k^2 — entries per binary truth table
static constexpr int UNARY_COUNT = 256; // k^k = 4^4 — total unary functions
static constexpr int DELTA_COUNT = 4;   // k    — constant unary functions

static const wire WIRE_VALS[BASE] = { W0, W1, W2, W3 };

// ─── Truth-table helpers ──────────────────────────────────────────────────────
// Unary  function: BASE   entries → vector<wire> of size BASE
// Binary function: BASE^2 entries → vector<wire> of size BASE2
// Ordering: row-major, index = a*BASE + b

template<typename UnaryFunc>
vector<wire> get_vector(UnaryFunc f) {
    vector<wire> v(BASE);
    for (int i = 0; i < BASE; i++)
        v[i] = f(WIRE_VALS[i]);
    return v;
}

template<typename BinaryFunc>
vector<wire> get_binary_vector(BinaryFunc f) {
    vector<wire> v(BASE2);
    int k = 0;
    for (int i = 0; i < BASE; i++)
        for (int j = 0; j < BASE; j++)
            v[k++] = f(WIRE_VALS[i], WIRE_VALS[j]);
    return v;
}

// Compose two unary truth-tables through gate Arb: h(x) = Arb(f(x), g(x))
template<typename ArbFunc>
vector<wire> compose(
    const vector<wire>& f,
    const vector<wire>& g,
    ArbFunc Arb)
{
    vector<wire> h(BASE);
    for (int i = 0; i < BASE; i++)
        h[i] = Arb(f[i], g[i]);
    return h;
}

// ─── VectorHash ───────────────────────────────────────────────────────────────
struct VectorHash {
    size_t operator()(const vector<wire>& v) const {
        size_t h = 0;
        for (auto x : v)
            h = h * BASE + (int)x;
        return h;
    }
};

// ─── Delta predicate ──────────────────────────────────────────────────────────
// A constant unary function: f(0)==f(1)==f(2)==f(3).
// There are BASE=4 such functions.
inline bool is_delta(const vector<wire>& v) {
    if ((int)v.size() != BASE) return false;
    for (int i = 1; i < BASE; i++)
        if (v[i] != v[0]) return false;
    return true;
}

#endif // ESSENTIALS_H
