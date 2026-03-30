#!/usr/bin/env python3
"""
Experiments for: On Universality and Efficient Arithmetic Realisation of Ternary Logic Gates

Implements:
  1. Universal gate enumeration (confirming 3,774 count)
  2. Isomorphism class analysis under S3 x Z2
  3. Property distribution of universal gates
  4. Ternary half-adder synthesis via depth-optimal BFS
  5. Binary NAND baseline comparison
  6. Full adder ternary vs binary efficiency analysis
  7. Formula-size (gate count) search for best gates
"""

import itertools
import time
import math
import json
import sys
from collections import Counter, defaultdict

# ============================================================
# Constants
# ============================================================
# Balanced ternary {-1, 0, 1} encoded internally as {0, 1, 2}
T = [0, 1, 2]
T_BAL = [-1, 0, 1]
INPUT_PAIRS = [(a, b) for a in T for b in T]  # 9 pairs, row-major

# Projection truth tables (9-tuples)
PI1 = tuple(a for a, b in INPUT_PAIRS)  # (0,0,0,1,1,1,2,2,2)
PI2 = tuple(b for a, b in INPUT_PAIRS)  # (0,1,2,0,1,2,0,1,2)

# Powers for integer encoding of 9-element truth tables
POW3 = [3**i for i in range(9)]  # [1, 3, 9, 27, 81, 243, 729, 2187, 6561]

# S3: all permutations of {0, 1, 2}
S3 = list(itertools.permutations(T))

# ============================================================
# Encoding helpers
# ============================================================
def encode9(tt):
    """Encode 9-element truth table as integer in [0, 19682]."""
    return sum(tt[i] * POW3[i] for i in range(9))

def decode9(code):
    """Decode integer to 9-element truth table tuple."""
    tt = []
    c = code
    for _ in range(9):
        tt.append(c % 3)
        c //= 3
    return tuple(tt)

# ============================================================
# Half-adder truth tables (balanced ternary)
# ============================================================
def compute_half_adder():
    """Compute balanced ternary half-adder sum and carry truth tables."""
    hsum, hcarry = [], []
    for a in T_BAL:
        for b in T_BAL:
            s = a + b
            if s >= 2:
                c = 1; s -= 3
            elif s <= -2:
                c = -1; s += 3
            else:
                c = 0
            hsum.append(s + 1)    # encode to {0,1,2}
            hcarry.append(c + 1)  # encode to {0,1,2}
    return tuple(hsum), tuple(hcarry)

HSUM, HCARRY = compute_half_adder()

