#ifndef SEARCH_UTILS_BINARY_H
#define SEARCH_UTILS_BINARY_H

#include "essentials.h"
#include "search_utils_unary.h"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <chrono>

// ─── gate_search ──────────────────────────────────────────────────────────────
// Given a binary gate Arb, try to generate `target` (a 4-entry truth table)
// by composing binary functions of the form  h(a,b) = Arb(f(a,b), g(a,b)).
//
// Seed set:
//   P1   : (a,b) → a          projection 1
//   P2   : (a,b) → b          projection 2
//   Cn/Cp: constants  (if constant_basis)
//   G    : (a,b) → Arb(a,b)  the gate itself
//
// Returns the gate-depth at which target was found, or -1 if unreachable.
// Also returns early if any function in `memo` is generated (short-circuit for
// universal gate search).
//
template<typename ArbFunc>
int gate_search(
    ArbFunc Arb,
    vector<wire> target,
    ExhaustMode mode          = ExhaustMode::FAST_MODE,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis       = true,
    bool debug_print          = false)
{
    const bool DEBUG = (mode == ExhaustMode::DEBUG_MODE);
    auto start = chrono::high_resolution_clock::now();

    // Binary projections and constants
    auto P1 = [](wire a, wire b) -> wire { return a; };
    auto P2 = [](wire a, wire b) -> wire { return b; };
    auto CF = [](wire a, wire b) -> wire { return T_FALSE; };
    auto CT = [](wire a, wire b) -> wire { return T_TRUE;  };
    auto G  = [Arb](wire a, wire b) -> wire { return Arb(a, b); };

    // Compose two binary truth-tables through Arb:  h[i] = Arb(f[i], g[i])
    auto compose_binary = [&](const vector<wire>& f, const vector<wire>& g) {
        vector<wire> h(4);
        for (int i = 0; i < 4; i++)
            h[i] = Arb(f[i], g[i]);
        return h;
    };

    // Total binary functions over {0,1}^2 = 2^4 = 16
    const size_t TOTAL_BINARY = 16;

    // ── DEBUG mode ───────────────────────────────────────────────────────────
    if (DEBUG) {
        map<vector<wire>, pair<string,int>> S;

        S[get_binary_vector(P1)] = {"a",        0};
        S[get_binary_vector(P2)] = {"b",        0};
        S[get_binary_vector(G)]  = {"Arb(a,b)", 0};
        if (constant_basis) {
            S[get_binary_vector(CF)] = {"F", 0};
            S[get_binary_vector(CT)] = {"T", 0};
        }

        for (const auto& [v, info] : S) {
            if (v == target || memo.count(v)) {
                if (debug_print) {
                    printf("TARGET FOUND in seeds!\n  expr: %s\n  depth: %d\n",
                           info.first.c_str(), info.second);
                }
                return info.second;
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;
            vector<pair<vector<wire>, pair<string,int>>> current(S.begin(), S.end());

            for (const auto& [v1, i1] : current)
            for (const auto& [v2, i2] : current) {
                auto h = compose_binary(v1, v2);

                if (S.find(h) == S.end()) {
                    int    d    = max(i1.second, i2.second) + 1;
                    string expr = "Arb(" + i1.first + ", " + i2.first + ")";
                    S[h] = {expr, d};
                    changed = true;

                    if (h == target || memo.count(h)) {
                        if (debug_print)
                            printf("TARGET FOUND!\n  expr: %s\n  depth: %d\n  funcs: %zu\n",
                                   expr.c_str(), d, S.size());
                        return d;
                    }
                }
            }
        }

        if (debug_print)
            printf("Target not generated. Total funcs: %zu\n", S.size());
        return -1;
    }

    // ── FAST_MODE (with depth) ───────────────────────────────────────────────
    else if (mode == ExhaustMode::FAST_MODE) {
        unordered_map<vector<wire>, int, VectorHash> S;

        S[get_binary_vector(P1)] = 0;
        S[get_binary_vector(P2)] = 0;
        S[get_binary_vector(G)]  = 0;
        if (constant_basis) {
            S[get_binary_vector(CF)] = 0;
            S[get_binary_vector(CT)] = 0;
        }

        for (const auto& [v, d] : S)
            if (v == target || memo.count(v)) return d;

        bool changed = true;
        while (changed) {
            changed = false;
            vector<pair<vector<wire>,int>> current(S.begin(), S.end());

            for (const auto& [f, df] : current)
            for (const auto& [g, dg] : current) {
                auto h = compose_binary(f, g);

                if (!S.count(h)) {
                    int d = max(df, dg) + 1;
                    S[h] = d;
                    changed = true;

                    if (h == target || memo.count(h)) return d;
                    if (S.size() == TOTAL_BINARY) return -1;
                }
            }
        }
        return -1;
    }

    // ── FAST_NO_DEPTH (set only) ─────────────────────────────────────────────
    else if (mode == ExhaustMode::FAST_NO_DEPTH) {
        set<vector<wire>> S;

        S.insert(get_binary_vector(P1));
        S.insert(get_binary_vector(P2));
        S.insert(get_binary_vector(G));
        if (constant_basis) {
            S.insert(get_binary_vector(CF));
            S.insert(get_binary_vector(CT));
        }

        for (const auto& v : S)
            if (v == target || memo.count(v)) return 1;

        bool changed = true;
        while (changed) {
            changed = false;
            vector<vector<wire>> current(S.begin(), S.end());

            for (const auto& f : current)
            for (const auto& g : current) {
                auto h = compose_binary(f, g);

                if (S.insert(h).second) {
                    if (h == target || memo.count(h)) return 1;
                    changed = true;
                    if (S.size() == TOTAL_BINARY) return -1;
                }
            }
        }
        return -1;
    }

    return -1;
}

// ─── nand_search ──────────────────────────────────────────────────────────────
// Check whether gate Arb can generate NAND.
//
// Binary NAND truth table (row-major, inputs in {F,T} order):
//   NAND(F,F)=T,  NAND(F,T)=T,  NAND(T,F)=T,  NAND(T,T)=F
//   → [T, T, T, F]
//
template<typename ArbFunc>
bool nand_search(
    ArbFunc Arb,
    ExhaustMode mode          = ExhaustMode::FAST_NO_DEPTH,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis       = true)
{
    vector<wire> nand_target = { T_TRUE, T_TRUE, T_TRUE, T_FALSE };
    return gate_search(Arb, nand_target, mode, memo, constant_basis) > 0;
}

// ─── nor_search ───────────────────────────────────────────────────────────────
// Check whether gate Arb can generate NOR.
//
// Binary NOR truth table:
//   NOR(F,F)=T,  NOR(F,T)=F,  NOR(T,F)=F,  NOR(T,T)=F
//   → [T, F, F, F]
//
template<typename ArbFunc>
bool nor_search(
    ArbFunc Arb,
    ExhaustMode mode          = ExhaustMode::FAST_NO_DEPTH,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis       = true)
{
    vector<wire> nor_target = { T_TRUE, T_FALSE, T_FALSE, T_FALSE };
    return gate_search(Arb, nor_target, mode, memo, constant_basis) > 0;
}

#endif // SEARCH_UTILS_BINARY_H