#!/usr/bin/env python3
"""
Fast numpy-accelerated depth search + formula size search + binary baseline.
Uses results from experiments.py (Exp 1-3, 8) which are already confirmed.
"""

import itertools
import time
import math
import json
import sys
import numpy as np
from collections import Counter, defaultdict

# ============================================================
# Constants (same encoding as experiments.py)
# ============================================================
T = [0, 1, 2]
T_BAL = [-1, 0, 1]
INPUT_PAIRS = [(a, b) for a in T for b in T]
POW3 = np.array([3**i for i in range(9)], dtype=np.int32)

PI1 = tuple(a for a, b in INPUT_PAIRS)
PI2 = tuple(b for a, b in INPUT_PAIRS)

S3 = list(itertools.permutations(T))

def encode9(tt):
    return sum(int(tt[i]) * int(3**i) for i in range(9))

def decode9(code):
    tt = []
    c = int(code)
    for _ in range(9):
        tt.append(c % 3)
        c //= 3
    return tuple(tt)

def compute_half_adder():
    hsum, hcarry = [], []
    for a in T_BAL:
        for b in T_BAL:
            s = a + b
            if s >= 2: c = 1; s -= 3
            elif s <= -2: c = -1; s += 3
            else: c = 0
            hsum.append(s + 1)
            hcarry.append(c + 1)
    return tuple(hsum), tuple(hcarry)

HSUM, HCARRY = compute_half_adder()

