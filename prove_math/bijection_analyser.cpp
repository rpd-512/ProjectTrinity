#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <fstream>


vector<vector<wire>> load_gates(const string& filename) {
    ifstream file(filename);
    string line;
    vector<vector<wire>> gates;

    while (getline(file, line)) {
        gates.push_back(get_vector(string_to_gate(line)));
    }
    file.close();
    return gates;
}

pair<int, int> count_bijections_and_half(const vector<wire>& gate) {
    int bijective = 0;
    int half_bijective = 0;

    auto classify = [&](const set<wire>& s) {
        if (s.size() == 3) {
            bijective++;
        } else if (s.size() >= 2) {
            half_bijective++;
        }
    };

    // Rows and columns
    for (int i = 0; i < 3; i++) {
        set<wire> row_set;
        set<wire> col_set;
        for (int j = 0; j < 3; j++) {
            row_set.insert(gate[i * 3 + j]);
            col_set.insert(gate[j * 3 + i]);
        }
        classify(row_set);
        classify(col_set);
    }

    // Main diagonal
    set<wire> diag1;
    for (int i = 0; i < 3; i++) {
        diag1.insert(gate[i * 3 + i]);
    }
    classify(diag1);

    // Anti-diagonal
    set<wire> diag2;
    for (int i = 0; i < 3; i++) {
        diag2.insert(gate[i * 3 + (2 - i)]);
    }
    classify(diag2);

    return {bijective, half_bijective};
}


int main(){
    vector<vector<wire>> all_gates = load_gates("universal_gates_constant_excluded.txt");

    int bijection_count = 0;
    int no_bij = 0;
    int no_hbj = 0;
    for (const auto& gate : all_gates) {
        auto [bijective, half_bijective] = count_bijections_and_half(gate);
        if(bijective == 0) {
            cout << "------------------------------" << endl;
            cout << "No bijective gates found for this gate." << endl;
            cout << "Gate: " << endl;
            auto gate_func = [gate](wire a, wire b) {
                return gate[a * 3 + b];
            };
            print_matrix(gate_func);
            no_bij++;
        }
        if(half_bijective == 0) {
            cout << "------------------------------" << endl;
            cout << "Fewer bijective gates found for this gate." << endl;
            cout << "Gate: " << endl;
            auto gate_func = [gate](wire a, wire b) {
                return gate[a * 3 + b];
            };
            print_matrix(gate_func);
            no_hbj++;
        }
    }

    cout << "Total gates with no bijective gates: " << no_bij << endl;
    cout << "Total gates with no half bijective gates: " << no_hbj << endl;
    return 0;
}