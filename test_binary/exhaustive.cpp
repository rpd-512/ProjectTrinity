#include "essentials.h"
#include "debug_utils.h"
#include "search_utils_unary.h"
#include "search_utils_binary.h"

#include <type_traits>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <string>

// ─── Global memo ─────────────────────────────────────────────────────────────
// Stores truth-table vectors of all known universal gates.
// Any gate that can generate a known universal gate is itself universal
// (transitivity short-circuit).
static unordered_set<vector<wire>, VectorHash> memo_of_completes;

// Set to true to include constant binary functions (always-F, always-T)
// as free seeds in the composition closure.
static bool constant_basis = false;

// ─── Bijection collection ─────────────────────────────────────────────────────
// In binary, the only bijections (permutations) of {F,T} are:
//   identity : [F, T]  (index 0)
//   NOT      : [T, F]  (index 1)
//
// Stored as a vector so we can index directly (avoids UB from advancing a
// set iterator by a computed offset, which triggered warnings in the ternary
// version when compiled with aggressive loop optimisations).
static const vector<vector<wire>> unary_bijection_vec = {
    { T_FALSE, T_TRUE  },   // identity
    { T_TRUE,  T_FALSE }    // NOT
};

// For the completeness check we also need a set<vector<wire>> form.
static const set<vector<wire>> unary_bijection_coll(
    unary_bijection_vec.begin(), unary_bijection_vec.end());

// ─── Apply a bijection by index to a wire value ───────────────────────────────
wire unary_bijection(wire a, int index) {
    return unary_bijection_vec[index][(int)a];
}

// ─── Completeness predicates ─────────────────────────────────────────────────

// Full unary completeness: can generate all 4 unary functions.
template<typename Gate>
bool is_unary_complete(Gate Arb) {
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(Arb, ExhaustMode::FAST_MODE, constant_basis).first == 4;
}

// Bijection completeness: can generate both bijections (identity + NOT).
template<typename Gate>
bool is_bijection_complete(Gate Arb) {
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        constant_basis,
        unary_bijection_coll
    ).first == unary_bijection_coll.size();
}

// Delta completeness: can generate both constant unary functions.
template<typename Gate>
bool is_delta_complete(Gate Arb) {
    // In binary, deltas are the two constant unary funcs: always-F, always-T.
    static const set<vector<wire>> delta_coll = {
        { T_FALSE, T_FALSE },   // always F
        { T_TRUE,  T_TRUE  }    // always T
    };
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        constant_basis,
        delta_coll
    ).first == delta_coll.size();
}

// ─── Isomorph generation ──────────────────────────────────────────────────────
// For a gate known to be universal, generate all gates isomorphic to it under
// re-labelling of inputs and output by bijections.
// In binary: 2 bijections × 2 bijections × 2 bijections = 8 isomorphs.
template<typename Gate>
void generate_isomorphs(Gate Arb) {
    const int NUM_BIJ = (int)unary_bijection_coll.size(); // 2

    for (int i = 0; i < NUM_BIJ; i++)
    for (int j = 0; j < NUM_BIJ; j++)
    for (int k = 0; k < NUM_BIJ; k++) {
        auto iso_func = [Arb, i, j, k](wire a, wire b) -> wire {
            a = unary_bijection(a, i);
            b = unary_bijection(b, j);
            return unary_bijection(Arb(a, b), k);
        };
        vector<wire> iso_vec = get_binary_vector(iso_func);

        // Use the same completeness check as the main loop.
        // Here we check bijection completeness (mirrors r1 in the ternary code).
        bool is_bij = is_bijection_complete(iso_func);
        if (is_bij)
            memo_of_completes.insert(iso_vec);
    }
}

// ─── Exhaustive search ────────────────────────────────────────────────────────
// Iterate over all 2^4 = 16 binary gates.
// For each gate not already in the memo:
//   1. Quick unary-completeness filter.
//   2. NAND reachability check (the binary universal gate).
//   3. If both pass, generate all isomorphs and add to memo.
void exhaustive_search(bool constant_basis) {
    const size_t TOTAL = 16;   // 2^4 binary gates
    size_t new_found = 0;

    for (size_t mask = 0; mask < TOTAL; mask++) {
        size_t temp = mask;
        array<wire, 4> table;
        for (int i = 0; i < 4; i++) {
            table[i] = static_cast<wire>(temp % 2);
            temp /= 2;
        }

        auto arb = [&table](wire a, wire b) -> wire {
            return table[(int)a * 2 + (int)b];
        };

        vector<wire> arb_vec = get_binary_vector(arb);
        if (memo_of_completes.count(arb_vec))
            continue;

        // Filter: must generate all 4 unary functions
        if (is_unary_complete(arb)) {
            // Confirm by checking NAND reachability
            if (nand_search(arb, ExhaustMode::FAST_NO_DEPTH, memo_of_completes, constant_basis)) {
                generate_isomorphs(arb);
                new_found++;
            }
        }
    }

    printf("\nTotal universal binary gates found: %zu\n", memo_of_completes.size());
}

// ─── Save results ─────────────────────────────────────────────────────────────
void save_memo(const unordered_set<vector<wire>, VectorHash>& memo,
               const string& filename)
{
    ofstream file(filename);
    for (const auto& v : memo) {
        for (auto w : v)
            file << (char)wire_to_char(w);
        file << "\n";
    }
    file.close();
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main() {
    printf("Binary logic exhaustive search\n");
    printf("Constant basis: %s\n\n",
           constant_basis ? "included" : "excluded");

    auto start = chrono::high_resolution_clock::now();
    exhaustive_search(constant_basis);
    auto end   = chrono::high_resolution_clock::now();

    auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
    printf("Time: %ld µs  (%.3f ms)\n", us, us / 1000.0);

    string fname = "universal_gates_binary_constant_"
                 + string(constant_basis ? "included" : "excluded")
                 + ".txt";
    save_memo(memo_of_completes, fname);
    printf("Results saved to: %s\n", fname.c_str());

    return 0;
}