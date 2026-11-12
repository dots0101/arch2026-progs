# A static instruction upper bound for the RV32I solver

For the source snapshot checked by `rubik_bound.py`, every execution of
`rubik_solve_asm` under its ordinary valid-memory API preconditions retires
at most **5,632,744 instructions inside its call tree**. This is below the
7,500,000-instruction assembly budget. It is a conservative bound from the
implementation's finite control flow, not an extrapolation from the measured
distance-11 states. It does not assert a corresponding bound for the C
implementation, physical CPU cycles, whole-test-ELF instructions, or host
wall-clock time.

Run from the repository root:

```sh
python3 mytests/rubik_bound.py --prefix riscv-none-elf- \
    --out results/rubik_instruction_bound.json
```

The script assembles an independent object with local symbols retained,
disassembles the expanded RV32I instructions, checks all call sites and
backedges, validates the table invariants used below, and writes the
accounting as JSON. The proof is specific to the four source SHA-256 hashes
embedded in the script and recorded in the JSON. A source change causes a
failure and requires a fresh review of the proof. This is not a general
automatic worst-case execution-time analyzer.

## 1. State invariants needed for termination

Input parsing either returns immediately or constructs a permutation of
pieces 0 through 6 with piece 7 fixed, with each orientation in 0 through 2.
The coordinate map is a permutation of 0 through 6. The string scan examines
at most fifteen bytes, the parsing loop at most seven positions, and the
orientation-sum reduction starts with a value at most fourteen.

For the instruction bound, the internal invariants needed are a permutation
of the movable pieces and individual orientations in 0 through 2. The script
checks the following properties of the actual immutable tables:

- Every move source row and symmetry-position row is a permutation fixing
  position 7. All stored move deltas and symmetry twist results are in 0..2.
- Every T destination position is in 0..6 and its twist is in 0..2. Every T
  permutation row permutes six entries. Removing piece 6 leaves pieces 0..5;
  XOR with one permutes that set, so T restores a permutation after inserting
  piece 6 at the new destination. The remaining-position tables have the
  required six distinct positions.
- All T orientation selectors address initialized scratch slots. Their
  maximum modulo-table index is twelve, and the table contains exactly the
  residues for indices zero through twelve. The final orientation sum is
  also at most twelve. Thus every written orientation remains in 0..2.
- Metadata selects only the six symmetries and the 870 orbit entries. The
  exceptional ranks have the stated array sizes. Identity-symmetry fast
  paths agree with all entries of the original identity tables.
- The B8 start table is monotone, has 1,198 entries for 1,197 buckets, and
  ends at the 24,793-element entry table. Every distance field is at most
  eight. The largest actual bucket has 205 elements.

In particular, the T kernel's search for piece 6 cannot continue indefinitely:
piece 6 is present in positions 0..6, so at most seven iterations occur. These
checks establish the invariants required by this bound; correctness of the
Rubik model and exact B8 distances is established separately by the full
state-graph checker described in `README.md`.

## 2. Number of search calls

There are nine moves at the root. Once a face has been used, its three turns
are excluded from the next level, leaving six moves. A complete traversal at
prefix depth d therefore has `9 * 6^(d-1)` leaves for d greater than zero.
The outer solver tries depths one, two, and three. Successful early exits can
only reduce these counts.

| Requested prefix depth | Leaf probes | All prefix frames, including depth zero | Move edges |
| ---: | ---: | ---: | ---: |
| 1 | 9 | 1 + 9 = 10 | 9 |
| 2 | 54 | 1 + 9 + 54 = 64 | 63 |
| 3 | 324 | 1 + 9 + 54 + 324 = 388 | 387 |
| Total | 387 | 462 | 459 |

A successful B8 lookup supplies a distance at most eight. Reconstruction
decreases that distance by exactly one at each accepted step and returns an
error if no acceptable neighbor exists. It can therefore examine at most
`8 * 9 = 72` neighbor moves. When the current distance is one, the assembly
compares the successor directly with the solved state instead of invoking
the canonicalizer. Consequently at most `7 * 9 = 63` suffix probes occur.
The initial root probe adds one, giving:

```
all probes       <= 1 + 387 + 63 = 451
all prefix frames <= 462
all move calls   <= 459 + 72 = 531
```

The finite depth-three bound does not assume that every input succeeds:
exhausting all three depths returns an error and still obeys the same upper
bound. Optimality and completeness follow from the separate exact graph/B8
argument; they are not prerequisites for bounding the number of calls.

