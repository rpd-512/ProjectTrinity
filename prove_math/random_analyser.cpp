#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <fstream>
#include <random>

int main() {
    // Load all gates
    ifstream file("universal_gates_constant_excluded.txt");
    string line;
    vector<vector<wire>> gates;

    while (getline(file, line)) {
        gates.push_back(get_vector(string_to_gate(line)));
    }
    file.close();

    if (gates.empty()) {
        printf("No gates found!\n");
        return 0;
    }

    // True randomness
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, gates.size() - 1);

    int r = dis(gen);
    auto gate_vec = gates[r];

    printf("Randomly selected gate #%d:\n", r);

    // Construct gate function
    auto gate = [gate_vec](wire a, wire b) {
        return gate_vec[a * 3 + b];
    };

    print_matrix(gate);

    // Define SUM gate (target)
    vector<wire> sum_gate = {
        T_POS,  T_NEG,  T_ZERO,
        T_NEG,  T_ZERO, T_POS,
        T_ZERO, T_POS,  T_NEG
    };

    printf("\n--- Unary Exhaust ---\n");
    unary_exhaust(gate, ExhaustMode::DEBUG_MODE, false);

    // printf("\n--- Binary Search (SUM) ---\n");
    // int gc = gate_search(gate, sum_gate, ExhaustMode::DEBUG_MODE, {}, false, true);

    // printf("Gates used: %d\n", gc);

    return 0;
}