# ============================================================
# EXPERIMENT 1: Universal Gate Enumeration
# ============================================================
def check_unary_complete(gate_tt):
    """Check if a binary-arity ternary gate is unary complete.

    Uses bitset representation for the set of generated unary functions
    and a precomputed composition table for speed.
    """
    # Flat gate lookup: gl[a*3+b] = output
    gl = list(gate_tt)

    # Precompute all 27 unary function values
    u_vals = []
    for code in range(27):
        u_vals.append((code % 3, (code // 3) % 3, code // 9))

    # Precompute composition table: comp[fi*27+gi] = hi
    comp = [0] * 729
    for fi in range(27):
        f = u_vals[fi]
        base = fi * 27
        for gi in range(27):
            g = u_vals[gi]
            h0 = gl[f[0] * 3 + g[0]]
            h1 = gl[f[1] * 3 + g[1]]
            h2 = gl[f[2] * 3 + g[2]]
            comp[base + gi] = h0 + 3 * h1 + 9 * h2

    # arb(x) = gate(x, x)
    arb = gl[0] + 3 * gl[4] + 9 * gl[8]
    identity = 21  # 0 + 3*1 + 9*2

    S = (1 << arb) | (1 << identity)
    ALL_BITS = (1 << 27) - 1

    changed = True
    while changed:
        if S == ALL_BITS:
            return True
        changed = False
        current = []
        bits = S
        for i in range(27):
            if bits & 1:
                current.append(i)
            bits >>= 1

        for fi in current:
            base = fi * 27
            for gi in current:
                hi = comp[base + gi]
                if not (S & (1 << hi)):
                    S |= (1 << hi)
                    changed = True

    return S == ALL_BITS


def experiment_1():
    """Find all universal (functionally complete) binary-arity ternary gates."""
    print("=" * 70)
    print("EXPERIMENT 1: Universal Gate Enumeration")
    print("=" * 70)

    t0 = time.time()
    universal = []
    for idx in range(19683):
        tt = decode9(idx)
        if check_unary_complete(tt):
            universal.append(tt)

    elapsed = time.time() - t0
    print(f"  Total binary-arity ternary gates: 19,683")
    print(f"  Universal gates found:             {len(universal)}")
    print(f"  Fraction:                          {len(universal)/19683*100:.2f}%")
    print(f"  Time:                              {elapsed:.1f}s")
    print()
    return universal


# ============================================================
# EXPERIMENT 2: Isomorphism Class Analysis
# ============================================================
def apply_value_perm(gate_tt, sigma):
    """Apply value permutation sigma to gate: g'(x,y) = sigma(g(sigma^-1(x), sigma^-1(y)))."""
    inv = [0] * 3
    for i, s in enumerate(sigma):
        inv[s] = i
    new_tt = [0] * 9
    for idx, (a, b) in enumerate(INPUT_PAIRS):
        orig_idx = inv[a] * 3 + inv[b]
        new_tt[idx] = sigma[gate_tt[orig_idx]]
    return tuple(new_tt)


def swap_inputs(gate_tt):
    """Swap two inputs: g'(x,y) = g(y,x)."""
    return tuple(gate_tt[b * 3 + a] for a, b in INPUT_PAIRS)


def canonical_form(gate_tt):
    """Canonical form under S3 value permutations x Z2 input swap."""
    forms = []
    for sigma in S3:
        t = apply_value_perm(gate_tt, sigma)
        forms.append(t)
        forms.append(swap_inputs(t))
    return min(forms)


def experiment_2(universal):
    """Group universal gates into isomorphism equivalence classes."""
    print("=" * 70)
    print("EXPERIMENT 2: Isomorphism Class Analysis")
    print("=" * 70)

    t0 = time.time()
    classes = defaultdict(list)
    for g in universal:
        c = canonical_form(g)
        classes[c].append(g)

    elapsed = time.time() - t0
    sizes = [len(v) for v in classes.values()]
    size_dist = Counter(sizes)

    print(f"  Total isomorphism classes: {len(classes)}")
    print(f"  Class size distribution:")
    for sz in sorted(size_dist.keys()):
        print(f"    Size {sz:2d}: {size_dist[sz]:4d} classes")
    print(f"  Max class size:           {max(sizes)}")
    print(f"  Mean class size:          {sum(sizes)/len(sizes):.2f}")
    print(f"  Time:                     {elapsed:.1f}s")
    print()

    representatives = [v[0] for v in classes.values()]
    return classes, representatives


# ============================================================
# EXPERIMENT 3: Property Analysis
# ============================================================
def experiment_3(universal):
    """Analyze structural properties of universal gates."""
    print("=" * 70)
    print("EXPERIMENT 3: Properties of Universal Gates")
    print("=" * 70)

    n = len(universal)

    # Commutativity: g(x,y) = g(y,x)
    n_comm = 0
    for g in universal:
        if all(g[a * 3 + b] == g[b * 3 + a] for a in T for b in T):
            n_comm += 1

    # Self-duality: NOT(g(x,y)) = g(NOT(x), NOT(y)) where NOT(v) = 2-v
    n_selfdual = 0
    for g in universal:
        if all(2 - g[a * 3 + b] == g[(2 - a) * 3 + (2 - b)] for a in T for b in T):
            n_selfdual += 1

    # Zero-preserving: g(0,0) = 0 (in balanced ternary: g(1,1) = 1 internally)
    n_zero = sum(1 for g in universal if g[4] == 1)

    # Surjectivity: output uses all three values
    n_surj = 0
    for g in universal:
        if len(set(g)) == 3:
            n_surj += 1

    # Output balance distribution
    balance_dist = Counter()
    for g in universal:
        counts = tuple(sorted([g.count(v) for v in T]))
        balance_dist[counts] += 1

    # Diagonal properties: g(x,x) behavior
    diag_types = Counter()
    for g in universal:
        diag = tuple(g[x * 3 + x] for x in T)
        if diag == (0, 1, 2):
            diag_types['identity'] += 1
        elif diag == (2, 1, 0):
            diag_types['negation'] += 1
        elif diag == (1, 2, 0):
            diag_types['increment'] += 1
        elif diag == (2, 0, 1):
            diag_types['decrement'] += 1
        elif len(set(diag)) == 1:
            diag_types['constant'] += 1
        else:
            diag_types['other'] += 1

    print(f"  Total universal gates:     {n}")
    print(f"  Commutative:               {n_comm} ({n_comm/n*100:.1f}%)")
    print(f"  Self-dual:                 {n_selfdual} ({n_selfdual/n*100:.1f}%)")
    print(f"  Zero-preserving:           {n_zero} ({n_zero/n*100:.1f}%)")
    print(f"  Surjective (all 3 outputs):{n_surj} ({n_surj/n*100:.1f}%)")
    print(f"\n  Output balance (sorted value counts in truth table):")
    for bal, cnt in sorted(balance_dist.items(), key=lambda x: -x[1]):
        print(f"    {bal}: {cnt} gates ({cnt/n*100:.1f}%)")
    print(f"\n  Diagonal g(x,x) behavior:")
    for dtype, cnt in sorted(diag_types.items(), key=lambda x: -x[1]):
        print(f"    {dtype:12s}: {cnt} ({cnt/n*100:.1f}%)")
    print()

    return {
        'total': n,
        'commutative': n_comm,
        'self_dual': n_selfdual,
        'zero_preserving': n_zero,
        'surjective': n_surj,
        'balance_dist': dict(balance_dist),
        'diagonal_types': dict(diag_types),
    }


# ============================================================
# EXPERIMENT 4: Depth-Optimal Half-Adder Synthesis
# ============================================================
def depth_search(gate_tt, targets, max_depth=7):
    """BFS by circuit depth to find minimum depth for target truth tables.

    Returns dict mapping each found target tuple to its minimum depth.
    """
    gl = list(gate_tt)  # flat lookup: gl[a*3+b]

    target_set = set(targets)
    results = {}

    known_codes = set()
    all_tts = []      # list of truth tables (tuples)
    prev_tts = []     # truth tables discovered at previous depth level

    # Depth 0: projections
    for tt in [PI1, PI2]:
        code = encode9(tt)
        if code not in known_codes:
            known_codes.add(code)
            all_tts.append(tt)
            prev_tts.append(tt)
            if tt in target_set:
                results[tt] = 0

    if len(results) == len(target_set):
        return results

    for d in range(1, max_depth + 1):
        new_tts = []

        # Compose: (all_tts x prev_tts) ∪ (prev_tts x all_tts)
        for f in all_tts:
            for g in prev_tts:
                code = (gl[f[0]*3+g[0]]         + gl[f[1]*3+g[1]] * 3    +
                        gl[f[2]*3+g[2]] * 9     + gl[f[3]*3+g[3]] * 27   +
                        gl[f[4]*3+g[4]] * 81    + gl[f[5]*3+g[5]] * 243  +
                        gl[f[6]*3+g[6]] * 729   + gl[f[7]*3+g[7]] * 2187 +
                        gl[f[8]*3+g[8]] * 6561)
                if code not in known_codes:
                    known_codes.add(code)
                    h = (gl[f[0]*3+g[0]], gl[f[1]*3+g[1]], gl[f[2]*3+g[2]],
                         gl[f[3]*3+g[3]], gl[f[4]*3+g[4]], gl[f[5]*3+g[5]],
                         gl[f[6]*3+g[6]], gl[f[7]*3+g[7]], gl[f[8]*3+g[8]])
                    new_tts.append(h)
                    if h in target_set:
                        results[h] = d

        for f in prev_tts:
            for g in all_tts:
                code = (gl[f[0]*3+g[0]]         + gl[f[1]*3+g[1]] * 3    +
                        gl[f[2]*3+g[2]] * 9     + gl[f[3]*3+g[3]] * 27   +
                        gl[f[4]*3+g[4]] * 81    + gl[f[5]*3+g[5]] * 243  +
                        gl[f[6]*3+g[6]] * 729   + gl[f[7]*3+g[7]] * 2187 +
                        gl[f[8]*3+g[8]] * 6561)
                if code not in known_codes:
                    known_codes.add(code)
                    h = (gl[f[0]*3+g[0]], gl[f[1]*3+g[1]], gl[f[2]*3+g[2]],
                         gl[f[3]*3+g[3]], gl[f[4]*3+g[4]], gl[f[5]*3+g[5]],
                         gl[f[6]*3+g[6]], gl[f[7]*3+g[7]], gl[f[8]*3+g[8]])
                    new_tts.append(h)
                    if h in target_set:
                        results[h] = d

        all_tts.extend(new_tts)
        prev_tts = new_tts

        if len(results) == len(target_set):
            break
        if not new_tts:
            break

    return results


def experiment_4(representatives, n_sample=30):
    """Find minimum depth for hsum and hcarry for representative universal gates."""
    print("=" * 70)
    print("EXPERIMENT 4: Half-Adder Synthesis (Depth-Optimal BFS)")
    print("=" * 70)

    targets = [HSUM, HCARRY]
    sample = representatives[:n_sample]

    all_results = []
    t0 = time.time()

    for i, g in enumerate(sample):
        t1 = time.time()
        res = depth_search(g, targets, max_depth=7)
        t2 = time.time()

        hsum_d = res.get(HSUM, -1)
        hcarry_d = res.get(HCARRY, -1)
        all_results.append({
            'gate_idx': encode9(g),
            'gate': g,
            'hsum_depth': hsum_d,
            'hcarry_depth': hcarry_d,
            'time': t2 - t1,
        })

        sys.stdout.write(f"\r  Gate {i+1}/{len(sample)}: D(hsum)={hsum_d}, D(hcarry)={hcarry_d}, "
                         f"generated {len(res)} targets, time={t2-t1:.1f}s")
        sys.stdout.flush()

    elapsed = time.time() - t0
    print()

    # Statistics
    valid = [r for r in all_results if r['hsum_depth'] >= 0 and r['hcarry_depth'] >= 0]
    hsum_depths = [r['hsum_depth'] for r in valid]
    hcarry_depths = [r['hcarry_depth'] for r in valid]

    print(f"\n  Tested {len(sample)} representative gates")
    print(f"  Gates with valid results: {len(valid)}/{len(sample)}")
    if hsum_depths:
        print(f"  hsum depth:  min={min(hsum_depths)}, max={max(hsum_depths)}, "
              f"mean={sum(hsum_depths)/len(hsum_depths):.2f}, "
              f"median={sorted(hsum_depths)[len(hsum_depths)//2]}")
        print(f"  hcarry depth: min={min(hcarry_depths)}, max={max(hcarry_depths)}, "
              f"mean={sum(hcarry_depths)/len(hcarry_depths):.2f}, "
              f"median={sorted(hcarry_depths)[len(hcarry_depths)//2]}")

        # Distribution
        print(f"\n  Depth distribution for hsum:")
        for d, cnt in sorted(Counter(hsum_depths).items()):
            print(f"    Depth {d}: {cnt} gates")
        print(f"  Depth distribution for hcarry:")
        for d, cnt in sorted(Counter(hcarry_depths).items()):
            print(f"    Depth {d}: {cnt} gates")

    print(f"\n  Total time: {elapsed:.1f}s")
    print()
    return all_results


# ============================================================
# EXPERIMENT 5: Formula Size (Gate Count) for Best Gates
# ============================================================
def formula_size_search(gate_tt, targets, max_size=40):
    """Find minimum formula size (tree complexity) for target truth tables.

    Formula size = number of gate applications in a tree circuit.
    Projections have size 0; compose(f, g) has size size_f + size_g + 1.
    """
    gl = list(gate_tt)

    target_codes = {encode9(t): t for t in targets}
    results = {}

    # by_size[s] = list of (code, truth_table) with formula size exactly s
    by_size = {}
    known = {}  # code -> min_size

    # Size 0: projections
    by_size[0] = []
    for tt in [PI1, PI2]:
        c = encode9(tt)
        if c not in known:
            known[c] = 0
            by_size[0].append((c, tt))
            if c in target_codes:
                results[c] = 0

    if len(results) == len(target_codes):
        return {target_codes[c]: s for c, s in results.items()}

    for s in range(1, max_size + 1):
        new_items = []
        # h = compose(f, g) with size_f + size_g + 1 = s
        for sf in range(s):
            sg = s - 1 - sf
            if sf not in by_size or sg not in by_size:
                continue
            for cf, f in by_size[sf]:
                for cg, g in by_size[sg]:
                    code = (gl[f[0]*3+g[0]]         + gl[f[1]*3+g[1]] * 3    +
                            gl[f[2]*3+g[2]] * 9     + gl[f[3]*3+g[3]] * 27   +
                            gl[f[4]*3+g[4]] * 81    + gl[f[5]*3+g[5]] * 243  +
                            gl[f[6]*3+g[6]] * 729   + gl[f[7]*3+g[7]] * 2187 +
                            gl[f[8]*3+g[8]] * 6561)
                    if code not in known:
                        known[code] = s
                        h = (gl[f[0]*3+g[0]], gl[f[1]*3+g[1]], gl[f[2]*3+g[2]],
                             gl[f[3]*3+g[3]], gl[f[4]*3+g[4]], gl[f[5]*3+g[5]],
                             gl[f[6]*3+g[6]], gl[f[7]*3+g[7]], gl[f[8]*3+g[8]])
                        new_items.append((code, h))
                        if code in target_codes:
                            results[code] = s

        by_size[s] = new_items

        if len(results) == len(target_codes):
            break

    return {target_codes[c]: s for c, s in results.items()}


def experiment_5(depth_results):
    """Compute formula sizes for the best gates by depth."""
    print("=" * 70)
    print("EXPERIMENT 5: Formula Size (Gate Count) for Best Gates")
    print("=" * 70)

    # Find gates with best (lowest) depth for hsum
    valid = [r for r in depth_results if r['hsum_depth'] >= 0 and r['hcarry_depth'] >= 0]
    if not valid:
        print("  No valid results to analyze")
        return []

    # Sort by combined full-adder metric: 2*D_hsum + D_hcarry
    valid.sort(key=lambda r: 2 * r['hsum_depth'] + r['hcarry_depth'])
    best_gates = valid[:5]  # top 5

    targets = [HSUM, HCARRY]
    size_results = []

    for i, r in enumerate(best_gates):
        g = r['gate']
        t1 = time.time()
        sizes = formula_size_search(g, targets, max_size=50)
        t2 = time.time()

        hsum_size = sizes.get(HSUM, -1)
        hcarry_size = sizes.get(HCARRY, -1)

        size_results.append({
            'gate_idx': r['gate_idx'],
            'gate': g,
            'hsum_depth': r['hsum_depth'],
            'hcarry_depth': r['hcarry_depth'],
            'hsum_size': hsum_size,
            'hcarry_size': hcarry_size,
            'time': t2 - t1,
        })

        print(f"  Gate {i+1}/5 (idx={r['gate_idx']}): "
              f"D(hsum)={r['hsum_depth']}, D(hcarry)={r['hcarry_depth']}, "
              f"Size(hsum)={hsum_size}, Size(hcarry)={hcarry_size}, "
              f"time={t2-t1:.1f}s")

    print()
    return size_results


# ============================================================
# EXPERIMENT 6: Binary NAND Baseline
# ============================================================
def experiment_6():
    """Compute depth and formula size for binary half-adder using NAND."""
    print("=" * 70)
    print("EXPERIMENT 6: Binary NAND Half-Adder Baseline")
    print("=" * 70)

    # Binary domain {0, 1}
    B_PAIRS = [(a, b) for a in [0, 1] for b in [0, 1]]
    NAND_TT = (1, 1, 1, 0)

    XOR_TT = (0, 1, 1, 0)  # half-adder sum
    AND_TT = (0, 0, 0, 1)  # half-adder carry

    gl = [0] * 4
    for i, (a, b) in enumerate(B_PAIRS):
        gl[a * 2 + b] = NAND_TT[i]

    # --- Depth BFS ---
    known_depth = {}
    all_tts = []
    prev_tts = []

    pi1_b = (0, 0, 1, 1)
    pi2_b = (0, 1, 0, 1)

    for tt in [pi1_b, pi2_b]:
        code = sum(tt[i] * (2 ** i) for i in range(4))
        known_depth[code] = 0
        all_tts.append(tt)
        prev_tts.append(tt)

    target_map = {XOR_TT: 'XOR', AND_TT: 'AND'}
    depth_results = {}
    for tt, name in target_map.items():
        code = sum(tt[i] * (2 ** i) for i in range(4))
        if code in known_depth:
            depth_results[name] = known_depth[code]

    for d in range(1, 10):
        new_tts = []
        for f in all_tts:
            for g in prev_tts:
                h = tuple(gl[f[i] * 2 + g[i]] for i in range(4))
                hc = sum(h[i] * (2 ** i) for i in range(4))
                if hc not in known_depth:
                    known_depth[hc] = d
                    new_tts.append(h)
                    if h in target_map and target_map[h] not in depth_results:
                        depth_results[target_map[h]] = d
        for f in prev_tts:
            for g in all_tts:
                h = tuple(gl[f[i] * 2 + g[i]] for i in range(4))
                hc = sum(h[i] * (2 ** i) for i in range(4))
                if hc not in known_depth:
                    known_depth[hc] = d
                    new_tts.append(h)
                    if h in target_map and target_map[h] not in depth_results:
                        depth_results[target_map[h]] = d
        all_tts.extend(new_tts)
        prev_tts = new_tts
        if len(depth_results) == 2:
            break
        if not new_tts:
            break

    # --- Formula size BFS ---
    by_size = {0: [(sum(tt[i] * (2 ** i) for i in range(4)), tt) for tt in [pi1_b, pi2_b]]}
    known_size = {c: 0 for c, _ in by_size[0]}
    size_results = {}

    for tt, name in target_map.items():
        c = sum(tt[i] * (2 ** i) for i in range(4))
        if c in known_size:
            size_results[name] = known_size[c]

    for s in range(1, 30):
        new_items = []
        for sf in range(s):
            sg = s - 1 - sf
            if sf not in by_size or sg not in by_size:
                continue
            for cf, f in by_size[sf]:
                for cg, g in by_size[sg]:
                    h = tuple(gl[f[i] * 2 + g[i]] for i in range(4))
                    hc = sum(h[i] * (2 ** i) for i in range(4))
                    if hc not in known_size:
                        known_size[hc] = s
                        new_items.append((hc, h))
                        if h in target_map and target_map[h] not in size_results:
                            size_results[target_map[h]] = s
        by_size[s] = new_items
        if len(size_results) == 2:
            break

    print(f"  Binary NAND half-adder:")
    print(f"    XOR (sum):  depth={depth_results.get('XOR', -1)}, "
          f"formula_size={size_results.get('XOR', -1)}")
    print(f"    AND (carry): depth={depth_results.get('AND', -1)}, "
          f"formula_size={size_results.get('AND', -1)}")
    print()
    return depth_results, size_results


# ============================================================
# EXPERIMENT 7: Full Adder Comparison
# ============================================================
def experiment_7(ternary_size_results, binary_depth, binary_size):
    """Compare ternary vs binary full adder via half-adder chaining."""
    print("=" * 70)
    print("EXPERIMENT 7: Full Adder — Ternary vs Binary Comparison")
    print("=" * 70)

    # Binary full adder metrics
    b_xor_d = binary_depth.get('XOR', -1)
    b_and_d = binary_depth.get('AND', -1)
    b_xor_s = binary_size.get('XOR', -1)
    b_and_s = binary_size.get('AND', -1)

    b_fa_sum_depth = 2 * b_xor_d
    b_fa_carry_depth = 2 * b_xor_d + b_and_d
    b_fa_gates = 3 * b_xor_s + 2 * b_and_s  # upper bound with sharing

    print(f"  Binary Full Adder (NAND, half-adder chaining):")
    print(f"    hsum (XOR): depth={b_xor_d}, gates={b_xor_s}")
    print(f"    hcarry (AND): depth={b_and_d}, gates={b_and_s}")
    print(f"    Full sum depth:    {b_fa_sum_depth}")
    print(f"    Full carry depth:  {b_fa_carry_depth}")
    print(f"    Full adder gates:  {b_fa_gates} (3×hsum + 2×hcarry)")
    print()

    # Ternary full adder metrics for each tested gate
    if not ternary_size_results:
        print("  No ternary results available")
        return

    print(f"  Ternary Full Adder (universal gate, half-adder chaining):")
    print(f"  {'Gate':>8s} | {'D(hs)':>5s} {'D(hc)':>5s} | {'S(hs)':>5s} {'S(hc)':>5s} | "
          f"{'FA_sum_D':>8s} {'FA_carry_D':>10s} {'FA_gates':>8s}")
    print(f"  {'-'*8}-+-{'-'*5}-{'-'*5}-+-{'-'*5}-{'-'*5}-+-{'-'*8}-{'-'*10}-{'-'*8}")

    best_depth = None
    best_gates = None

    for r in ternary_size_results:
        d_hs = r['hsum_depth']
        d_hc = r['hcarry_depth']
        s_hs = r['hsum_size']
        s_hc = r['hcarry_size']

        if d_hs < 0 or d_hc < 0 or s_hs < 0 or s_hc < 0:
            continue

        fa_sum_d = 2 * d_hs
        fa_carry_d = 2 * d_hs + d_hc
        fa_gates = 3 * s_hs + 2 * s_hc

        print(f"  {r['gate_idx']:>8d} | {d_hs:>5d} {d_hc:>5d} | {s_hs:>5d} {s_hc:>5d} | "
              f"{fa_sum_d:>8d} {fa_carry_d:>10d} {fa_gates:>8d}")

        if best_depth is None or fa_carry_d < best_depth[0]:
            best_depth = (fa_carry_d, fa_sum_d, fa_gates, r)
        if best_gates is None or fa_gates < best_gates[0]:
            best_gates = (fa_gates, fa_sum_d, fa_carry_d, r)

    print()

    if best_depth:
        print(f"  Best ternary by carry depth: gate {best_depth[3]['gate_idx']}")
        print(f"    Sum depth:   {best_depth[1]}")
        print(f"    Carry depth: {best_depth[0]}")
        print(f"    Gate count:  {best_depth[2]}")

    if best_gates:
        print(f"  Best ternary by gate count: gate {best_gates[3]['gate_idx']}")
        print(f"    Sum depth:   {best_gates[1]}")
        print(f"    Carry depth: {best_gates[2]}")
        print(f"    Gate count:  {best_gates[0]}")

    print()

    # Information density comparison
    bits_per_trit = math.log2(3)
    print(f"  Information Density:")
    print(f"    1 trit = {bits_per_trit:.4f} bits")
    print(f"    For N bits of information, need ceil(N/{bits_per_trit:.3f}) = ceil({1/bits_per_trit:.4f}·N) trits")
    print()

    if best_depth:
        t_carry_d = best_depth[0]
        t_gates = best_depth[2]

        print(f"  Ripple-Carry Adder Comparison (N-bit equivalent):")
        print(f"  {'N':>4s} | {'Bin_FAs':>7s} {'Bin_depth':>9s} {'Bin_gates':>9s} | "
              f"{'Ter_FAs':>7s} {'Ter_depth':>9s} {'Ter_gates':>9s} | "
              f"{'Depth_ratio':>11s} {'Gate_ratio':>10s}")
        print(f"  {'-'*4}-+-{'-'*7}-{'-'*9}-{'-'*9}-+-"
              f"{'-'*7}-{'-'*9}-{'-'*9}-+-{'-'*11}-{'-'*10}")

        for N in [4, 8, 16, 32, 64]:
            n_trits = math.ceil(N / bits_per_trit)

            bin_depth = N * b_fa_carry_depth
            bin_gates = N * b_fa_gates
            ter_depth = n_trits * t_carry_d
            ter_gates = n_trits * t_gates

            depth_ratio = ter_depth / bin_depth if bin_depth > 0 else float('inf')
            gate_ratio = ter_gates / bin_gates if bin_gates > 0 else float('inf')

            print(f"  {N:>4d} | {N:>7d} {bin_depth:>9d} {bin_gates:>9d} | "
                  f"{n_trits:>7d} {ter_depth:>9d} {ter_gates:>9d} | "
                  f"{depth_ratio:>11.3f} {gate_ratio:>10.3f}")

    print()


# ============================================================
# EXPERIMENT 8: Search Space & Memoisation Timing
# ============================================================
def experiment_8(universal):
    """Timing analysis: naive vs isomorphism-optimized universal gate search."""
    print("=" * 70)
    print("EXPERIMENT 8: Search Space Optimisation Timing")
    print("=" * 70)

    # Naive: check all 19,683 gates
    t0 = time.time()
    count_naive = 0
    for idx in range(19683):
        tt = decode9(idx)
        if check_unary_complete(tt):
            count_naive += 1
    t_naive = time.time() - t0

    # Optimised: check one per isomorphism class (of ALL gates, not just universal)
    t0 = time.time()
    all_classes = defaultdict(list)
    for idx in range(19683):
        tt = decode9(idx)
        c = canonical_form(tt)
        all_classes[c].append(tt)
    t_classify = time.time() - t0

    t0 = time.time()
    count_opt = 0
    for canon, members in all_classes.items():
        rep = members[0]
        if check_unary_complete(rep):
            count_opt += len(members)
    t_opt = time.time() - t0

    print(f"  Naive search:")
    print(f"    Gates checked: 19,683")
    print(f"    Universal found: {count_naive}")
    print(f"    Time: {t_naive:.2f}s")
    print(f"\n  Isomorphism-optimised search:")
    print(f"    Total iso classes: {len(all_classes)}")
    print(f"    Classification time: {t_classify:.2f}s")
    print(f"    Gates checked: {len(all_classes)} (one per class)")
    print(f"    Universal found: {count_opt}")
    print(f"    Search time: {t_opt:.2f}s")
    print(f"    Total time: {t_classify + t_opt:.2f}s")
    print(f"    Speedup (search only): {t_naive/t_opt:.1f}x")
    print(f"    Speedup (total):       {t_naive/(t_classify + t_opt):.1f}x")
    print()

    return {
        'naive_time': t_naive,
        'classify_time': t_classify,
        'opt_search_time': t_opt,
        'n_all_classes': len(all_classes),
    }


# ============================================================
# MAIN
# ============================================================
if __name__ == '__main__':
    print()
    print("╔══════════════════════════════════════════════════════════════════════╗")
    print("║  Ternary Logic Gate Experiments                                     ║")
    print("║  On Universality and Efficient Arithmetic Realisation               ║")
    print("╚══════════════════════════════════════════════════════════════════════╝")
    print()

    # Verify targets
    print("Half-adder truth tables (internal encoding {0,1,2} for {-1,0,+1}):")
    print(f"  HSUM   = {HSUM}  (decoded: {tuple(v-1 for v in HSUM)})")
    print(f"  HCARRY = {HCARRY}  (decoded: {tuple(v-1 for v in HCARRY)})")
    print()

    # Run experiments
    universal = experiment_1()
    classes, reps = experiment_2(universal)
    props = experiment_3(universal)
    timing = experiment_8(universal)

    # Depth search on representative gates
    depth_results = experiment_4(reps, n_sample=min(50, len(reps)))

    # Formula size for best gates
    size_results = experiment_5(depth_results)

    # Binary baseline
    binary_depth, binary_size = experiment_6()

    # Full comparison
    experiment_7(size_results, binary_depth, binary_size)

    print("╔══════════════════════════════════════════════════════════════════════╗")
    print("║  ALL EXPERIMENTS COMPLETE                                           ║")
    print("╚══════════════════════════════════════════════════════════════════════╝")

    # Save key results
    summary = {
        'n_universal': len(universal),
        'n_iso_classes': len(classes),
        'properties': {k: v for k, v in props.items() if not isinstance(v, dict)},
        'hsum_tt': list(HSUM),
        'hcarry_tt': list(HCARRY),
        'binary_nand': {
            'xor_depth': binary_depth.get('XOR', -1),
            'and_depth': binary_depth.get('AND', -1),
            'xor_size': binary_size.get('XOR', -1),
            'and_size': binary_size.get('AND', -1),
        },
    }

    if size_results:
        summary['best_ternary'] = [{
            'gate_idx': r['gate_idx'],
            'hsum_depth': r['hsum_depth'],
            'hcarry_depth': r['hcarry_depth'],
            'hsum_size': r['hsum_size'],
            'hcarry_size': r['hcarry_size'],
        } for r in size_results]

    with open('results.json', 'w') as f:
        json.dump(summary, f, indent=2)
    print("\nResults saved to results.json")
