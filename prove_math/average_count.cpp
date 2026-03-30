#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include "../includes/optimal_search_utils_binary.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <random>
#include <numeric>

vector<wire> decode9(int code)
{
    vector<wire> tt(9);
    for (int i = 0; i < 9; i++) {
        tt[i] = code % 3;
        code /= 3;
    }
    return tt;
}

int main(int argc, char* argv[])
{
    int N = 15; // default
    if (argc > 1) N = stoi(argv[1]);
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

    // Total valid gate codes: 3^9 = 19683
    constexpr int MAX_CODE = 19683;

    mt19937 rng(42);
    uniform_int_distribution<int> dist(0, MAX_CODE - 1);

    // Per-run timing storage
    vector<long long> unified_times(N), exhaust_times(N);

    // Gate/depth results (optional, stored for inspection)
    struct Result { int gates; int depth; };
    vector<Result> unified_sum(N), unified_carry(N);
    vector<Result> exhaust_sum(N),  exhaust_carry(N);

    indicators::show_console_cursor(false);
    indicators::BlockProgressBar bar{
        indicators::option::BarWidth{50},
        indicators::option::MaxProgress{N},
        indicators::option::PrefixText{"Benchmarking "},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
    };

    for (int i = 0; i < N; i++) {
        int code = dist(rng);
        vector<wire> Arb = decode9(code);
        int code_s = dist(rng);
        vector<wire> Tgt = decode9(code_s);
        // --- unified search ---
        auto s1 = chrono::high_resolution_clock::now();
        auto [g1, d1] = gate_search_unified(Arb, Tgt, OptMode::DEPTH_OPTIMAL);
        auto e1 = chrono::high_resolution_clock::now();
        unified_times[i] = chrono::duration_cast<chrono::microseconds>(e1 - s1).count();
        unified_sum[i]   = {g1, d1};

        // --- exhaustive search ---
        auto s2 = chrono::high_resolution_clock::now();
        auto [sg, sd] = gate_search(Arb, Tgt,   ExhaustMode::FAST_MODE, {}, true);
        auto e2 = chrono::high_resolution_clock::now();
        exhaust_times[i] = chrono::duration_cast<chrono::microseconds>(e2 - s2).count();
        exhaust_sum[i]   = {sg, sd};

        bar.tick();
    }

    indicators::show_console_cursor(true);
    bar.mark_as_completed();

    // --- Compute statistics ---
    auto stats = [&](vector<long long> times) {
        int n = times.size();
        sort(times.begin(), times.end());

        double avg  = accumulate(times.begin(), times.end(), 0LL) / (double)n;
        long long mn  = times.front();
        long long mx  = times.back();

        auto percentile = [&](double p) -> long long {
            int idx = (int)ceil(p / 100.0 * n) - 1;
            return times[clamp(idx, 0, n - 1)];
        };

        long long p50 = percentile(50);
        long long p90 = percentile(90);
        long long p99 = percentile(99);

        cout << fixed << setprecision(2);
        cout << "  Avg: " << avg  << " us\n";
        cout << "  Min: " << mn   << " us\n";
        cout << "  p50: " << p50  << " us\n";
        cout << "  p90: " << p90  << " us\n";
        cout << "  p99: " << p99  << " us\n";
        cout << "  Max: " << mx   << " us\n";
    };

    cout << "\n========== BENCHMARK RESULTS (N=" << N << ") ==========\n";

    cout << "\n--- gate_search_unified ---\n";
    stats(unified_times);

    cout << "\n--- gate_search (FAST_MODE) ---\n";
    stats(exhaust_times);

    // Speedup on median (more robust than avg)
    auto median = [&](vector<long long> times) -> long long {
        sort(times.begin(), times.end());
        return times[times.size() / 2];
    };
    double u_med = median(unified_times);
    double e_med = median(exhaust_times);
    cout << "\nSpeedup unified vs exhaust (median): " << e_med / u_med << "x\n";
    cout << "Speedup unified vs exhaust (avg):    "
         << accumulate(exhaust_times.begin(), exhaust_times.end(), 0LL) /
            (double)accumulate(unified_times.begin(), unified_times.end(), 0LL) << "x\n";
            
    return 0;
}