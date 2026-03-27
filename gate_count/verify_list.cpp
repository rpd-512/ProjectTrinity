#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <fstream>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

int main(){
    ifstream file("universal_gates_constant_included.txt");
    string line;
    vector<vector<wire>> gates;
    while (getline(file, line)) {
        gates.push_back(get_vector(string_to_gate(line)));
    }
    file.close();
    int u_c = 0, b_c = 0;
    indicators::show_console_cursor(false);
    using namespace indicators;
    BlockProgressBar bar{
        option::BarWidth{40},
        option::ForegroundColor{Color::white},
        option::PrefixText{"Check Universals"},
        option::ShowElapsedTime{true},
        option::Start{"|"},
        option::End{"|"},
        option::ShowRemainingTime{true},
        option::FontStyles{
            vector<FontStyle>{FontStyle::bold}},
        option::MaxProgress{gates.size()}
    };

    int c = 0;
    for(const auto& gate_vec : gates){
        bar.set_option(option::PostfixText{
            to_string(c++) + "/" + to_string(gates.size()) +
            " - " + to_string(u_c) + " unary-complete, "
            + to_string(b_c) + " universal"
        });
        bar.tick();

        auto arb = [&gate_vec](wire a, wire b) {
            return gate_vec[a * 3 + b];
        };
        if(unary_exhaust(arb, ExhaustMode::FAST_MODE).first == 27)
            u_c++;

        if(nand_search(arb, ExhaustMode::FAST_NO_DEPTH, {}, true))
            b_c++;
    }
    printf("\nSummary:\n");
    printf("Total gates tested: %zu\n", gates.size());
    printf("Unary-complete: %d\n", u_c);
    printf("Universal (+ NAND): %d\n", b_c);
    return 0;
}