#ifndef NAND_SEARCH_H
#define NAND_SEARCH_H

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <chrono>

#include "search_utils_unary.h"

template<typename Func>
vector<wire> get_binary_vector(Func f)
{
    vector<wire> v(9);

    wire vals[3] = {T_NEG, T_ZERO, T_POS};

    int k = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            v[k++] = f(vals[i], vals[j]);

    return v;
}

static int nand_checks = 0;

// Returns {gate_count, depth} or {-1, -1} if not found
pair<int,int> gate_search(
    vector<wire> Arb, // Arbitrary Function
    vector<wire> Tgt, // Target function to generate
    ExhaustMode mode = ExhaustMode::FAST_MODE,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool debug_print = false)
{
    const bool DEBUG = (mode == ExhaustMode::DEBUG_MODE);
    auto start = chrono::high_resolution_clock::now();

    auto P1 = [](wire a, wire b){ return a; };
    auto P2 = [](wire a, wire b){ return b; };

    auto G  = [Arb](wire a, wire b){ return Arb[a * 3 + b]; };

    if (DEBUG)
    {
        // map: truth-table -> {expression string, gate depth}
        unordered_map<vector<wire>, pair<string, pair<int, int>>, VectorHash> S;

        S[get_binary_vector(P1)] = {"x",        {0, 0}};
        S[get_binary_vector(P2)] = {"y",        {0, 0}};
        S[get_binary_vector(G)]  = {"Arb(x,y)", {1, 1}};
        for (const auto& p : S)
        {
            const auto& [expr, d] = p.second;

            if (p.first == Tgt)
            {
                if(debug_print){
                    printf("\n===== TARGET FOUND =====\n");
                    printf("Expression:\n%s\n", expr.c_str());
                    printf("\nTruth table vector:\n");
                    print_vector(p.first);
                    auto end = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                    printf("\nGates used: %d\n", d.first);
                    printf("Functions generated: %zu\n", S.size());
                    printf("Execution time: %ld microseconds\n", duration.count());
                    printf("======================\n");
                }
                return d;
            }
            if (memo.count(p.first))
            {
                if(debug_print){
                    printf("\n===== TARGET FOUND (memo) =====\n");
                    printf("Expression:\n%s\n", expr.c_str());
                    printf("\nTruth table vector:\n");
                    print_vector(p.first);
                    auto end = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                    printf("\nGates used: %d\n", d.first);
                    printf("Functions generated: %zu\n", S.size());
                    printf("Execution time: %ld microseconds\n", duration.count());
                    printf("=============================\n");
                }
                return d;
            }
        }

        bool changed = true;

        while (changed)
        {
            changed = false;
            vector<pair<vector<wire>, pair<string,pair<int, int>>>> current(
                S.begin(), S.end());

            for (const auto& p1 : current)
            for (const auto& p2 : current)
            {
                auto h = compose(p1.first, p2.first, Arb);
                int gc = p1.second.second.first + p2.second.second.first + 1;
                int dp = max(p1.second.second.second, p2.second.second.second) + 1;                string expr = "Arb(" + p1.second.first + ", " + p2.second.first + ")";
                if (S.find(h) == S.end())
                {

                    S[h] = {expr, {gc, dp}};
                    changed = true;

                    if (h == Tgt)
                    {
                        if(debug_print){
                            printf("\n===== TARGET FOUND =====\n");
                            printf("Expression:\n%s\n", expr.c_str());
                            printf("\nTruth table vector:\n");
                            print_vector(h);
                            auto end = chrono::high_resolution_clock::now();
                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                            printf("\nGates used: %d\n", gc);
                            printf("Functions generated: %zu\n", S.size());
                            printf("Execution time: %ld microseconds\n", duration.count());
                            printf("======================\n");
                        }
                        return {gc, dp};
                    }
                    if (memo.count(h))
                    {
                        if(debug_print){
                            printf("\n===== TARGET FOUND (memo) =====\n");
                            printf("Expression:\n%s\n", expr.c_str());
                            printf("\nTruth table vector:\n");
                            print_vector(h);
                            auto end = chrono::high_resolution_clock::now();
                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                            printf("\nGates used: %d\n", gc);
                            printf("Functions generated: %zu\n", S.size());
                            printf("Execution time: %ld microseconds\n", duration.count());
                            printf("=============================\n");
                        }
                        return {gc, dp};
                    }
                }
                //else if (gc < S[h].second.first || (gc == S[h].second.first && dp < S[h].second.second))
                else if (dp < S[h].second.second || (dp == S[h].second.second && gc < S[h].second.first))
                {
                    S[h] = {expr, {gc, dp}};
                    changed = true;
                }

            }
        }
        if(debug_print){
            printf("\ntarget not generated.\n");
            printf("Total functions: %zu\n", S.size());
        }
        return {-1, -1};
    }

    else if(mode == ExhaustMode::FAST_MODE)
    {
        // unordered_map: truth-table -> gate depth
        unordered_map<vector<wire>, pair<int, int>, VectorHash> S;

        S[get_binary_vector(P1)] = {0, 0};
        S[get_binary_vector(P2)] = {0, 0};
        S[get_binary_vector(G)]  = {1, 1};
        for (const auto& p : S)
        {
            if (p.first == Tgt || memo.count(p.first))
                return p.second;
        }

        bool changed = true;
        
        while (changed)
        {
            changed = false;

            vector<pair<vector<wire>, pair<int, int>>> current(
                S.begin(), S.end());

            for (const auto& [f, df] : current)
            for (const auto& [g, dg] : current)
            {
                auto h = compose(f, g, Arb);

                int gc = df.first + dg.first + 1;
                int dp = max(df.second, dg.second) + 1;
                if (!S.count(h)){
                    S[h] = {gc, dp};
                    changed = true;

                    if (h == Tgt || memo.count(h))
                        return {gc, dp};

                    if (S.size() == 19683)
                        return {-1, -1};
                }
                else if (gc < S[h].first || (gc == S[h].first && dp < S[h].second)){
                //else if (dp < S[h].second || (dp == S[h].second && gc < S[h].first)){
                    S[h] = {gc, dp};
                    changed = true;
                }
            }
        }

        return {-1, -1};
    }

    else if (mode == ExhaustMode::FAST_NO_DEPTH)
    {
        set<vector<wire>> S;

        S.insert(get_binary_vector(P1));
        S.insert(get_binary_vector(P2));
        S.insert(get_binary_vector(G));

        for (const auto& p : S)
            if (p == Tgt || memo.count(p))
                return {1, 1};

        bool changed = true;

        while (changed)
        {
            changed = false;

            vector<vector<wire>> current(S.begin(), S.end());

            for (const auto& f : current)
            for (const auto& g : current)
            {
                auto h = compose(f, g, Arb);

                if (S.insert(h).second)
                {
                    if (h == Tgt || memo.count(h))
                        return {1,1};

                    changed = true;

                    if (S.size() == 19683)
                        return {-1,-1};
                }
            }
        }

        return {-1,-1};
    }
    return {-1,-1}; // should never reach here
}

bool nand_search(
    vector<wire> Arb,
    ExhaustMode mode = ExhaustMode::FAST_NO_DEPTH,
    unordered_set<vector<wire>, VectorHash> memo = {}){

    vector<wire> Tgt = {
        T_POS, T_POS,  T_POS,
        T_POS, T_ZERO, T_ZERO,
        T_POS, T_ZERO, T_NEG
    };
    nand_checks++;
    return gate_search(Arb, Tgt, mode, memo).first > 0;
}

#endif