#!/usr/bin/env python3
"""Reproduce a conservative instruction bound for the audited RV32I solver.

This is snapshot-specific static accounting, not a general WCET analyzer.
The accompanying RUBIK_ASSEMBLY_BOUND.md supplies the loop/invariant proof.
Source hashes fail closed if that proof's implementation changes.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess


EXPECTED_SHA256 = {
    "src/rubik/rubik.S": "29579e535aa5c53c2cc98122a72a57beef07dba032c3292f41ab49551868cdb6",
    "src/rubik/rubik_search.inc": "7e9e02102ab28bfce1ecc5e02af7eb6bb9129b7d3f1891780fb9a208721c4e77",
    "src/rubik/rubik_canon.inc": "347c48340bd64d4ae7f4d04ba09a534b223b1b133823c5f0264597c6850fe3dd",
    "src/rubik/rubik_tables.inc": "557fc6643035785adbd594aef6ad14cc2e9bd65dd5dfff5ff789593427344ff2",
}
FUNCTIONS = [
    "rubik_solve_asm", ".Lr48_move", ".Lr48_probe", ".Lr48_prefix",
    ".Lr48_meta", ".Lr48_ori", ".Lr48_sym", ".Lr48_T", ".Lr48_key",
]
EXPECTED_BACKEDGES = {
    "rubik_solve_asm": Counter({
        ".Lsolve_length": 1, ".Lsolve_parse": 1, ".Lsolve_sum": 1,
        ".Lsolve_search": 1, ".Lsolve_neighbor": 1, ".Lsolve_finish": 1,
        ".Lsolve_size_loop": 1, ".Lsolve_emit": 1,
    }),
    ".Lr48_move": Counter(),
    ".Lr48_probe": Counter({".Lprobe_binary": 2}),
    ".Lr48_prefix": Counter({".Lprefix_turn": 1, ".Lprefix_face": 1}),
    ".Lr48_meta": Counter(),
    ".Lr48_ori": Counter(),
    ".Lr48_sym": Counter(),
    ".Lr48_T": Counter({".Lr48_find_six": 1}),
    ".Lr48_key": Counter({".Lr48_meta_loop": 1, ".Lr48_key_loop": 1}),
}
EXPECTED_CALL_SITES = {
    "rubik_solve_asm": Counter({".Lr48_probe": 2, ".Lr48_prefix": 1,
                               ".Lr48_move": 1}),
    ".Lr48_move": Counter(),
    ".Lr48_probe": Counter({".Lr48_key": 1}),
    ".Lr48_prefix": Counter({".Lr48_probe": 1, ".Lr48_move": 1,
                             ".Lr48_prefix": 1}),
    ".Lr48_meta": Counter(),
    ".Lr48_ori": Counter(),
    ".Lr48_sym": Counter(),
    ".Lr48_T": Counter(),
    ".Lr48_key": Counter({".Lr48_T": 7, ".Lr48_sym": 7,
                          ".Lr48_meta": 1, ".Lr48_ori": 2}),
}


def require(condition, message):
    if not condition:
        raise SystemExit("Bound audit failed: " + message)


def numeric_tables(text):
    tables = {}
    name = None
    for line in text.splitlines():
        match = re.match(r"\.Ldata_(\w+):", line)
        if match:
            name = match[1]
            tables[name] = []
        match = re.match(r"\s*\.(?:byte|2byte|4byte)\s+(.+)", line)
        if match and name and not match[1].startswith(".L"):
            tables[name].extend(int(x.strip(), 0) for x in match[1].split(","))
    return tables


def check_tables(t):
    require(sorted(t["new_to_old"]) == list(range(7)), "input map")
    require(t["pop7"] == [i.bit_count() for i in range(128)], "popcount table")
    require(t["mod3_lut"] == [i % 3 for i in range(13)], "modulo table")
    for i in range(7):
        require(t["t_remtab"][6*i:6*i+6] == [j for j in range(7) if i != j],
                "remaining-position table")
    for name, rows in (("n2_b8_move_src", 9), ("n2_b8_sym_pos", 6)):
        require(len(t[name]) == rows * 8, name + " length")
        for i in range(rows):
            row = t[name][8*i:8*i+8]
            require(sorted(row) == list(range(8)) and row[7] == 7,
                    name + " must preserve a permutation and the fixed corner")
    require(len(t["n2_b8_move_delta"]) == 72 and
            all(0 <= x <= 2 for x in t["n2_b8_move_delta"]), "move twists")
    require(len(t["n2_b8_sym_inv"]) == 6 and
            all(0 <= x < 6 for x in t["n2_b8_sym_inv"]), "symmetry inverse")
    require(len(t["n2_b8_sym_twist"]) == 1152 and
            all(0 <= x <= 2 for x in t["n2_b8_sym_twist"]), "symmetry twists")
    require(t["n2_b8_sym_pos"][:8] == list(range(8)) and
            t["n2_b8_sym_inv"][0] == 0 and
            t["n2_b8_sym_twist"][:192] == [i % 3 for i in range(192)],
            "identity-symmetry specialization")
    require(len(t["h48_t_dst"]) == 21 and
            all((x & 7) < 7 and (x >> 3) < 3 for x in t["h48_t_dst"]),
            "T destinations")
    require(len(t["h48_t_perm_R"]) == 126, "T permutation table length")
    for i in range(21):
        require(sorted(t["h48_t_perm_R"][6*i:6*i+6]) == list(range(6)),
                "T must permute the six non-distinguished pieces")
    require(len(t["h48_t_ori_ctl"]) == 21, "T orientation table length")
    for word in t["h48_t_ori_ctl"]:
        for i in range(5):
            item = (word >> (5*i)) & 31
            selector = item & 7
            require(selector in (0, 1, 2, 3, 4, 7), "T scratch-slot selector")
            require((item >> 3) + (10 if selector == 7 else 4) <= 12,
                    "T modulo-table access")
    require(len(t["n2_b8_pmeta"]) == 5040 and
            all((m >> 3) < 870 and (m & 7) < 6 for m in t["n2_b8_pmeta"]),
            "permutation metadata ranges")
    require(len(t["h48_common_base"]) == 814 and
            len(t["h48_exc_base"]) == len(t["h48_exc_action"]) == 56,
            "orbit table lengths")
    require(all(0 <= x < 14 for x in t["h48_exc_action"]) and
            len(t["h48_action_rank"]) == 14 * 729, "exception action ranges")
    starts, entries = t["h48_b8_start"], t["h48_b8_entry"]
    require(len(starts) == 1198 and starts[0] == 0 and
            starts[-1] == len(entries) and
            all(0 <= a <= b <= len(entries) for a, b in zip(starts, starts[1:])),
            "radius-eight bucket ranges")
    require(all(0 <= (x >> 9) & 15 <= 8 for x in entries),
            "a probe may return only a distance from zero through eight")
    return {"buckets": 1197, "entries": len(entries),
            "max_bucket_length": max(b-a for a, b in zip(starts, starts[1:])),
            "maximum_stored_distance": max((x >> 9) & 15 for x in entries),
            "state_preservation_checks": "passed"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", default="riscv-none-elf-")
    parser.add_argument("--out", default="results/rubik_instruction_bound.json")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    for name, expected in EXPECTED_SHA256.items():
        actual = hashlib.sha256((repo / name).read_bytes()).hexdigest()
        require(actual == expected, name + " changed; its loop proof needs review")
    tables = check_tables(numeric_tables((repo / "src/rubik/rubik_tables.inc").read_text()))
    build = repo / "build/mytests/rubik_bound"
    build.mkdir(parents=True, exist_ok=True)

    def run(tool, *arguments):
        return subprocess.run([args.prefix + tool, *map(str, arguments)],
                              cwd=repo, check=True, text=True,
                              stdout=subprocess.PIPE).stdout

    obj = build / "rubik_bound.o"
    run("as", "-L", "-march=rv32i_zicsr", "-mabi=ilp32",
        "src/rubik/rubik.S", "-o", obj)
    dis = run("objdump", "-dr", "-M", "no-aliases", obj)
    sym = run("readelf", "-sW", obj)
    (build / "rubik_bound.dis").write_text(dis)
    (build / "rubik_bound.symbols").write_text(sym)
    labels, sizes = {}, {}
    for line in sym.splitlines():
        m = re.match(r"\s*\d+:\s*([0-9a-f]+)\s+(\d+)\s+\S+\s+\S+\s+\S+\s+(\d+)\s+(\S+)", line)
        if m and m[3] == "1":
            labels[m[4]], sizes[m[4]] = int(m[1], 16), int(m[2])
    instructions, relocations = {}, []
    for line in dis.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+(\S+)\s*(.*)", line)
        if m:
            instructions[int(m[1], 16)] = (m[2], m[3])
        m = re.match(r"\s*([0-9a-f]+):\s+R_RISCV_(CALL_PLT|CALL|JAL)\s+(\S+)", line)
        if m:
            relocations.append((int(m[1], 16), m[2], m[3]))
    ranges, counts, call_sites, backedges = {}, {}, {}, {}
    for i, name in enumerate(FUNCTIONS):
        lo = labels[name]
        hi = labels[FUNCTIONS[i+1]] if i+1 < len(FUNCTIONS) else lo + sizes[name]
        ranges[name] = (lo, hi)
        local = {a: ins for a, ins in instructions.items() if lo <= a < hi}
        require(len(local)*4 == hi-lo, name + " has unaccounted instruction bytes")
        counts[name] = len(local)
        calls = Counter()
        for address, kind, target in relocations:
            if lo <= address < hi:
                op, rest = instructions[address]
                if kind in ("CALL", "CALL_PLT") or (op == "jal" and rest.startswith("ra,")):
                    calls[target] += 1
        require(calls == EXPECTED_CALL_SITES[name], name + " call graph changed")
        call_sites[name] = dict(calls)
        backward = Counter()
        for address, (op, rest) in local.items():
            if op in ("beq", "bne", "blt", "bge", "bltu", "bgeu") or (op == "jal" and rest.startswith("zero,")):
                m = re.search(r"([0-9a-f]+)\s+<([^>]+)>", rest)
                require(m is not None, "unresolved control-flow destination")
                destination = int(m[1], 16)
                require(lo <= destination < hi, "unexpected tail branch")
                if op != "jal":
                    require(-4096 <= destination-address < 4096,
                            "conditional branch needs a long expansion")
                if destination <= address:
                    backward[m[2]] += 1
        require(backward == EXPECTED_BACKEDGES[name], name + " loop structure changed")
        backedges[name] = dict(backward)

    # Each non-call instruction in a straight-line kernel is visited at most
    # once. T has only the bounded search for the position of piece six.
    find_six_body = (labels[".Lr48_found_six"] - labels[".Lr48_find_six"]) // 4
    local_T = counts[".Lr48_T"] + 6 * find_six_body
    key_local = 8 * counts[".Lr48_key"]
    # Both candidate branches are charged, though only one can execute.
    key_total = (key_local + 7*local_T + 14*counts[".Lr48_sym"] +
                 8*counts[".Lr48_meta"] + 16*counts[".Lr48_ori"])
    # lhu bounds the lower_bound interval by 65535. Its length halves on each
    # body execution: <=16 bodies and <=17 evaluations of its loop condition.
    probe_local = 17 * counts[".Lr48_probe"]
    prefix_local = 9 * counts[".Lr48_prefix"]
    root_local = 72 * counts["rubik_solve_asm"]
    move_local = counts[".Lr48_move"]
    leaves = [9, 54, 324]
    frames = [1+sum(leaves[:i]) for i in range(1, 4)]
    prefix_frames = sum(frames)
    prefix_edges = sum(x-1 for x in frames)
    suffix_probes = 7*9
    suffix_moves = 8*9
    probes = 1 + sum(leaves) + suffix_probes
    moves = prefix_edges + suffix_moves
    derived = (probes*(key_total+probe_local) + prefix_frames*prefix_local +
               moves*move_local + root_local)
    require(key_total <= 10000 and probe_local <= 1000 and prefix_local <= 1024
            and move_local <= 128 and root_local <= 16384, "rounded component limit")
    require(probes <= 451 and prefix_frames <= 512 and moves <= 1024,
            "rounded call-count limit")
    published = 451*(10000+1000) + 512*1024 + 1024*128 + 16384
    require(derived <= published < 7500000, "instruction budget")
    result = {
        "kind": "snapshot-specific static upper-bound proof accounting",
        "scope": "rubik_solve_asm call tree, not C, startup, test harness, or host wall time",
        "source_sha256": EXPECTED_SHA256,
        "assembler": run("as", "--version").splitlines()[0],
        "data_invariants": tables,
        "static_instruction_counts": counts,
        "backedge_targets": backedges,
        "static_call_sites": call_sites,
        "dynamic_count_bounds": {
            "prefix_leaves_by_depth": leaves, "prefix_frames_by_depth": frames,
            "prefix_frames_total": prefix_frames, "prefix_edges": prefix_edges,
            "suffix_probes": suffix_probes, "suffix_moves": suffix_moves,
            "all_probes": probes, "all_moves": moves,
        },
        "component_instruction_bounds": {
            "T_including_find_six": local_T, "canonical_key_local": key_local,
            "canonical_key_including_callees": key_total,
            "probe_excluding_key": probe_local,
            "prefix_frame_excluding_callees": prefix_local,
            "move": move_local, "root_excluding_callees": root_local,
        },
        "derived_call_tree_upper_bound": derived,
        "published_rounded_upper_bound": published,
        "instruction_budget": 7500000,
        "headroom_below_budget": 7500000-published,
        "maximum_solver_stack_bytes": 128 + 4*80 + 16 + 192 + 16,
        "status": "PASS",
    }
    output = Path(args.out)
    if not output.is_absolute():
        output = repo / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in (
        "derived_call_tree_upper_bound", "published_rounded_upper_bound",
        "instruction_budget", "maximum_solver_stack_bytes", "status")}, indent=2))
    print("Evidence:", output)


if __name__ == "__main__":
    main()
