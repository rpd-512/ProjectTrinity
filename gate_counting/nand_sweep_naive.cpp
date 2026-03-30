#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>


int main(){
    int universal_count = 0;
    int unary_count = 0;
    int TOTAL = 19683; // 3^9 possible gates

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
        option::MaxProgress{TOTAL}
    };


    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < TOTAL; i++) {
        bar.set_option(option::PostfixText{
            to_string(i) + "/" + to_string(TOTAL) +
            " - " + to_string(unary_count) + " unary-complete, "
            " - " + to_string(universal_count) + " universal gates"
        });
        bar.tick();

        string gate_str = "";
        int tmp = i;
        for (int pos = 0; pos < 9; pos++) {
            switch(tmp % 3) {
                case 0: gate_str += '+'; break;
                case 1: gate_str += '-'; break;
                case 2: gate_str += '0'; break;
            }
            tmp /= 3;
        }
        vector<wire> gate = get_vector(string_to_gate(gate_str));
        if (unary_exhaust(gate, ExhaustMode::FAST_MODE).size() == 27){
            unary_count++;
            if(nand_search(gate, ExhaustMode::FAST_NO_DEPTH, {}))
                universal_count++;
        }
    }

    auto duration = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start
    );
    cout << "Universal gates: " << universal_count << " / 19683" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    return 0;
}