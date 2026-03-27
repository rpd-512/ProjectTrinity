//942897552 total universals

#include "essentials.h"
#include "debug_utils.h"
#include "search_utils_unary.h"

#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <string>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
using namespace std;

/* ============================================================
   Exhaustive Search Over All 4^16 = 4,294,967,296 Quaternary Gates
   With memoization + isomorphism pruning (mirrors main.cpp style).
   ============================================================ */

// ── Global memo ───────────────────────────────────────────────────────────────
static unordered_set<vector<wire>, VectorHash> memo_of_completes;

// ── Bijection collection (all 4! = 24 permutations of {W0,W1,W2,W3}) ─────────
static vector<vector<wire>> unary_bijection_vec;
static set<vector<wire>>    unary_bijection_coll;

void init_bijections() {
    vector<wire> perm = { W0, W1, W2, W3 };
    do {
        unary_bijection_vec.push_back(perm);
        unary_bijection_coll.insert(perm);
    } while (next_permutation(perm.begin(), perm.end()));
    // unary_bijection_vec.size() == 24
}

wire unary_bijection(wire a, int index) {
    return unary_bijection_vec[index][(int)a];
}

// ── Global shim ───────────────────────────────────────────────────────────────
static array<wire, BASE2> g_table;

wire g_arb_shim(wire a, wire b) {
    return g_table[(int)a * BASE + (int)b];
}

wire (*make_arb(const array<wire, BASE2>& table))(wire, wire) {
    g_table = table;
    return g_arb_shim;
}

// ── Completeness predicates ───────────────────────────────────────────────────

// Full unary completeness: generates all 256 unary functions.
template<typename Gate>
bool is_unary_complete(Gate Arb) {
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(Arb, ExhaustMode::FAST_MODE, false).first
           == (size_t)UNARY_COUNT;
}

// Delta completeness: generates all 4 constant unary functions.
template<typename Gate>
bool is_delta_complete(Gate Arb) {
    static const set<vector<wire>> delta_coll = {
        { W0, W0, W0, W0 },
        { W1, W1, W1, W1 },
        { W2, W2, W2, W2 },
        { W3, W3, W3, W3 }
    };
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        false,
        delta_coll
    ).first == delta_coll.size();
}

// ── Isomorph generation ───────────────────────────────────────────────────────
// For a confirmed gate, insert all (4!)^3 = 13,824 isomorphs into the memo.
template<typename Gate>
void generate_isomorphs(Gate Arb) {
    const int NUM_BIJ = (int)unary_bijection_vec.size(); // 24

    for (int i = 0; i < NUM_BIJ; i++)
    for (int j = 0; j < NUM_BIJ; j++)
    for (int k = 0; k < NUM_BIJ; k++) {
        auto iso_func = [&](wire a, wire b) -> wire {
            a = unary_bijection(a, i);
            b = unary_bijection(b, j);
            return unary_bijection(Arb(a, b), k);
        };
        memo_of_completes.insert(get_binary_vector(iso_func));
    }
}

// ── Exhaustive loop ───────────────────────────────────────────────────────────
void exhaustive_search() {
    const long long TOTAL = 4294967296LL;  // 4^16

    long long full256_count   = 0;
    long long fullDelta_count = 0;

    indicators::show_console_cursor(false);
    using namespace indicators;
    BlockProgressBar bar{
        option::BarWidth{40},
        option::ForegroundColor{Color::white},
        option::PrefixText{"Exhaustive Search "},
        option::ShowElapsedTime{true},
        option::Start{"|"},
        option::End{"|"},
        option::ShowRemainingTime{true},
        option::FontStyles{
            vector<FontStyle>{FontStyle::bold}},
        option::MaxProgress{TOTAL}
    };

    for (long long mask = 0; mask < TOTAL; mask++) {
        bar.set_option(option::PostfixText{
            to_string(mask) + "/" + to_string(TOTAL)
            + " - " + to_string(full256_count)   + " full-256"
            + " / " + to_string(fullDelta_count)  + " delta found"
        });
        bar.tick();

        long long temp = mask;
        array<wire, BASE2> table;
        for (int i = 0; i < BASE2; i++) {
            table[i] = static_cast<wire>(temp % BASE);
            temp /= BASE;
        }

        auto arb = make_arb(table);
        vector<wire> arb_vec = get_binary_vector(arb);

        // ── Memo hit: both counts already credited when isomorphs were generated
        if (memo_of_completes.count(arb_vec)) {
            // We know it's full-256 (only full-256 gates seed the memo).
            // Delta status was recorded at generation time; re-check cheaply.
            full256_count++;
            if (is_delta_complete(arb)) fullDelta_count++;
            continue;
        }

        // ── Fresh gate: run the full unary exhaust
        auto result = unary_exhaust(arb, ExhaustMode::FAST_MODE, false);

        bool full256   = (result.first  == (size_t)UNARY_COUNT);
        bool fullDelta = (result.second == (size_t)DELTA_COUNT);

        if (full256) {
            full256_count++;
            memo_of_completes.insert(arb_vec);   // seed memo with this gate …
            generate_isomorphs(arb);              // … and all 13,824 isomorphs
        }
        if (fullDelta) fullDelta_count++;
    }

    indicators::show_console_cursor(true);
    cout << "\n=================================\n";
    cout << "Total gates tested : " << TOTAL          << "\n";
    cout << "Generate all 256   : " << full256_count  << "\n";
    cout << "Generate all 4 Δ   : " << fullDelta_count << "\n";
    cout << "=================================\n";
}

/* ============================================================ */

int main() {
    init_bijections();

    printf("Quaternary (base-4) unary exhaustive search\n");
    printf("Bijections (4!):        %zu\n",   unary_bijection_vec.size());
    printf("Isomorphs per gate:     %zu\n\n",
           unary_bijection_vec.size() *
           unary_bijection_vec.size() *
           unary_bijection_vec.size());

    auto start = chrono::high_resolution_clock::now();
    exhaustive_search();
    auto end   = chrono::high_resolution_clock::now();

    auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
    printf("Time: %lld µs  (%.3f ms)  (%.3f s)\n",
           (long long)us, us / 1000.0, us / 1000000.0);

    return 0;
}