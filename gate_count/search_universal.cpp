#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>


using namespace std;

/* ============================================================
   Config: set true to print all universally complete gates,
           set false to only print the final counts.
   ============================================================ */
static const bool PRINT_UNIVERSAL_GATES = false;

/* ============================================================
   Global shim so we can pass a function pointer into the
   existing unary_exhaust / nand_search interfaces.
   ============================================================ */
static array<wire, 9> g_table;

wire g_arb_shim(wire a, wire b) {
    return g_table[a * 3 + b];
}

wire (*make_arb(const array<wire, 9>& table))(wire, wire) {
    g_table = table;
    return g_arb_shim;
}

/* ============================================================
   Main search
   ============================================================ */
void exhaustive_search() {
    const size_t TOTAL = 19683;

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

    size_t unary_complete_count    = 0;
    size_t universal_complete_count = 0;

    for (size_t mask = 0; mask < TOTAL; mask++) {
        bar.set_option(option::PostfixText{
            to_string(mask) + "/" + to_string(TOTAL)
            + " - " + to_string(universal_complete_count) + " universal gates found"
        });
        bar.tick();

        /* Build the truth-table for this gate */
        size_t temp = mask;
        array<wire, 9> table;
        for (int i = 0; i < 9; i++) {
            table[i] = static_cast<wire>(temp % 3);
            temp /= 3;
        }

        auto arb = make_arb(table);

        /* ── Step 1: is it unary-complete? ── */
        auto result   = unary_exhaust(arb, ExhaustMode::FAST_MODE, false);
        bool full27   = (result.first  == 27);
        bool fullDelta = (result.second == 3);
        bool unary_complete = full27 && fullDelta;

        if (!unary_complete) continue;
        unary_complete_count++;

        /* ── Step 2: can it construct NAND? ── */
        bool can_nand = nand_search(arb, ExhaustMode::FAST_MODE, {}, false);

        if (can_nand) {
            universal_complete_count++;

            if (PRINT_UNIVERSAL_GATES) {
                printf("\n──────────────────────────────────\n");
                printf("Universal gate #%zu  (mask %zu)\n",
                       universal_complete_count, mask);
                printf("Truth table matrix (rows = a, cols = b):\n");
                print_matrix(arb);
            }
        }
    }

    indicators::show_console_cursor(true);

    printf("\n=================================\n");
    printf("Total gates tested       : %zu\n", TOTAL);
    printf("Unary-complete           : %zu\n", unary_complete_count);
    printf("Universal (+ NAND)       : %zu\n", universal_complete_count);
    printf("=================================\n");
}

/* ============================================================ */
int main() {
    auto start = chrono::high_resolution_clock::now();

    exhaustive_search();

    auto end      = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    double s  = duration.count() / 1000000.0;

    printf("Time taken: %ld us (%.3f ms) (%.3f s)\n",
           duration.count(), ms, s);

    return 0;
}