#include "essentials.h"
#include "debug_utils.h"
#include "search_utils_unary.h"

using namespace std;

/* ============================================================
   Exhaustive Search Over All 2^4 = 16 Binary Gates
   (Binary port of the ternary unary exhaustive search)
   ============================================================

   For each of the 16 binary gates we ask:
     - Does it generate all 4 unary functions?      (full unary closure)
     - Does it generate both constant unary funcs?  (delta closure)

   In the ternary system the analogous numbers were:
     - 27 unary functions total  → binary: 4
     - 3  delta functions        → binary: 2
   ============================================================ */

// ── Global shim (mirrors the ternary version exactly) ────────────────────────
static array<wire, 4> g_table;

wire g_arb_shim(wire a, wire b) {
    return g_table[(int)a * 2 + (int)b];
}

wire (*make_arb(const array<wire, 4>& table))(wire, wire) {
    g_table = table;
    return g_arb_shim;
}

// ── Exhaustive loop ───────────────────────────────────────────────────────────
void exhaustive_search() {
    const size_t TOTAL = 16;   // 2^4

    size_t full4_count     = 0;  // gates that generate all 4 unary funcs
    size_t fullDelta_count = 0;  // gates that generate both constant funcs

    for (size_t mask = 0; mask < TOTAL; mask++) {
        size_t temp = mask;
        array<wire, 4> table;

        for (int i = 0; i < 4; i++) {
            table[i] = static_cast<wire>(temp % 2);
            temp /= 2;
        }

        // constant_basis = false  (mirrors the ternary main: `false`)
        auto result = unary_exhaust(make_arb(table), ExhaustMode::FAST_MODE, false);

        bool full4   = (result.first  == 4);
        bool fullDelta = (result.second == 2);

        if (full4)     full4_count++;
        if (fullDelta) fullDelta_count++;
    }

    cout << "\n=================================\n";
    cout << "Total gates tested: " << TOTAL << "\n";
    cout << "Generate all 4 unary: " << full4_count   << "\n";
    cout << "Generate both deltas: " << fullDelta_count << "\n";
    cout << "=================================\n";
}

/* ============================================================ */

int main() {
    auto start = chrono::high_resolution_clock::now();
    exhaustive_search();
    auto end = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    double s  = duration.count() / 1000000.0;

    cout << "Time taken: " << duration.count() << " us"
         << " (" << ms << " ms)"
         << " (" << s  << " s)\n";

    return 0;
}