# ============================================================
# Fast unary completeness check (from experiments.py)
# ============================================================
def check_unary_complete(gate_tt):
    gl = list(gate_tt)
    u_vals = [(c % 3, (c // 3) % 3, c // 9) for c in range(27)]
    comp = [0] * 729
    for fi in range(27):
        f = u_vals[fi]
        for gi in range(27):
            g = u_vals[gi]
            comp[fi*27+gi] = gl[f[0]*3+g[0]] + 3*gl[f[1]*3+g[1]] + 9*gl[f[2]*3+g[2]]
    arb = gl[0] + 3*gl[4] + 9*gl[8]
    S = (1 << arb) | (1 << 21)
    ALL = (1 << 27) - 1
    changed = True
    while changed:
        if S == ALL: return True
        changed = False
        current = [i for i in range(27) if S & (1 << i)]
        for fi in current:
            for gi in current:
                hi = comp[fi*27+gi]
                if not (S & (1 << hi)):
                    S |= (1 << hi)
                    changed = True
    return S == ALL

# ============================================================
# Isomorphism helpers
# ============================================================
def apply_value_perm(gate_tt, sigma):
    inv = [0]*3
    for i, s in enumerate(sigma): inv[s] = i
    return tuple(sigma[gate_tt[inv[a]*3+inv[b]]] for a, b in INPUT_PAIRS)

def swap_inputs(gate_tt):
    return tuple(gate_tt[b*3+a] for a, b in INPUT_PAIRS)

def canonical_form(gate_tt):
    forms = []
    for sigma in S3:
        t = apply_value_perm(gate_tt, sigma)
        forms.append(t)
        forms.append(swap_inputs(t))
    return min(forms)

# ============================================================
# NUMPY-ACCELERATED DEPTH SEARCH
# ============================================================
def depth_search_numpy(gate_tt, targets, max_depth=7, chunk_size=2000):
    """BFS by depth using numpy for batch composition."""
    gl = np.zeros((3, 3), dtype=np.int8)
    for i, (a, b) in enumerate(INPUT_PAIRS):
        gl[a][b] = gate_tt[i]

    target_codes = set()
    for t in targets:
        target_codes.add(encode9(t))
    results = {}

    # Init with projections
    pi1_np = np.array(PI1, dtype=np.int8)
    pi2_np = np.array(PI2, dtype=np.int8)

    all_np = np.stack([pi1_np, pi2_np])  # (2, 9)
    known_codes = set()
    known_codes.add(int((pi1_np.astype(np.int32) * POW3).sum()))
    known_codes.add(int((pi2_np.astype(np.int32) * POW3).sum()))

    for c in known_codes & target_codes:
        results[c] = 0

    if len(results) == len(target_codes):
        return results

    new_np = all_np.copy()  # at depth 0, everything is "new"

    for d in range(1, max_depth + 1):
        if len(new_np) == 0:
            break

        N_all = len(all_np)
        N_new = len(new_np)

        discovered_codes = set()
        discovered_tts = []

        # Process in chunks to manage memory
        # Compose: all_chunks x new
        for start in range(0, N_all, chunk_size):
            end = min(start + chunk_size, N_all)
            f_chunk = all_np[start:end]  # (C, 9)
            C = len(f_chunk)

            codes = np.zeros((C, N_new), dtype=np.int32)
            for k in range(9):
                fk = f_chunk[:, k].astype(np.intp)
                gk = new_np[:, k].astype(np.intp)
                hk = gl[fk[:, None], gk[None, :]]  # (C, N_new)
                codes += hk.astype(np.int32) * int(POW3[k])

            unique_in_chunk = set(np.unique(codes).tolist())
            for c in unique_in_chunk:
                if c not in known_codes and c not in discovered_codes:
                    discovered_codes.add(c)
                    discovered_tts.append(decode9(c))
                    if c in target_codes:
                        results[c] = d

        # Compose: new x all_chunks
        for start in range(0, N_all, chunk_size):
            end = min(start + chunk_size, N_all)
            g_chunk = all_np[start:end]
            C = len(g_chunk)

            codes = np.zeros((N_new, C), dtype=np.int32)
            for k in range(9):
                fk = new_np[:, k].astype(np.intp)
                gk = g_chunk[:, k].astype(np.intp)
                hk = gl[fk[:, None], gk[None, :]]
                codes += hk.astype(np.int32) * int(POW3[k])

            unique_in_chunk = set(np.unique(codes).tolist())
            for c in unique_in_chunk:
                if c not in known_codes and c not in discovered_codes:
                    discovered_codes.add(c)
                    discovered_tts.append(decode9(c))
                    if c in target_codes:
                        results[c] = d

        if len(results) == len(target_codes):
            break

        if not discovered_tts:
            break

        known_codes.update(discovered_codes)
        new_np = np.array(discovered_tts, dtype=np.int8)
        all_np = np.concatenate([all_np, new_np])

    return results


# ============================================================
# FORMULA SIZE SEARCH (unchanged from experiments.py)
# ============================================================
def formula_size_search(gate_tt, targets, max_size=50):
    gl = list(gate_tt)
    target_codes = {encode9(t): t for t in targets}
    results = {}
    by_size = {}
    known = {}

    by_size[0] = []
    for tt in [PI1, PI2]:
        c = encode9(tt)
        if c not in known:
            known[c] = 0
            by_size[0].append((c, tt))
            if c in target_codes: results[c] = 0

    if len(results) == len(target_codes):
        return {target_codes[c]: s for c, s in results.items()}

    for s in range(1, max_size + 1):
        new_items = []
        for sf in range(s):
            sg = s - 1 - sf
            if sf not in by_size or sg not in by_size: continue
            for cf, f in by_size[sf]:
                for cg, g in by_size[sg]:
                    code = sum(gl[f[i]*3+g[i]] * POW3[i] for i in range(9))
                    code = int(code)
                    if code not in known:
                        known[code] = s
                        h = tuple(gl[f[i]*3+g[i]] for i in range(9))
                        new_items.append((code, h))
                        if code in target_codes: results[code] = s
        by_size[s] = new_items
        if len(results) == len(target_codes): break

    return {target_codes.get(c, None): s for c, s in results.items() if c in target_codes}


# ============================================================
# BINARY NAND BASELINE
# ============================================================
def binary_nand_baseline():
    B_PAIRS = [(a, b) for a in [0, 1] for b in [0, 1]]
    NAND_TT = (1, 1, 1, 0)
    XOR_TT = (0, 1, 1, 0)
    AND_TT = (0, 0, 0, 1)

    gl = [0] * 4
    for i, (a, b) in enumerate(B_PAIRS):
        gl[a * 2 + b] = NAND_TT[i]

    def encode4(tt): return sum(tt[i] * (2**i) for i in range(4))

    pi1_b, pi2_b = (0, 0, 1, 1), (0, 1, 0, 1)
    target_map = {XOR_TT: 'XOR', AND_TT: 'AND'}

    # Depth BFS
    known = {}
    all_tts = []
    prev_tts = []
    for tt in [pi1_b, pi2_b]:
        known[encode4(tt)] = 0
        all_tts.append(tt)
        prev_tts.append(tt)

    depth_res = {}
    for d in range(1, 10):
        new_tts = []
        for f in all_tts:
            for g in prev_tts:
                h = tuple(gl[f[i]*2+g[i]] for i in range(4))
                hc = encode4(h)
                if hc not in known:
                    known[hc] = d; new_tts.append(h)
                    if h in target_map: depth_res[target_map[h]] = d
        for f in prev_tts:
            for g in all_tts:
                h = tuple(gl[f[i]*2+g[i]] for i in range(4))
                hc = encode4(h)
                if hc not in known:
                    known[hc] = d; new_tts.append(h)
                    if h in target_map: depth_res[target_map[h]] = d
        all_tts.extend(new_tts)
        prev_tts = new_tts
        if len(depth_res) == 2: break
        if not new_tts: break

    # Formula size BFS
    by_size = {0: [(encode4(tt), tt) for tt in [pi1_b, pi2_b]]}
    known_s = {c: 0 for c, _ in by_size[0]}
    size_res = {}
    for s in range(1, 30):
        new_items = []
        for sf in range(s):
            sg = s - 1 - sf
            if sf not in by_size or sg not in by_size: continue
            for cf, f in by_size[sf]:
                for cg, g in by_size[sg]:
                    h = tuple(gl[f[i]*2+g[i]] for i in range(4))
                    hc = encode4(h)
                    if hc not in known_s:
                        known_s[hc] = s; new_items.append((hc, h))
                        if h in target_map: size_res[target_map[h]] = s
        by_size[s] = new_items
        if len(size_res) == 2: break

    return depth_res, size_res


# ============================================================
# MAIN
# ============================================================
if __name__ == '__main__':
    print("\n" + "=" * 70)
    print("FAST EXPERIMENTS: Depth Search + Formula Size + Comparison")
    print("=" * 70)

    # Step 1: Find all universal gates and compute iso classes
    print("\nStep 1: Finding universal gates...")
    t0 = time.time()
    universal = []
    for idx in range(19683):
        tt = decode9(idx)
        if check_unary_complete(tt):
            universal.append(tt)
    print(f"  Found {len(universal)} universal gates in {time.time()-t0:.1f}s")

    # Step 2: Compute isomorphism class representatives
    print("\nStep 2: Computing isomorphism classes...")
    t0 = time.time()
    classes = defaultdict(list)
    for g in universal:
        c = canonical_form(g)
        classes[c].append(g)
    reps = [v[0] for v in classes.values()]
    print(f"  {len(classes)} classes, {len(reps)} representatives in {time.time()-t0:.1f}s")

    # Step 3: Depth search on ALL representatives (numpy-accelerated)
    print(f"\nStep 3: Depth search on {len(reps)} representative gates...")
    targets = [HSUM, HCARRY]
    hsum_code = encode9(HSUM)
    hcarry_code = encode9(HCARRY)

    all_depth_results = []
    t0 = time.time()
    for i, g in enumerate(reps):
        t1 = time.time()
        res = depth_search_numpy(g, targets, max_depth=7)
        t2 = time.time()

        hsum_d = res.get(hsum_code, -1)
        hcarry_d = res.get(hcarry_code, -1)
        all_depth_results.append({
            'gate_idx': encode9(g),
            'gate': g,
            'hsum_depth': hsum_d,
            'hcarry_depth': hcarry_d,
            'time': t2 - t1,
        })

        # Progress
        elapsed = time.time() - t0
        eta = elapsed / (i+1) * (len(reps) - i - 1) if i > 0 else 0
        sys.stdout.write(f"\r  [{i+1}/{len(reps)}] D(hs)={hsum_d} D(hc)={hcarry_d} "
                         f"t={t2-t1:.0f}s elapsed={elapsed:.0f}s ETA={eta:.0f}s   ")
        sys.stdout.flush()

    total_time = time.time() - t0
    print(f"\n  Total depth search time: {total_time:.1f}s")

    # Step 3b: Statistics
    valid = [r for r in all_depth_results if r['hsum_depth'] >= 0 and r['hcarry_depth'] >= 0]
    hsum_depths = [r['hsum_depth'] for r in valid]
    hcarry_depths = [r['hcarry_depth'] for r in valid]

    print(f"\n  Results: {len(valid)}/{len(reps)} gates successfully synthesised both targets")
    if hsum_depths:
        print(f"  hsum depth:  min={min(hsum_depths)} max={max(hsum_depths)} "
              f"mean={np.mean(hsum_depths):.2f} median={int(np.median(hsum_depths))}")
        print(f"  hcarry depth: min={min(hcarry_depths)} max={max(hcarry_depths)} "
              f"mean={np.mean(hcarry_depths):.2f} median={int(np.median(hcarry_depths))}")
        print(f"\n  hsum depth distribution:")
        for d, cnt in sorted(Counter(hsum_depths).items()):
            print(f"    depth {d}: {cnt} gates ({cnt/len(valid)*100:.1f}%)")
        print(f"  hcarry depth distribution:")
        for d, cnt in sorted(Counter(hcarry_depths).items()):
            print(f"    depth {d}: {cnt} gates ({cnt/len(valid)*100:.1f}%)")

        # Best by full-adder carry depth (2*D_hs + D_hc)
        valid.sort(key=lambda r: 2*r['hsum_depth'] + r['hcarry_depth'])
        print(f"\n  Top 10 gates by full-adder carry depth (2*D_hs + D_hc):")
        for r in valid[:10]:
            fa_cd = 2*r['hsum_depth'] + r['hcarry_depth']
            print(f"    Gate {r['gate_idx']:>5d}: D(hs)={r['hsum_depth']} D(hc)={r['hcarry_depth']} "
                  f"FA_carry_depth={fa_cd}")

    # Step 4: Formula size for top 10 gates
    print(f"\nStep 4: Formula size search for top 10 gates...")
    top_gates = valid[:10] if valid else []
    size_results = []
    for i, r in enumerate(top_gates):
        t1 = time.time()
        sizes = formula_size_search(r['gate'], targets, max_size=50)
        t2 = time.time()

        hsum_s = sizes.get(HSUM, -1)
        hcarry_s = sizes.get(HCARRY, -1)

        size_results.append({
            'gate_idx': r['gate_idx'],
            'gate': r['gate'],
            'hsum_depth': r['hsum_depth'],
            'hcarry_depth': r['hcarry_depth'],
            'hsum_size': hsum_s,
            'hcarry_size': hcarry_s,
        })

        fa_gates = 3*hsum_s + 2*hcarry_s if hsum_s >= 0 and hcarry_s >= 0 else -1
        print(f"  Gate {r['gate_idx']:>5d}: S(hs)={hsum_s} S(hc)={hcarry_s} "
              f"FA_gates={fa_gates} time={t2-t1:.1f}s")

    # Step 5: Binary NAND baseline
    print(f"\nStep 5: Binary NAND baseline...")
    binary_depth, binary_size = binary_nand_baseline()
    print(f"  XOR: depth={binary_depth['XOR']}, size={binary_size['XOR']}")
    print(f"  AND: depth={binary_depth['AND']}, size={binary_size['AND']}")

    b_xor_d = binary_depth['XOR']
    b_and_d = binary_depth['AND']
    b_xor_s = binary_size['XOR']
    b_and_s = binary_size['AND']
    b_fa_sum_d = 2 * b_xor_d
    b_fa_carry_d = 2 * b_xor_d + b_and_d
    b_fa_gates = 3 * b_xor_s + 2 * b_and_s

    print(f"  Binary FA: sum_depth={b_fa_sum_d} carry_depth={b_fa_carry_d} gates={b_fa_gates}")

    # Step 6: Full comparison table
    print(f"\n{'='*70}")
    print("FULL ADDER COMPARISON TABLE")
    print(f"{'='*70}")
    print(f"\n  Binary (NAND):")
    print(f"    h_sum: depth={b_xor_d}, gates={b_xor_s}")
    print(f"    h_carry: depth={b_and_d}, gates={b_and_s}")
    print(f"    FA sum depth:   {b_fa_sum_d}")
    print(f"    FA carry depth: {b_fa_carry_d}")
    print(f"    FA gate count:  {b_fa_gates}")

    if size_results:
        best = min(size_results, key=lambda r: 2*r['hsum_depth']+r['hcarry_depth'])
        t_hs_d = best['hsum_depth']
        t_hc_d = best['hcarry_depth']
        t_hs_s = best['hsum_size']
        t_hc_s = best['hcarry_size']
        t_fa_sum_d = 2 * t_hs_d
        t_fa_carry_d = 2 * t_hs_d + t_hc_d
        t_fa_gates = 3 * t_hs_s + 2 * t_hc_s

        print(f"\n  Ternary (best gate {best['gate_idx']}):")
        print(f"    h_sum: depth={t_hs_d}, gates={t_hs_s}")
        print(f"    h_carry: depth={t_hc_d}, gates={t_hc_s}")
        print(f"    FA sum depth:   {t_fa_sum_d}")
        print(f"    FA carry depth: {t_fa_carry_d}")
        print(f"    FA gate count:  {t_fa_gates}")

        # Best by gate count
        best_gc = min(size_results, key=lambda r: 3*r['hsum_size']+2*r['hcarry_size']
                      if r['hsum_size'] >= 0 and r['hcarry_size'] >= 0 else 999999)
        if best_gc != best:
            t2_fa_gates = 3*best_gc['hsum_size'] + 2*best_gc['hcarry_size']
            print(f"\n  Ternary (best by gates, gate {best_gc['gate_idx']}):")
            print(f"    FA gate count:  {t2_fa_gates}")
            t_fa_gates_best = min(t_fa_gates, t2_fa_gates)
        else:
            t_fa_gates_best = t_fa_gates

        # Ripple-carry comparison
        bits_per_trit = math.log2(3)
        print(f"\n  Ripple-Carry Adder Comparison:")
        print(f"  {'N':>4s} | {'B_FAs':>5s} {'B_depth':>7s} {'B_gates':>7s} | "
              f"{'T_FAs':>5s} {'T_depth':>7s} {'T_gates':>7s} | "
              f"{'D_ratio':>7s} {'G_ratio':>7s}")
        print(f"  {'-'*4}-+-{'-'*5}-{'-'*7}-{'-'*7}-+-{'-'*5}-{'-'*7}-{'-'*7}-+-{'-'*7}-{'-'*7}")

        for N in [4, 8, 16, 32, 64]:
            n_trits = math.ceil(N / bits_per_trit)
            bd = N * b_fa_carry_d
            bg = N * b_fa_gates
            td = n_trits * t_fa_carry_d
            tg = n_trits * t_fa_gates_best
            print(f"  {N:>4d} | {N:>5d} {bd:>7d} {bg:>7d} | "
                  f"{n_trits:>5d} {td:>7d} {tg:>7d} | "
                  f"{td/bd:>7.3f} {tg/bg:>7.3f}")

    # Save results
    summary = {
        'n_universal': 3774,
        'n_iso_classes': len(classes),
        'binary_nand': {
            'xor_depth': b_xor_d, 'and_depth': b_and_d,
            'xor_size': b_xor_s, 'and_size': b_and_s,
            'fa_sum_depth': b_fa_sum_d, 'fa_carry_depth': b_fa_carry_d,
            'fa_gates': b_fa_gates,
        },
        'depth_stats': {
            'n_tested': len(reps),
            'n_valid': len(valid),
            'hsum_depth_min': min(hsum_depths) if hsum_depths else -1,
            'hsum_depth_max': max(hsum_depths) if hsum_depths else -1,
            'hcarry_depth_min': min(hcarry_depths) if hcarry_depths else -1,
            'hcarry_depth_max': max(hcarry_depths) if hcarry_depths else -1,
            'hsum_depth_dist': dict(Counter(hsum_depths)),
            'hcarry_depth_dist': dict(Counter(hcarry_depths)),
        },
    }
    if size_results:
        summary['best_ternary'] = [{
            'gate_idx': r['gate_idx'],
            'gate': list(r['gate']),
            'hsum_depth': r['hsum_depth'],
            'hcarry_depth': r['hcarry_depth'],
            'hsum_size': r['hsum_size'],
            'hcarry_size': r['hcarry_size'],
        } for r in size_results]

    with open('results.json', 'w') as f:
        json.dump(summary, f, indent=2)
    print(f"\nResults saved to results.json")
    print("DONE.")
