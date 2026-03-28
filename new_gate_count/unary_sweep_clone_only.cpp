#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/new_search_utils_unary.h"

int main(){
    auto start = chrono::high_resolution_clock::now();
    int universal_count = 0;

    for (int i = 0; i < 19683; i++) {
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
        if (unary_exhaust(gate, ExhaustMode::FAST_MODE).size() == 27)
            universal_count++;
    }

    auto duration = chrono::duration_cast<chrono::milliseconds>(
        chrono::high_resolution_clock::now() - start
    );
    cout << "Universal gates: " << universal_count << " / 19683" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    return 0;
}