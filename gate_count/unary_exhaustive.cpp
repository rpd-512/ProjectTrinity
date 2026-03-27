#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"

using namespace std;
using namespace std::chrono;

wire Arb(wire a, wire b){
    static const wire arbMatrix[3][3] = {
        T_ZERO, T_ZERO, T_NEG,
        T_NEG,  T_POS,  T_POS,
        T_POS,  T_POS,  T_POS
    };
    return arbMatrix[a][b];
}

int main(){
    auto result = unary_exhaust(Arb, ExhaustMode::DEBUG_MODE);
    printf("Generated %lu out of 27 unary functions\n", result.first);
    printf("Delta count: %ld\n", result.second);

    return 0;
}