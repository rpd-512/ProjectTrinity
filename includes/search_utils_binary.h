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

struct VectorHash
{
    size_t operator()(const vector<wire>& v) const{
        size_t h = 0;
        for (auto x : v)
            h = h * 3 + (int)x;
        return h;
    }
};

template<typename ArbFunc>
int gate_search(
    ArbFunc Arb,
    vector<wire> target,
    ExhaustMode mode = ExhaustMode::FAST_MODE,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis = true,
    bool debug_print = false)
{
    const bool DEBUG = (mode == ExhaustMode::DEBUG_MODE);
    auto start = chrono::high_resolution_clock::now();

    auto P1 = [](wire a, wire b){ return a; };
    auto P2 = [](wire a, wire b){ return b; };

    auto Cn = [](wire a, wire b){ return T_NEG; };
    auto Cz = [](wire a, wire b){ return T_ZERO; };
    auto Cp = [](wire a, wire b){ return T_POS; };

    auto G  = [Arb](wire a, wire b){ return Arb(a,b); };

    auto compose_binary =
    [&](const vector<wire>& f,
        const vector<wire>& g)
    {
        vector<wire> h(9);

        for(int i = 0; i < 9; i++)
            h[i] = Arb(f[i], g[i]);

        return h;
    };

    if (DEBUG)
    {
        // map: truth-table -> {expression string, gate depth}
        unordered_map<vector<wire>, pair<string,int>, VectorHash> S;

        S[get_binary_vector(P1)] = {"x",        0};
        S[get_binary_vector(P2)] = {"y",        0};
        if (constant_basis) {
            S[get_binary_vector(Cn)] = {"-",    0};
            S[get_binary_vector(Cz)] = {"0",    0};
            S[get_binary_vector(Cp)] = {"+",    0};
        }
        S[get_binary_vector(G)]  = {"Arb(x,y)", 1};

        for (const auto& p : S)
        {
            const auto& [expr, d] = p.second;

            if (p.first == target)
            {
                if(debug_print){
                    printf("\n===== TARGET FOUND =====\n");
                    printf("Expression:\n%s\n", expr.c_str());
                    printf("\nTruth table vector:\n");
                    print_vector(p.first);
                    auto end = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                    printf("\nGates used: %d\n", d);
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
                    printf("\nGates used: %d\n", d);
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

            vector<pair<vector<wire>, pair<string,int>>> current(
                S.begin(), S.end());

            for (const auto& p1 : current)
            for (const auto& p2 : current)
            {
                auto h = compose_binary(p1.first, p2.first);
                int d = p1.second.second + p2.second.second + 1;
                string expr = "Arb(" + p1.second.first + ", " + p2.second.first + ")";
                if (S.find(h) == S.end())
                {

                    S[h] = {expr, d};
                    changed = true;

                    if (h == target)
                    {
                        if(debug_print){
                            printf("\n===== TARGET FOUND =====\n");
                            printf("Expression:\n%s\n", expr.c_str());
                            printf("\nTruth table vector:\n");
                            print_vector(h);
                            auto end = chrono::high_resolution_clock::now();
                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                            printf("\nGates used: %d\n", d);
                            printf("Functions generated: %zu\n", S.size());
                            printf("Execution time: %ld microseconds\n", duration.count());
                            printf("======================\n");
                        }
                        return d;
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
                            printf("\nGates used: %d\n", d);
                            printf("Functions generated: %zu\n", S.size());
                            printf("Execution time: %ld microseconds\n", duration.count());
                            printf("=============================\n");
                        }
                        return d;
                    }
                }
                else if (d < S[h].second)
                {
                    S[h] = {expr, d};
                    changed = true;
                }

            }
        }
        if(debug_print){
            printf("\ntarget not generated.\n");
            printf("Total functions: %zu\n", S.size());
        }
        return -1;
    }

    else if(mode == ExhaustMode::FAST_MODE)
    {
        // unordered_map: truth-table -> gate depth
        unordered_map<vector<wire>, int, VectorHash> S;

        S[get_binary_vector(P1)] = 0;
        S[get_binary_vector(P2)] = 0;
        if (constant_basis) {
            S[get_binary_vector(Cn)] = 0;
            S[get_binary_vector(Cz)] = 0;
            S[get_binary_vector(Cp)] = 0;
        }
        S[get_binary_vector(G)] = 1;

        for (const auto& p : S)
        {
            if (p.first == target || memo.count(p.first))
                return p.second;
        }

        bool changed = true;

        while (changed)
        {
            changed = false;

            vector<pair<vector<wire>, int>> current(
                S.begin(), S.end());

            for (const auto& [f, df] : current)
            for (const auto& [g, dg] : current)
            {
                auto h = compose_binary(f, g);

                int d = df + dg + 1;
                if (!S.count(h)){
                    S[h] = d;
                    changed = true;

                    if (h == target || memo.count(h))
                        return d;

                    if (S.size() == 19683)
                        return -1;
                }
                else if (d < S[h]){
                    S[h] = d;
                    changed = true;
                }
            }
        }

        return -1;
    }

    else if (mode == ExhaustMode::FAST_NO_DEPTH)
    {
        set<vector<wire>> S;

        S.insert(get_binary_vector(P1));
        S.insert(get_binary_vector(P2));
        if (constant_basis) {
            S.insert(get_binary_vector(Cn));
            S.insert(get_binary_vector(Cz));
            S.insert(get_binary_vector(Cp));
        }
        S.insert(get_binary_vector(G));

        for (const auto& p : S)
            if (p == target || memo.count(p))
                return 1;

        bool changed = true;

        while (changed)
        {
            changed = false;

            vector<vector<wire>> current(S.begin(), S.end());

            for (const auto& f : current)
            for (const auto& g : current)
            {
                auto h = compose_binary(f, g);

                if (S.insert(h).second)
                {
                    if (h == target || memo.count(h))
                        return 1;

                    changed = true;

                    if (S.size() == 19683)
                        return -1;
                }
            }
        }

        return -1;
    }
    return -1;
}

template<typename ArbFunc>
bool nand_search(
    ArbFunc Arb,
    ExhaustMode mode = ExhaustMode::FAST_NO_DEPTH,
    unordered_set<vector<wire>, VectorHash> memo = {},
    bool constant_basis = true){

    vector<wire> target = {
        T_POS, T_POS,  T_POS,
        T_POS, T_ZERO, T_ZERO,
        T_POS, T_ZERO, T_NEG
    };
    return gate_search(Arb, target, mode, memo, constant_basis) > 0;
}

#endif