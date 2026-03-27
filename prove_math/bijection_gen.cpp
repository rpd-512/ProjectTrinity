#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"

//  Arb(Arb(Arb(x,x), Arb(x, Arb(x,x))), x)

int main() {
    string g_string = "+000-00--";
    auto gate = string_to_gate(g_string);
    print_matrix(gate);
    unary_exhaust(gate, ExhaustMode::DEBUG_MODE, false);
    return 0;
}