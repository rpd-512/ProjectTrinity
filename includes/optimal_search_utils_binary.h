#ifndef OPTIMAL_NAND_SEARCH_H
#define OPTIMAL_NAND_SEARCH_H

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <chrono>



enum class OptMode {
    DEPTH_OPTIMAL,
    SIZE_OPTIMAL
};

// Returns {gate_count, depth} or {-1, -1} if not found.
// memo_of_completes: set of functions that are known sufficient — reaching any
// one of them during search is treated identically to reaching Tgt itself.
pair<int,int> gate_search_unified(
    vector<wire> Arb,
    vector<wire> Tgt,
    OptMode opt_mode,
    const unordered_set<vector<wire>, VectorHash> memo_of_completes = {},
    int max_depth = 20,
    int max_size = 40)
{
    auto P1 = get_binary_vector([](wire a, wire b){ return a; });
    auto P2 = get_binary_vector([](wire a, wire b){ return b; });

    // Seed check — Tgt or any memo member found at cost {0,0}
    if (Tgt == P1 || memo_of_completes.count(P1)) return {0, 0};
    if (Tgt == P2 || memo_of_completes.count(P2)) return {0, 0};
    if (memo_of_completes.count(Tgt))             return {0, 0};

    // =========================================================
    // DEPTH OPTIMAL (BFS, TREE MODEL)
    // =========================================================
    if (opt_mode == OptMode::DEPTH_OPTIMAL)
    {
        unordered_map<vector<wire>, pair<int,int>, VectorHash> S;

        vector<vector<wire>> all_tts;
        vector<vector<wire>> prev;

        S[P1] = {0,0};
        S[P2] = {0,0};

        all_tts = {P1, P2};
        prev    = {P1, P2};

        for (int d = 1; d <= max_depth; d++)
        {
            vector<vector<wire>> new_layer;

            unordered_map<vector<wire>, int, VectorHash> best_gc;

            // Compute candidates from cross product of all_tts x prev
            auto process = [&](const vector<wire>& f, const vector<wire>& g)
            {
                auto h = compose(f, g, Arb);
                int gc = S[f].first + S[g].first + 1;

                if (!best_gc.count(h) || gc < best_gc[h])
                    best_gc[h] = gc;
            };

            for (auto& f : all_tts)
            for (auto& g : prev)
                process(f,g);

            for (auto& f : prev)
            for (auto& g : all_tts)
                process(f,g);

            pair<int,int> best = {INT_MAX, d};
            bool found = false;

            for (auto& [h, gc] : best_gc)
            {
                if (!S.count(h))
                {
                    S[h] = {gc, d};
                    new_layer.push_back(h);
                }
                else if (S[h].second == d && gc < S[h].first)
                {
                    S[h].first = gc;
                }

                // Memo hit is terminal — same semantics as h == Tgt
                if (h == Tgt || memo_of_completes.count(h))
                {
                    found = true;
                    if (gc < best.first)
                        best = {gc, d};
                }
            }

            if (found) return best;

            all_tts.insert(all_tts.end(), new_layer.begin(), new_layer.end());
            prev = new_layer;

            if (prev.empty()) break;
        }

        return {-1,-1};
    }

    // =========================================================
    // SIZE OPTIMAL (DP, DAG MODEL)
    // =========================================================
    else
    {
        auto encode9 = [](const vector<wire>& tt){
            int code = 0, mul = 1;
            for(int i=0;i<9;i++){
                code += tt[i]*mul;
                mul *= 3;
            }
            return code;
        };

        unordered_map<int, pair<int,int>> known; // code → {size, depth}
        unordered_map<int, vector<vector<wire>>> by_size;

        int c1 = encode9(P1);
        int c2 = encode9(P2);

        known[c1] = {0,0};
        known[c2] = {0,0};

        by_size[0] = {P1, P2};

        int target_code = encode9(Tgt);

        for (int s = 1; s <= max_size; s++)
        {
            vector<vector<wire>> new_items;

            for (int sf = 0; sf < s; sf++)
            {
                int sg = s - 1 - sf;

                if (!by_size.count(sf) || !by_size.count(sg)) continue;

                for (auto& f : by_size[sf])
                for (auto& g : by_size[sg])
                {
                    auto h = compose(f, g, Arb);
                    int ch = encode9(h);

                    if (!known.count(ch))
                    {
                        int depth = max(known[encode9(f)].second,
                                        known[encode9(g)].second) + 1;

                        known[ch] = {s, depth};
                        new_items.push_back(h);

                        // Memo hit is terminal — same semantics as ch == target_code
                        if (ch == target_code || memo_of_completes.count(h))
                            return {s, depth};
                    }
                }
            }

            by_size[s] = new_items;
        }

        return {-1,-1};
    }
}

#endif