#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"

#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;
using namespace indicators;

// ─────────────────────────────────────────────────────────────
// Convert "+-0" string → vector<wire>
// ─────────────────────────────────────────────────────────────
vector<wire> parse_gate(const string& s) {
    vector<wire> g(9);

    for (int i = 0; i < 9; i++) {
        if (s[i] == '+') g[i] = T_POS;
        else if (s[i] == '-') g[i] = T_NEG;
        else if (s[i] == '0') g[i] = T_ZERO;
        else {
            cerr << "Invalid character in gate: " << s[i] << endl;
            exit(1);
        }
    }

    return g;
}



int main(int argc, char* argv[]) {
    string save_file = "gate_metrics.csv";
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>\n";
        return 1;
    }

    string input_file = argv[1];

    // ── Load already processed gates from CSV ────────────────────
    unordered_set<string> already_done;
    {
        ifstream existing(save_file);
        string row;
        bool header = true;
        while (getline(existing, row)) {
            if (header) { header = false; continue; }
            if (!row.empty())
                already_done.insert(row.substr(0, row.find(',')));
        }
    }
    cout << "Skipping " << already_done.size() << " already processed gates\n";


    // ── First pass: count lines ───────────────────────────────
    ifstream file_count(input_file);
    int total_lines = 0;
    string tmp;
    while (getline(file_count, tmp)) {
        if (!tmp.empty()) total_lines++;
    }
    file_count.close();

    // ── Open actual file ──────────────────────────────────────
    ifstream file(input_file);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << input_file << endl;
        return 1;
    }

    ofstream csv(save_file, ios::app);
    
    // Targets
    vector<wire> sum_gate = {
        T_POS,  T_NEG,  T_ZERO,
        T_NEG,  T_ZERO, T_POS,
        T_ZERO, T_POS,  T_NEG
    };

    vector<wire> carry_gate = {
        T_NEG,  T_ZERO,  T_ZERO,
        T_ZERO, T_ZERO,  T_ZERO,
        T_ZERO, T_ZERO,  T_POS
    };

    // ── Progress bar ──────────────────────────────────────────
    show_console_cursor(false);

    BlockProgressBar bar{
        option::BarWidth{40},
        option::ForegroundColor{Color::white},
        option::PrefixText{"Processing Gates"},
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::Start{"|"},
        option::End{"|"},
        option::MaxProgress{total_lines}
    };

    string line;
    int count = 0;

    auto start = chrono::high_resolution_clock::now();

    // ─────────────────────────────────────────────────────────
    // Main loop
    // ─────────────────────────────────────────────────────────
    while (getline(file, line)) {

        if (line.empty()) continue;
        if (line.size() != 9) continue;

        if (already_done.count(line)) {
            count++;
            bar.tick();
            continue;
        }

        vector<wire> gate = parse_gate(line);
        //cout << "Processing gate: " << line << endl;
        auto sum_res = gate_search(gate, sum_gate, ExhaustMode::FAST_MODE);
        //cout << "  Sum:   " << sum_res.first << " gates, depth " << sum_res.second << endl;
        auto carry_res = gate_search(gate, carry_gate, ExhaustMode::FAST_MODE);
        //cout << "  Carry: " << carry_res.first << " gates, depth " << carry_res.second << endl;
        csv << line << ","
            << sum_res.first   << "," << sum_res.second << ","
            << carry_res.first << "," << carry_res.second
            << "\n";
        csv.flush();
        count++;

        bar.set_option(option::PostfixText{
            to_string(count) + "/" + to_string(total_lines)
        });

        bar.tick();
    }

    show_console_cursor(true);

    file.close();
    csv.close();

    auto duration = chrono::duration_cast<chrono::seconds>(
        chrono::high_resolution_clock::now() - start
    );

    cout << "\nDone.\n";
    cout << "Processed " << count << " gates\n";
    cout << "Time: " << duration.count() << " seconds\n";

    return 0;
}