## 3. Static instructions and local execution bounds

Each assembler pseudo-instruction is counted after expansion. An unresolved
`call` is charged as its two-instruction form; linker relaxation can shorten
it. Conditional branches stay within their native range and all callees are
in the same small assembly text region, so no longer branch sequence is
introduced when linking this object.

| Routine | Static instructions in its complete text | Conservative bound for local instructions per invocation, excluding callees |
| --- | ---: | ---: |
| Public wrapper | 207 | 207 × 72 = 14,904 |
| Move | 83 | 83 |
| B8 probe | 43 | 43 × 17 = 731 |
| Prefix frame | 62 | 62 × 9 = 558 |
| Permutation metadata | 70 | 70 |
| Orientation rank | 125 | 125 |
| Symmetry | 133 | 133 |
| T transform | 212 | 212 + 6 × 5 = 242 |
| Canonical key | 135 | 135 × 8 = 1,080 |

These intentionally count instructions on mutually exclusive paths. The
orientation and symmetry text includes both the identity fast path and the
general path, although only one executes.

The local multipliers are justified as follows:

- In the public wrapper, no instruction is visited more than 72 times: the
  largest repetition is the at-most-eight reconstruction levels times nine
  neighbors. Parsing needs at most fifteen scan iterations, seven parse
  iterations and five sum-loop condition evaluations; prefix depth is tried
  three times; reconstruction's outer condition is visited at most nine
  times; and each output loop has at most eleven iterations.
- A prefix frame has at most nine child edges. Its three face selections,
  turn loop and all remaining local instructions are therefore each visited
  at most nine times. Callee work, including recursive frames, is accounted
  for separately.
- A probe's lower and upper indices are loaded as unsigned 16-bit values.
  Its lower-bound interval can be no longer than 65,535 even without using
  the much smaller actual maximum bucket. Each iteration reduces the length
  to at most half. There are at most sixteen bodies and seventeen loop
  condition evaluations. Charging every local instruction seventeen times
  is conservative.
- The T kernel has only one loop. Its five-instruction piece-search body is
  already counted once in the text size and is charged six additional times.
  Its remaining small loops were expanded by the assembler.
- The canonical-key kernel has two separate loops of eight iterations. No
  instruction belongs to both, so every local instruction executes at most
  eight times. All other routines in the table have no backedges.

## 4. A canonical-key call, including all of its helpers

State construction calls T seven times and symmetry six times. Metadata is
computed eight times. At most eight orientation candidates are then examined.
For each candidate the following bound charges **both** the common and
exceptional branches: two orientation-rank calls and one symmetry call.
Only one branch actually executes, so this is an overestimate.

```
key <= 1080 + 7*242 + 6*133 + 8*70 + 8*(2*125 + 133)
    = 7,196 instructions
```

The checked rounded ceilings are 10,000 instructions for the complete key,
1,000 for the probe excluding the key, 1,024 for a prefix frame excluding
callees, 128 for a move, and 16,384 for the wrapper excluding callees.

## 5. Complete upper bound

Combining the tighter values above gives the reproducible conservative
call-tree bound:

```
451*(7196+731) + 462*558 + 531*83 + 14904 = 3,891,850
```

For the reported bound, round the component limits upward and also round
462 frames to 512 and 531 moves to 1,024:

```
451*(10000+1000) + 512*1024 + 1024*128 + 16384
    = 5,632,744 < 7,500,000
```

This leaves **1,867,256 instructions** below the required limit. Both figures
are upper bounds, not predictions of an actual worst-case input. The measured
878,484 maximum over all 2,644 distance-11 states is useful separate evidence;
it is not substituted for this all-input control-flow argument. Measurements
in `PERF_CALL` can also include a few caller instructions; the published
rounding margin is far larger than that effect in the supplied harness.

## 6. Solver stack

At prefix depth three there are four live prefix frames, for remaining depths
three, two, one, and zero. At the deepest leaf, the call chain is:

```
wrapper 128 + four prefix frames 4*80 + probe 16 + key 192 + T 16 = 672 bytes
```

The remaining canonical helpers and move routine are leaves with no stack.
Every frame size is a multiple of sixteen. The 672-byte bound concerns the
solver's call tree; startup and test-harness frames are separate. It fits
comfortably within the official 4 KiB stack reservation together with the
actual supplied harness, which also completed its target tests.
