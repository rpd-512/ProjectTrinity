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
// Given a binary gate Arb, try to generate `target` (a BASE2-entry truth table)
// by composing binary functions h(a,b) = Arb(f(a,b), g(a,b)).
//
// Seed set:
//   P1       : (a,b) → a           projection 1
//   P2       : (a,b) → b           projection 2
//   C0..C3   : constants           (if constant_basis)
//   G        : (a,b) → Arb(a,b)   the gate itself
//
// Returns gate-depth at which target was found, or -1 if unreachable.
// Short-circuits as soon as any function in `memo` is generated.
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

    auto P1 = [](wire a, wire b) -> wire { return a; };
    auto P2 = [](wire a, wire b) -> wire { return b; };
    auto C0 = [](wire a, wire b) -> wire { return W0; };
    auto C1 = [](wire a, wire b) -> wire { return W1; };
    auto C2 = [](wire a, wire b) -> wire { return W2; };
    auto C3 = [](wire a, wire b) -> wire { return W3; };
    auto G  = [Arb](wire a, wire b) -> wire { return Arb(a, b); };

    // Compose two BASE2-entry truth-tables through Arb: h[i] = Arb(f[i], g[i])
    auto compose_binary = [&](const vector<wire>& f, const vector<wire>& g) {
        vector<wire> h(BASE2);
        for (int i = 0; i < BASE2; i++)
            h[i] = Arb(f[i], g[i]);
        return h;
    };

    // Total binary functions over W^2 = BASE^(BASE^2) — we cap iteration there
    // but practically the closure saturates long before reaching that.
    // We use a generous cap: if S grows beyond BASE^(BASE^2) we've found all.
    // In practice this is never reached; the closure either hits target or not.

    // ── DEBUG mode ───────────────────────────────────────────────────────────
    if (DEBUG) {
        map<vector<wire>, pair<string,int>> S;

        S[get_binary_vector(P1)] = {"a",        0};
        S[get_binary_vector(P2)] = {"b",        0};
        S[get_binary_vector(G)]  = {"Arb(a,b)", 0};
        if (constant_basis) {
            S[get_binary_vector(C0)] = {"c0", 0};
            S[get_binary_vector(C1)] = {"c1", 0};
            S[get_binary_vector(C2)] = {"c2", 0};
            S[get_binary_vector(C3)] = {"c3", 0};
        }

        for (const auto& [v, info] : S)
            if (v == target || memo.count(v)) {
                if (debug_print)
                    printf("TARGET FOUND in seeds!\n  expr: %s  depth: %d\n",
                           info.first.c_str(), info.second);
                return info.second;
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
                    string expr = "Arb(" + i1.first + "," + i2.first + ")";
                    S[h] = {expr, d};
                    changed = true;
                    if (h == target || memo.count(h)) {
                        if (debug_print)
                            printf("TARGET FOUND!\n  expr: %s  depth: %d  funcs: %zu\n",
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
            S[get_binary_vector(C0)] = 0;
            S[get_binary_vector(C1)] = 0;
            S[get_binary_vector(C2)] = 0;
            S[get_binary_vector(C3)] = 0;
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
            S.insert(get_binary_vector(C0));
            S.insert(get_binary_vector(C1));
            S.insert(get_binary_vector(C2));
            S.insert(get_binary_vector(C3));
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
                }
            }
        }
        return -1;
    }

    return -1;
}

// ─── nand_search ──────────────────────────────────────────────────────────────
// Check whether gate Arb can generate the quaternary NAND.
//
// Quaternary NAND: G(a,b) = (BASE-1) - max(a,b)  i.e. 3 - max(a,b)
// Truth table (row-major, a and b in {0,1,2,3}):
//   Row 0: 3 2 1 0
//   Row 1: 2 2 1 0
//   Row 2: 1 1 1 0
//   Row 3: 0 0 0 0
//
template<typename ArbFunc>
bool nand_search(
    ArbFunc Arb,
    ExhaustMode mode          = ExhaustMode::FAST_NO_DEPTH,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis       = false)
{
    vector<wire> target = {
        W3, W2, W1, W0,
        W2, W2, W1, W0,
        W1, W1, W1, W0,
        W0, W0, W0, W0
    };
    return gate_search(Arb, target, mode, memo, constant_basis) > 0;
}

#endif // SEARCH_UTILS_BINARY_H
