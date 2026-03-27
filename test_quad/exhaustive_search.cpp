#include "essentials.h"
#include "debug_utils.h"
#include "search_utils_unary.h"
#include "search_utils_binary.h"

#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <string>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

// ─── Global memo ─────────────────────────────────────────────────────────────
static unordered_set<vector<wire>, VectorHash> memo_of_completes;

static bool constant_basis = false;

// ─── Bijection collection ─────────────────────────────────────────────────────
// All 4! = 24 permutations of {W0,W1,W2,W3}, generated programmatically
// and stored as a vector for O(1) indexed access (avoids the UB-prone
// std::advance-on-set-iterator pattern from the original ternary code).
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

// ─── Completeness predicates ─────────────────────────────────────────────────

// Full unary completeness: generates all 256 unary functions.
template<typename Gate>
bool is_unary_complete(Gate Arb) {
    vector<wire> v = get_binary_vector(Arb);
    if (memo_of_completes.count(v)) return true;
    return unary_exhaust(Arb, ExhaustMode::FAST_MODE, constant_basis).first
           == (size_t)UNARY_COUNT;
}

// Bijection completeness: generates all 24 bijections of {0,1,2,3}.
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
        constant_basis,
        delta_coll
    ).first == delta_coll.size();
}

// ─── Isomorph generation ──────────────────────────────────────────────────────
// For a confirmed universal gate, insert all (4!)^3 = 13824 isomorphs.
template<typename Gate>
void generate_isomorphs(Gate Arb) {
    const int NUM_BIJ = (int)unary_bijection_vec.size(); // 24

    for (int i = 0; i < NUM_BIJ; i++)
    for (int j = 0; j < NUM_BIJ; j++)
    for (int k = 0; k < NUM_BIJ; k++) {
        auto iso_func = [Arb, i, j, k](wire a, wire b) -> wire {
            a = unary_bijection(a, i);
            b = unary_bijection(b, j);
            return unary_bijection(Arb(a, b), k);
        };
        vector<wire> iso_vec = get_binary_vector(iso_func);

        bool is_bij = is_bijection_complete(iso_func);
        if (is_bij)
            memo_of_completes.insert(iso_vec);
    }
}

// ─── Exhaustive search ────────────────────────────────────────────────────────
// Iterate over all 4^16 = 4,294,967,296 quaternary gates.
// For each gate not already in the memo:
//   1. Quick unary-completeness filter.
//   2. Quaternary NAND reachability check.
//   3. If both pass, generate all 13824 isomorphs and add to memo.
void exhaustive_search(bool constant_basis) {
    const long long TOTAL = 4294967296LL;  // 4^16

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
            + " - " + to_string(memo_of_completes.size()) + " universal gates found"
        });
        bar.tick();

        long long temp = mask;
        array<wire, BASE2> table;
        for (int i = 0; i < BASE2; i++) {
            table[i] = static_cast<wire>(temp % BASE);
            temp /= BASE;
        }

        auto arb = [&table](wire a, wire b) -> wire {
            return table[(int)a * BASE + (int)b];
        };

        vector<wire> arb_vec = get_binary_vector(arb);
        if (memo_of_completes.count(arb_vec))
            continue;

        if (is_unary_complete(arb)) {
            if (nand_search(arb, ExhaustMode::FAST_NO_DEPTH,
                            memo_of_completes, constant_basis)) {
                generate_isomorphs(arb);
            }
        }
    }

    indicators::show_console_cursor(true);
    printf("\nTotal universal quaternary gates found: %zu\n",
           memo_of_completes.size());
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
    init_bijections();
    printf("Quaternary (base-4) logic exhaustive search\n");
    printf("Constant basis: %s\n\n",
           constant_basis ? "included" : "excluded");
    printf("Bijections (4!): %zu\n", unary_bijection_vec.size());
    printf("Isomorphs per gate (4!^3): %zu\n\n",
           unary_bijection_vec.size() *
           unary_bijection_vec.size() *
           unary_bijection_vec.size());

    auto start = chrono::high_resolution_clock::now();
    exhaustive_search(constant_basis);
    auto end   = chrono::high_resolution_clock::now();

    auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
    printf("Time: %lld µs  (%.3f ms)\n", (long long)us, us / 1000.0);

    string fname = "universal_gates_quaternary_constant_"
                 + string(constant_basis ? "included" : "excluded")
                 + ".txt";
    save_memo(memo_of_completes, fname);
    printf("Results saved to: %s\n", fname.c_str());

    return 0;
}
