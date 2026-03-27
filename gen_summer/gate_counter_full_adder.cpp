#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"

#include <fstream>
#include <csignal>
#include <atomic>
#include <thread>
#include <future>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>


// ── Globals for signal handling ──────────────────────────────────────────────
static std::atomic<bool> g_interrupted{false};

void handle_sigint(int) {
    g_interrupted.store(true);
}

// Run gate_search in a background thread; returns -1 if interrupted mid-search.
int interruptible_gate_search(auto Arb, const vector<wire>& target,
                              ExhaustMode mode) {
    std::promise<int> promise;
    auto future = promise.get_future();

    std::thread worker([&]() {
        promise.set_value(
            gate_search(Arb, target, mode, {}, false, true)
        );
    });

    // Poll until the worker finishes or we get a signal.
    while (future.wait_for(std::chrono::milliseconds(100)) ==
           std::future_status::timeout) {
        if (g_interrupted.load()) {
            // Detach — the thread will finish on its own after main exits.
            worker.detach();
            return -1;
        }
    }

    worker.join();
    return future.get();
}

// ── File helpers ──────────────────────────────────────────────────────────────
vector<string> load_gates(const string& filename) {
    ifstream file(filename);
    string line;
    vector<string> gates;
    while (getline(file, line))
        gates.push_back(line);
    file.close();
    return gates;
}

// Checkpoint format: one "count gate_string" line per completed entry.
// We load whatever is already in gate_counts.txt so we can skip those gates.
unordered_map<string, int> load_checkpoint(const string& filename) {
    unordered_map<string, int> done;
    ifstream f(filename);
    if (!f.is_open()) return done;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        auto space = line.find(' ');
        if (space == string::npos) continue;
        int   count = stoi(line.substr(0, space));
        string gate =      line.substr(space + 1);
        done[gate] = count;
    }
    return done;
}

// Append a single result immediately so we never lose it on interrupt.
void append_result(const string& filename, const string& gate, int count) {
    ofstream f(filename, ios::app);
    f << count << " " << gate << "\n";
}

// Re-write the output file sorted by gate count (called at the very end).
void write_sorted(const string& filename,
                  const unordered_map<string, int>& results) {
    auto cmp = [](const pair<string,int>& a, const pair<string,int>& b){
        return a.second < b.second;
    };
    multiset<pair<string,int>, decltype(cmp)> sorted_set(cmp);
    for (const auto& kv : results)
        sorted_set.insert(kv);

    ofstream out(filename);
    for (const auto& [gate, count] : sorted_set)
        out << count << " " << gate << "\n";
}


int main() {
    // ── Signal setup ─────────────────────────────────────────────────────────
    signal(SIGINT,  handle_sigint);
    signal(SIGTERM, handle_sigint);

    const string CHECKPOINT_FILE = "gate_counts.txt";

    // ── Load inputs ───────────────────────────────────────────────────────────
    vector<string> gate_strings = load_gates("../prove_math/universal_gates_constant_excluded.txt");

    vector<wire> sum_gate = {
        T_POS,  T_NEG,  T_ZERO,
        T_NEG,  T_ZERO, T_POS,
        T_ZERO, T_POS,  T_NEG
    };
    vector<wire> cry_gate = {
        T_NEG,  T_ZERO,  T_ZERO,
        T_ZERO,  T_ZERO, T_ZERO,
        T_ZERO, T_ZERO,  T_POS
    };

    // ── Resume: find out which gates are already done ─────────────────────────
    unordered_map<string, int> all_results = load_checkpoint(CHECKPOINT_FILE);
    size_t already_done = all_results.size();

    if (already_done > 0)
        cout << "Resuming from checkpoint — " << already_done
             << " / " << gate_strings.size() << " already done.\n";

    // ── Progress bar ──────────────────────────────────────────────────────────
    indicators::show_console_cursor(false);
    using namespace indicators;
    BlockProgressBar bar{
        option::BarWidth{40},
        option::ForegroundColor{Color::white},
        option::PrefixText{"Counting Gates for Sum+Carry "},
        option::ShowElapsedTime{true},
        option::Start{"|"},
        option::End{"|"},
        option::ShowRemainingTime{true},
        option::FontStyles{vector<FontStyle>{FontStyle::bold}},
        option::MaxProgress{gate_strings.size()}
    };

    // Fast-forward the bar past already-completed entries.
    for (size_t k = 0; k < already_done; ++k)
        bar.tick();

    // ── Main loop ─────────────────────────────────────────────────────────────
    for (size_t i = 0; i < gate_strings.size(); ++i) {

        // Check for Ctrl-C / SIGTERM between iterations.
        if (g_interrupted.load()) {
            cout << "\nInterrupted — progress saved to " << CHECKPOINT_FILE << "\n";
            break;
        }

        const string& gs = gate_strings[i];

        // Skip if already computed in a previous run.
        if (all_results.count(gs))
            continue;

        bar.set_option(option::PostfixText{
            to_string(i) + "/" + to_string(gate_strings.size())
        });
        bar.tick();

        auto Arb = string_to_gate(gs);
        int sgc  = interruptible_gate_search(Arb, sum_gate, ExhaustMode::FAST_MODE);

        if (sgc == -1) {
            // Interrupted mid-search — don't save a partial result for this gate.
            cout << "\nInterrupted mid-search — this gate will be retried next run.\n";
            break;
        }

        // Persist immediately so a crash/interrupt only loses the current item.
        append_result(CHECKPOINT_FILE, gs, sgc);
        all_results[gs] = sgc;
    }

    indicators::show_console_cursor(true);

    // ── If we finished everything, re-sort the file ───────────────────────────
    if (!g_interrupted.load()) {
        cout << "\nAll done — writing sorted results to " << CHECKPOINT_FILE << "\n";
        write_sorted(CHECKPOINT_FILE, all_results);
    }
}