//write a proram to take in string and output the formula for generating an arbitrary gate

#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include "../includes/optimal_search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

vector<wire> decode9(int code)
{
    vector<wire> tt(9);
    for (int i = 0; i < 9; i++) {
        tt[i] = code % 3;
        code /= 3;
    }
    return tt;
}


int main(){

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

    //auto G = string_to_gate(gate_str);
    //auto Cir = [G](wire x, wire y){
    //    return G(G(x, G(y, x)), G(G(G(G(y, x), y), x), y));
    //};
    //print_matrix(get_vector(Cir));
    //vector<wire> Arb = decode9(451);
    vector<wire> Arb = get_vector(string_to_gate("++++-+++0"));
    print_matrix(Arb);

    auto s2 = chrono::high_resolution_clock::now();
    auto [sum_gates, sum_depth] = gate_search(Arb, SumTgt, ExhaustMode::FAST_MODE,{}, true);
    cout << "===== SUM TARGET =====\n";
    cout << "Gates used: " << sum_gates << endl;
    cout << "Depth: " << sum_depth << endl;
    auto [carry_gates, carry_depth] = gate_search(Arb, CarryTgt, ExhaustMode::FAST_MODE,{}, true);
    cout << "===== CARRY TARGET =====\n";
    cout << "Gates used: " << carry_gates << endl;
    cout << "Depth: " << carry_depth << endl;
    auto e2 = chrono::high_resolution_clock::now();
    auto duration2 = chrono::duration_cast<chrono::microseconds>(e2 - s2);
    cout << "Execution time: " << duration2.count() << " microseconds\n";

    
    auto s1 = chrono::high_resolution_clock::now();
    auto [g1, d1] = gate_search_unified(Arb, SumTgt, OptMode::DEPTH_OPTIMAL);
    auto [g2, d2] = gate_search_unified(Arb, CarryTgt, OptMode::SIZE_OPTIMAL);
    cout << "===== SUM TARGET =====\n";
    cout << "Gates used: " << g1 << endl;
    cout << "Depth: " << d1 << endl;
    cout << "===== CARRY TARGET =====\n";
    cout << "Gates used: " << g2 << endl;
    cout << "Depth: " << d2 << endl;
    auto e1 = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(e1 - s1);
    cout << "Execution time: " << duration.count() << " microseconds\n";



    return 0;
}