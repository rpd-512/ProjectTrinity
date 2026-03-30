#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include <fstream>

static unordered_set<vector<wire>, VectorHash> memo_of_completes;
static unordered_set<vector<wire>, VectorHash> memo_of_incompletes;

ofstream out("universal_gates.txt");

static const vector<vector<wire>> unary_bijection = {
    {T_NEG, T_ZERO, T_POS},
    {T_NEG, T_POS, T_ZERO},
    {T_ZERO, T_NEG, T_POS},
    {T_ZERO, T_POS, T_NEG},
    {T_POS, T_NEG, T_ZERO},
    {T_POS, T_ZERO, T_NEG}
};

vector<wire> transpose_inputs(vector<wire> gate){
    vector<wire> transposed(9);
    for(int a = 0; a < 3; a++)
        for(int b = 0; b < 3; b++)
            transposed[3*a + b] = gate[3*b + a];
    return transposed;
}

static const int inv_perm[6] = {0, 1, 2, 4, 3, 5};

void generate_isomorphs(vector<wire> Arb, bool is_complete){
    auto run = [&](vector<wire> gate){
        for(int k = 0; k < 6; k++){
            auto iso_func = [gate, k](wire a, wire b){
                wire a_mapped = unary_bijection[inv_perm[k]][a];
                wire b_mapped = unary_bijection[inv_perm[k]][b];
                return unary_bijection[k][gate[3*a_mapped + b_mapped]];
            };
            vector<wire> iso_vec = get_vector(iso_func);
            if(memo_of_completes.count(iso_vec)) continue;
            if(memo_of_incompletes.count(iso_vec)) continue;
            if(is_complete) {
                memo_of_completes.insert(iso_vec);
            }
            else {
                memo_of_incompletes.insert(iso_vec);
            }
        }
    };
    run(Arb);
    run(transpose_inputs(Arb)); // you already have this function
}

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
        //bar.set_option(option::PostfixText{
        //    to_string(i) + "/" + to_string(TOTAL) +
        //    " - " + to_string(unary_count) + " unary-complete, "
        //    " - " + to_string(universal_count) + " universal gates"
        //});
        //bar.tick();
        universal_count = memo_of_completes.size();
        string gate_str = "";
        vector <wire> gate;
        int tmp = i;
        for (int pos = 0; pos < 9; pos++) {
            switch(tmp % 3) {
                case 0: gate.push_back(T_POS); break;
                case 1: gate.push_back(T_ZERO); break;
                case 2: gate.push_back(T_NEG); break;
            }
            tmp /= 3;
        }
        
        if(memo_of_completes.count(gate)) continue;
        if(memo_of_incompletes.count(gate)) continue;


        // ... inside the loop, replace the unary_exhaust block:
        if (unary_exhaust(gate, ExhaustMode::FAST_MODE).size() == 27){
            unary_count++;
            generate_isomorphs(gate, true);
        }
        else {
            generate_isomorphs(gate, false);
        }
    }

    auto duration = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start
    );
    cout << "Universal gates: " << universal_count << " / 19683" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    cout << "Total NAND checks: " << nand_checks << endl;
    cout << "Total Unary checks: " << unary_checks << endl;
    
    // Output the universal gates to a file
    out << "Universal gates:\n";
    for (const auto& gate : memo_of_completes) {
        string gate_str = "";
        for (auto w : gate) {
            switch(w) {
                case T_NEG: gate_str += '-'; break;
                case T_ZERO: gate_str += '0'; break;
                case T_POS: gate_str += '+'; break;
            }
        }
        out << gate_str << "\n";  // save the canonical gate
    }
    out.flush();
    
        
    return 0;
}