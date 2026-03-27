#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_binary.h"

using namespace std;
using namespace std::chrono;

wire Arb(wire a, wire b){
    static const wire arbMatrix[3][3] = {
        T_NEG, T_ZERO, T_ZERO,
        T_ZERO,  T_POS,  T_POS,
        T_ZERO,  T_POS,  T_NEG
    };
    return arbMatrix[a][b];
}

int main()
{
    cout << "\n========== TERNARY NAND SEARCH ==========\n";

    print_matrix(Arb);

    bool found = nand_search(
        Arb,
        ExhaustMode::DEBUG_MODE   // change to FAST_MODE if needed
    );

    if(found)
        cout << "\nResult: NAND is constructible.\n";
    else
        cout << "\nResult: NAND cannot be constructed.\n";

    cout << "=========================================\n";

    return 0;
}