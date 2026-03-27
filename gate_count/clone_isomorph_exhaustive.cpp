#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"

#include <type_traits>
#include <unordered_set>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <fstream>

static unordered_set<vector<wire>, VectorHash> memo_of_completes;
static bool constant_basis = false; // set to false to exclude constant functions from the basis

static const wire unary_bijection_coll[6][3] = {
    {T_NEG, T_ZERO, T_POS},
    {T_NEG, T_POS, T_ZERO},
    {T_ZERO, T_NEG, T_POS},
    {T_ZERO, T_POS, T_NEG},
    {T_POS, T_NEG, T_ZERO},
    {T_POS, T_ZERO, T_NEG}
};

static const set<vector<wire>> unary_bijection_set = {
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
    auto it = unary_bijection_set.begin();
    advance(it, index);
    return (*it)[(int)a];
}

wire unary_delta(wire a, int index) {
    auto it = unary_delta_coll.begin();
    advance(it, index);
    return (*it)[(int)a];
}

bool is_unary_complete(vector<wire> v){
    if(memo_of_completes.count(v))
        return true;
    auto arb = [&v](wire a, wire b) {
            return v[(int)a * 3 + (int)b];
        };
    return unary_exhaust(arb, ExhaustMode::FAST_MODE, constant_basis).first == 27;
}

bool is_bijection_complete(vector<wire> v){
    if(memo_of_completes.count(v))
        return true;
    auto Arb = [v](wire a, wire b){
        return v[a * 3 + b];
    };
    return unary_exhaust(
        Arb,
        ExhaustMode::FAST_MODE,
        constant_basis,
        unary_bijection_set
    ).first == 6;
}

void generate_isomorphs(vector<wire> v){
    for(uint8_t i = 0; i < 6; i++)
    for(uint8_t j = 0; j < 6; j++)
    for(uint8_t k = 0; k < 6; k++){

        vector<wire> iso_vec(9);
        for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++) {
            wire a2 = unary_bijection_coll[i][a];
            wire b2 = unary_bijection_coll[j][b];
            wire val = v[a2 * 3 + b2];
            iso_vec[a * 3 + b] = unary_bijection_coll[k][val];
        }

        if(memo_of_completes.count(iso_vec)) continue;

        bool is_bij = is_bijection_complete(iso_vec);
        if (is_bij)
            memo_of_completes.insert(iso_vec);
    }
}

void exhaustive_search(bool constant_basis) {
    const size_t TOTAL = 19683;
    size_t universal_complete_count = 0;
    indicators::show_console_cursor(false);
    using namespace indicators;
    // BlockProgressBar bar{
    //     option::BarWidth{40},
    //     option::ForegroundColor{Color::white},
    //     option::PrefixText{"Exhaustive Search "},
    //     option::ShowElapsedTime{true},
    //     option::Start{"|"},
    //     option::End{"|"},
    //     option::ShowRemainingTime{true},
    //     option::FontStyles{
    //         vector<FontStyle>{FontStyle::bold}},
    //     option::MaxProgress{TOTAL}
    // };

    for (size_t mask = 0; mask < TOTAL; mask++) {
        //bar.set_option(option::PostfixText{
        //    to_string(mask) + "/" + to_string(TOTAL)
        //    + " - " + to_string(memo_of_completes.size()) + " universal gates found"
        //});
        //bar.tick();
        size_t temp = mask;
        vector<wire> table(9);
        for (int i = 0; i < 9; i++) {
            table[i] = static_cast<wire>(temp % 3);
            temp /= 3;
        }

        if (memo_of_completes.count(table))
            continue;
        
        if (is_unary_complete(table)) {
            generate_isomorphs(table);
            
        }
    }
    indicators::show_console_cursor(true);
    printf("\nTotal universal gates found: %zu\n", memo_of_completes.size());

}

void save_memo(const unordered_set<vector<wire>, VectorHash>& memo, const string& filename) {
    ofstream file(filename);
    auto wire_to_char = [](wire w) {
        switch (w) {
            case T_NEG: return '-';
            case T_ZERO: return '0';
            case T_POS: return '+';
            default: return '?';
        }
    };
    for (const auto& v : memo) {
        for (auto w : v) {
            file << wire_to_char(w);
        }
        file << "\n";
    }
    file.close();
}


int main() {
    if(constant_basis){
        printf("Including constant functions in the basis.\n");
    }
    else{
        printf("Excluding constant functions from the basis.\n");
    }
    auto start = chrono::high_resolution_clock::now();
    exhaustive_search(constant_basis);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    double s  = duration.count() / 1000000.0;

    printf("Time taken: %ld us (%.3f ms) (%.3f s)\n",
           duration.count(), ms, s);


    save_memo(memo_of_completes, "universal_gates_constant_" + string(constant_basis ? "included" : "excluded") + ".txt");
    return 0;
}