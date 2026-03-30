import math

# ================================
# Constants
# ================================
T = [0, 1, 2]
INPUT_PAIRS = [(a, b) for a in T for b in T]
POW3 = [3**i for i in range(9)]

PI1 = tuple(a for a, b in INPUT_PAIRS)
PI2 = tuple(b for a, b in INPUT_PAIRS)

# ================================
# Encode / Decode
# ================================
def encode9(tt):
    return sum(tt[i] * POW3[i] for i in range(9))

def decode9(code):
    tt = []
    for _ in range(9):
        tt.append(code % 3)
        code //= 3
    return tuple(tt)

# ================================
# Half Adder
# ================================
def compute_half_adder():
    T_BAL = [-1, 0, 1]
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
            hsum.append(s + 1)
            hcarry.append(c + 1)
    return tuple(hsum), tuple(hcarry)

HSUM, HCARRY = compute_half_adder()

# ================================
# Depth Search (BFS)
# ================================
def depth_search(gate_tt, targets, max_depth=7):
    gl = list(gate_tt)

    known = {}
    all_tts = []
    prev = []

    for tt in [PI1, PI2]:
        c = encode9(tt)
        known[c] = 0
        all_tts.append(tt)
        prev.append(tt)

    results = {}

    for d in range(1, max_depth+1):
        new = []

        for f in all_tts:
            for g in prev:
                h = tuple(gl[f[i]*3 + g[i]] for i in range(9))
                c = encode9(h)
                if c not in known:
                    known[c] = d
                    new.append(h)
                    if h in targets:
                        results[h] = d

        for f in prev:
            for g in all_tts:
                h = tuple(gl[f[i]*3 + g[i]] for i in range(9))
                c = encode9(h)
                if c not in known:
                    known[c] = d
                    new.append(h)
                    if h in targets:
                        results[h] = d

        all_tts.extend(new)
        prev = new

        if len(results) == len(targets):
            break

    return results

# ================================
# Formula Size Search + Expression
# ================================
def formula_size_search(gate_tt, targets, max_size=40):
    gl = list(gate_tt)

    known = {}   # code -> (size, expression)
    by_size = {0: []}
    results = {}

    # projections
    for name, tt in [("x", PI1), ("y", PI2)]:
        c = encode9(tt)
        known[c] = (0, name)
        by_size[0].append((c, tt))

    for s in range(1, max_size+1):
        new_items = []

        for sf in range(s):
            sg = s - 1 - sf
            if sf not in by_size or sg not in by_size:
                continue

            for cf, f in by_size[sf]:
                for cg, g in by_size[sg]:

                    h = tuple(gl[f[i]*3 + g[i]] for i in range(9))
                    c = encode9(h)

                    if c not in known:
                        expr_f = known[cf][1]
                        expr_g = known[cg][1]

                        expr = f"G({expr_f}, {expr_g})"

                        known[c] = (s, expr)
                        new_items.append((c, h))

                        if h in targets:
                            results[h] = (s, expr)

        by_size[s] = new_items

        if len(results) == len(targets):
            break

    return results

# ================================
# MAIN
# ================================
if __name__ == "__main__":
    gate_idx = 451  # change this
    gate = decode9(gate_idx)
    print(gate)
    print(f"Gate index: {gate_idx}")
    print(f"Truth table: {gate}")

    targets = [HSUM, HCARRY]

    print("\nComputing depth...")
    depth_res = depth_search(gate, targets)

    print("Computing size + formula...")
    size_res = formula_size_search(gate, targets)

    Ds = depth_res.get(HSUM, -1)
    Dc = depth_res.get(HCARRY, -1)

    Gs, expr_s = size_res.get(HSUM, (-1, "NA"))
    Gc, expr_c = size_res.get(HCARRY, (-1, "NA"))

    DFA = 2*Ds + Dc if Ds >= 0 and Dc >= 0 else -1
    GFA = 3*Gs + 2*Gc if Gs >= 0 and Gc >= 0 else -1

    print("\n=== RESULT ===")
    print(f"D_s = {Ds}, D_c = {Dc}")
    print(f"G_s = {Gs}")
    print(f"Formula (SUM): {expr_s}\n")

    print(f"G_c = {Gc}")
    print(f"Formula (CARRY): {expr_c}\n")

    print(f"D_FA = {DFA}")
    print(f"G_FA = {GFA}")