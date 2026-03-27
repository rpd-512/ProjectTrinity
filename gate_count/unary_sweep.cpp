#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"

using namespace std;

/* ============================================================
   Exhaustive Search Over All 3^9 Binary Gates
   ============================================================ */

static array<wire, 9> g_table;

wire g_arb_shim(wire a, wire b) {
    return g_table[a * 3 + b];
}

wire (*make_arb(const array<wire, 9>& table))(wire, wire) {
    g_table = table;
    return g_arb_shim;
}

void exhaustive_search() {
    const size_t TOTAL = 19683;

    size_t full27_count = 0;
    size_t fullDelta_count = 0;

    for (size_t mask = 0; mask < TOTAL; mask++) {

        size_t temp = mask;
        array<wire,9> table;

        for (int i = 0; i < 9; i++) {
            table[i] = static_cast<wire>(temp % 3);
            temp /= 3;
        }
        auto result = unary_exhaust(make_arb(table), ExhaustMode::FAST_MODE, false);

        bool full27   = (result.first  == 27);
        bool fullDelta = (result.second == 3);

        if (full27)      full27_count++;
        if (fullDelta)   fullDelta_count++;
        //if(full27){
        //    printf("Found gate with both properties! Mask: %zu\n", mask);
        //    print_matrix(make_arb(table));
        //    printf("\n");
        //}
    }

    cout << "\n=================================\n";
    cout << "Total gates tested: 19683\n";
    cout << "Generate all 27 unary: " << full27_count << "\n";
    cout << "Generate all 3 deltas: " << fullDelta_count << "\n";
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