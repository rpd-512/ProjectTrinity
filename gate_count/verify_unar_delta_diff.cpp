#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

static unordered_set<vector<wire>, VectorHash> empty_memo;
static bool constant_basis = true;


static const set<vector<wire>> unary_bijection_coll = {
    {T_NEG, T_ZERO, T_POS},
    {T_NEG, T_POS, T_ZERO},
    {T_ZERO, T_NEG, T_POS},
    {T_ZERO, T_POS, T_NEG},
    {T_POS, T_NEG, T_ZERO},
    {T_POS, T_ZERO, T_NEG}
};

static const set<vector<wire>> unary_delta_coll = {
    {T_POS, T_NEG, T_NEG},
    {T_NEG, T_POS, T_NEG},
    {T_NEG, T_NEG, T_POS}
};

static const wire NAND[9] = {
    T_POS, T_POS,  T_POS,
    T_POS, T_ZERO, T_ZERO,
    T_POS, T_ZERO, T_NEG
};

wire unary_bijection(wire a, int index) {
    auto it = unary_bijection_coll.begin();
    advance(it, index);
    return (*it)[(int)a + 1];
}

wire unary_delta(wire a, int index) {
    auto it = unary_delta_coll.begin();
    advance(it, index);
    return (*it)[(int)a + 1];
}

template<typename Gate>
bool is_unary_complete(Gate Arb){
    vector<wire> v = get_vector(Arb);
    if(empty_memo.count(v))
        return true;
    return unary_exhaust(Arb, ExhaustMode::FAST_MODE, constant_basis).first == 27;
}

template<typename Gate>
bool is_bijection_complete(Gate Arb){
    vector<wire> v = get_vector(Arb);
    if(empty_memo.count(v))
        return true;
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        constant_basis,
        unary_bijection_coll
    ).first == unary_bijection_coll.size();
}

template<typename Gate>
bool is_delta_complete(Gate Arb){
    vector<wire> v = get_vector(Arb);
    if(empty_memo.count(v))
        return true;
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        constant_basis,
        unary_delta_coll
    ).first == unary_delta_coll.size();
}


int main() {
    const size_t TOTAL = 19683;
    size_t count = 0;

    indicators::show_console_cursor(false);
    using namespace indicators;
    BlockProgressBar bar{
        option::BarWidth{40},
        option::ForegroundColor{Color::white},
        option::PrefixText{"Scanning gates "},
        option::ShowElapsedTime{true},
        option::Start{"|"},
        option::End{"|"},
        option::ShowRemainingTime{true},
        option::FontStyles{vector<FontStyle>{FontStyle::bold}},
        option::MaxProgress{TOTAL}
    };

    for (size_t mask = 0; mask < TOTAL; mask++) {
        bar.set_option(option::PostfixText{
            to_string(mask) + "/" + to_string(TOTAL)
            + " - delta+nand!unary: " + to_string(count)
        });
        bar.tick();

        size_t temp = mask;
        array<wire, 9> table;
        for (int i = 0; i < 9; i++) {
            table[i] = static_cast<wire>(temp % 3);
            temp /= 3;
        }

        auto arb = [&table](wire a, wire b) {
            return table[(int)a * 3 + (int)b];
        };

        if (!is_delta_complete(arb))
            continue;

        if (is_unary_complete(arb))
            continue;

        if (nand_search(arb, ExhaustMode::FAST_NO_DEPTH, empty_memo, constant_basis)) {
            count++;
            bar.set_option(option::PostfixText{
                to_string(mask) + "/" + to_string(TOTAL)
                + " - delta+nand!unary: " + to_string(count)
            });
        }
    }

    indicators::show_console_cursor(true);

    printf("\n=================================\n");
    printf("delta+nand complete, NOT unary complete: %zu\n", count);
    printf("=================================\n");
    return 0;
}