#!/usr/bin/env python3
import csv
import pathlib
import sys

paths = [pathlib.Path(p) for p in sys.argv[1:]]
rows = []
for path in paths:
    with path.open(newline="") as f:
        rows.extend(csv.DictReader(f, delimiter="\t"))

def num(row, key):
    return float(row[key])

def one(cpu, kernel, variant, *, size=None, terms=None):
    found = []
    for r in rows:
        if r["cpu"] != cpu or r["kernel"] != kernel or r["variant"] != variant:
            continue
        if size is not None and int(r["size"]) != size:
            continue
        if terms is not None and int(r["terms"]) != terms:
            continue
        found.append(r)
    if len(found) != 1:
        raise SystemExit(f"expected one row: {cpu=} {kernel=} {variant=} {size=} {terms=}, got {len(found)}")
    return found[0]

cpus = sorted({r["cpu"] for r in rows})
out = []
for cpu in cpus:
    base_poly = one(cpu, "poly-frame", "f64-scalar", terms=24)
    base_poly_ns = num(base_poly, "ns_per_unit")
    poly_variants = sorted({
        r["variant"] for r in rows if r["cpu"] == cpu and r["kernel"] == "poly-frame"
    })
    for variant in poly_variants:
        r24 = one(cpu, "poly-frame", variant, terms=24)
        candidates = [
            r for r in rows
            if r["cpu"] == cpu and r["kernel"] == "poly-frame"
            and r["variant"] == variant and num(r, "ns_per_unit") <= base_poly_ns
        ]
        max_terms = max((int(r["terms"]) for r in candidates), default=0)
        out.append({
            "cpu": cpu,
            "kernel": "poly-frame",
            "variant": variant,
            "baseline": "f64-scalar/24-terms",
            "speedup_at_baseline_shape": base_poly_ns / num(r24, "ns_per_unit"),
            "headroom": f"{max_terms} terms measured under baseline budget",
            "error_at_baseline_shape": num(r24, "error"),
            "bytes_per_component": int(r24["bytes_per_component"]),
        })

    base_fft = one(cpu, "fft", "f64-scalar", size=1024)
    base_fft_ns = num(base_fft, "ns_per_unit")
    fft_variants = sorted({
        r["variant"] for r in rows if r["cpu"] == cpu and r["kernel"] == "fft"
    })
    for variant in fft_variants:
        r1024 = one(cpu, "fft", variant, size=1024)
        candidates = [
            r for r in rows
            if r["cpu"] == cpu and r["kernel"] == "fft"
            and r["variant"] == variant and num(r, "ns_per_unit") <= base_fft_ns
        ]
        max_n = max((int(r["size"]) for r in candidates), default=0)
        out.append({
            "cpu": cpu,
            "kernel": "fft",
            "variant": variant,
            "baseline": "f64-scalar/N=1024",
            "speedup_at_baseline_shape": base_fft_ns / num(r1024, "ns_per_unit"),
            "headroom": f"N={max_n} measured under baseline budget",
            "error_at_baseline_shape": num(r1024, "error"),
            "bytes_per_component": int(r1024["bytes_per_component"]),
        })

fieldnames = [
    "cpu", "kernel", "variant", "baseline",
    "speedup_at_baseline_shape", "headroom",
    "error_at_baseline_shape", "bytes_per_component"
]
writer = csv.DictWriter(sys.stdout, fieldnames=fieldnames, delimiter="\t", lineterminator="\n")
writer.writeheader()
for r in out:
    r = dict(r)
    r["speedup_at_baseline_shape"] = f"{r['speedup_at_baseline_shape']:.3f}"
    r["error_at_baseline_shape"] = f"{r['error_at_baseline_shape']:.9g}"
    writer.writerow(r)
