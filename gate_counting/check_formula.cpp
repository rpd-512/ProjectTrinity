//write a proram to take in string and output the formula for generating an arbitrary gate

#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>


int main(){
    string gate_str="0-+0+0---";
    auto G = string_to_gate(gate_str);
    vector<wire> SumTgt = {
        T_POS, T_NEG,  T_ZERO,
        T_NEG, T_ZERO, T_POS,
        T_ZERO, T_POS, T_NEG
    };
    vector<wire> CarryTgt = {
        T_NEG, T_ZERO,  T_ZERO,
        T_ZERO, T_ZERO,  T_ZERO,
        T_ZERO, T_ZERO,  T_POS
    };

    auto Cir = [G](wire x, wire y){
        return G(G(x, G(y, x)), G(G(G(G(y, x), y), x), y));
    };
    print_matrix(get_vector(Cir));
    vector<wire> sum_gate = get_vector(string_to_gate(gate_str));
    vector<wire> carry_gate = get_vector(string_to_gate(gate_str));
    auto [sum_gates, sum_depth] = gate_search(sum_gate, SumTgt, ExhaustMode::FAST_MODE,{}, true);
    cout << "===== SUM TARGET =====\n";
    cout << "Gates used: " << sum_gates << endl;
    cout << "Depth: " << sum_depth << endl;
    auto [carry_gates, carry_depth] = gate_search(carry_gate, CarryTgt, ExhaustMode::FAST_MODE,{}, true);
    cout << "===== CARRY TARGET =====\n";
    cout << "Gates used: " << carry_gates << endl;
    cout << "Depth: " << carry_depth << endl;
    return 0;
}