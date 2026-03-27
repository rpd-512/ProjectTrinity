import sys
import csv

def load_file(path):
    data = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            count, gate = line.split(None, 1)
            data[gate] = int(count)
    return data

def main():
    if len(sys.argv) != 4:
        print("Usage: python subtract_gates.py file1.txt file2.txt output.csv")
        sys.exit(1)

    file1, file2, outfile = sys.argv[1], sys.argv[2], sys.argv[3]

    d1 = load_file(file1)  # file1 counts (used for subtraction)
    d2 = load_file(file2)  # sum_count

    missing = [g for g in d1 if g not in d2]
    if missing:
        print(f"Warning: {len(missing)} gate(s) in file1 not found in file2:")
        for g in missing[:10]:
            print(f"  {g}")
        if len(missing) > 10:
            print(f"  ... and {len(missing) - 10} more")

    rows = []
    for gate, c2 in d2.items():
        c1 = d1.get(gate)
        if c1 is None:
            continue
        rows.append({
            "sum_count":   c2,
            "carry_count": c1 - c2,
            "gate":        gate,
        })

    rows.sort(key=lambda r: (r["sum_count"], r["carry_count"]))

    with open(outfile, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["sum_count", "carry_count", "gate"])
        writer.writeheader()
        writer.writerows(rows)

    print(f"Written {len(rows)} entries to {outfile}")

if __name__ == "__main__":
    main()