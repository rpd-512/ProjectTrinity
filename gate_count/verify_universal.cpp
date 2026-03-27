#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"

wire Arb(wire a, wire b){
    static const wire arbMatrix[3][3] = {
        T_ZERO, T_ZERO, T_NEG,
        T_NEG,  T_POS,  T_POS,
        T_POS,  T_POS,  T_POS
    };
    return arbMatrix[a][b];
}


int main() {
    string g_string = "+000---+-";
    auto arb = string_to_gate(g_string);
    vector<wire> carry_gate = {
        T_NEG, T_ZERO, T_ZERO,
        T_ZERO, T_ZERO, T_ZERO,
        T_ZERO, T_ZERO, T_POS
    };
    vector<wire> sum_gate = {
        T_POS, T_NEG,  T_ZERO,
        T_NEG, T_ZERO, T_POS,
        T_ZERO, T_POS, T_NEG
    };

    printf("Testing gate:\n");
    unary_exhaust(arb, ExhaustMode::DEBUG_MODE, false);
    int s_gc = gate_search(arb, sum_gate, ExhaustMode::DEBUG_MODE, {}, false, true);
    int c_gc = gate_search(arb, carry_gate, ExhaustMode::DEBUG_MODE, {}, false, true);
    printf("Sum gates used: %d\n", s_gc);
    printf("Carry gates used: %d\n", c_gc);

}