# Independent supplementary checks

These are additional checks, kept outside `src/`. They do not replace or
modify the official contracts, grading harness, budgets, or test vectors.
None of the full-state oracle data is linked into a submitted solver.
The C and directly authored assembly implementations are AI-assisted work;
these files do not assert that the student personally authored them.

Run from the repository root. A host C compiler and Python 3 are sufficient
for the first and third commands. The target check also needs the cross
compiler and the system-emulation rv32emu built by the official project.
The output directories below are siblings of the repository to keep generated
binaries and full-state tables out of commits.

```sh
mytests/run_rubik_oracle.sh ../evidence/rubik-oracle
CROSS_COMPILE=riscv-none-elf- mytests/run_rubik_d11_target.sh ../evidence/rubik-d11
mytests/run_fixed_error.sh ../evidence/fixed-error
```

If the compiler uses a different prefix, set `CROSS_COMPILE` accordingly.
An absolute prefix is also supported. `RV32EMU` can select an existing system
emulator. The default is `third_party/rv32emu/build/rv32emu`. These scripts do
not rebuild the emulator or remove the shared `build/` tree.

## Rubik: independent state graph

`rubik_oracle.c` implements the instructor's published R/B/D source and twist
tables directly, with no call into the H48 implementation while constructing
the reference graph. A state consists of a permutation of seven movable
corners and seven twists in Z/3 whose sum is zero. The seventh twist is implied
by the first six. Hence there are

`7! * 3^6 = 5040 * 729 = 3,674,160`

legal states. The reference uses a factorial permutation rank and a base-three
orientation rank. It verifies every component rank/decode round trip and
every move/inverse pair. It precomputes 5040 by 9 permutation transitions and
729 by 9 orientation transitions from the published face tables. BFS from
the identity then examines all 33,067,440 directed edges, discovering every
state. Unit-length BFS in this reversible graph gives exact HTM distances.

The measured distance histogram is:

| Distance | States |
|---:|---:|
| 0 | 1 |
| 1 | 9 |
| 2 | 54 |
| 3 | 321 |
| 4 | 1,847 |
| 5 | 9,992 |
| 6 | 50,136 |
| 7 | 227,536 |
| 8 | 870,072 |
| 9 | 1,887,748 |
| 10 | 623,800 |
| 11 | 2,644 |

The coordinate conjugacy from the official convention to the previous H48
implementation is `[3,1,5,2,0,4,6]`, applied to both positions and cubie labels.
Twists are unchanged. Official move indices R,R2,R',B,B2,B',D,D2,D' map to old
indices `[2,1,0,8,7,6,5,4,3]`. The checker tests that conjugacy for every
permutation and every orientation component under all nine moves.

After constructing the independent reference, the checker includes the C
implementation solely to inspect its private `b8_probe`. For **all 3,674,160
states**, it requires that the probe succeeds if and only if the independent
distance is at most eight, and that every returned distance agrees exactly.
The completed run found **zero mismatches**.

It then calls the public `rubik_solve` on **all 2,644 distance-11 states**.
It parses each returned move string, replays it using the independent official
move tables, and requires that the exact BFS distance decrease by one after
every move, ending at the goal after exactly eleven moves. All cases passed.
The generated `rubik_d11.csv` contains each input, output and work count;
`rubik_worst_probe_states.csv` retains forty demanding states. The generated
case include is compared with the checked-in `rubik_d11_cases.inc`.

## Why a prefix of at most three is sufficient and optimal

The full graph has diameter eleven, and the verified B8 predicate is exact.
For a state with distance d greater than eight, take a shortest path. After
exactly d-8 moves it reaches distance eight. The required prefix length is
therefore at most three. A shortest path never has two consecutive turns of
the same face: they combine into one allowed turn or cancel. Excluding such
adjacent face repetitions consequently excludes no shortest solution.

The solver checks prefix lengths in increasing order. If it entered B8 with a
shorter prefix than d-8, the prefix followed by a B8 solution would contradict
the independent distance d. Once it enters B8, following neighbors whose
exact distance decreases by one yields an optimal suffix. Thus the finite
radius-eight search, rather than a full-state lookup on the target, suffices.

The C statistics count prefix probes, not the successful suffix reconstruction.
For the existing C reconstruction, the total adds `move_index+1` for each
suffix move, since the nine moves are tried in their documented order. Across
all distance-11 states, the maximum prefix count is **221**, and the maximum
total B8 count is **260**, both attained by `14325671111111`. These are work
counts, not instruction counts. The assembly's direct solved-state checks can
avoid some final B8 probes, so the C-derived total must not be presented as an
exact assembly count.

## Rubik: exhaustive farthest-state target check

`rubik_d11_target.c` uses all 2,644 farthest states generated by that BFS. It
calls **only the assembly solver**, measures it with the unchanged
`tests/tperf.h`/`common/perf.S` mechanism, requires a return value of eleven,
and independently parses and replays the result. Each call must remain within
`RUBIK_INSN_BUDGET` (7,500,000). The driver has the same official runtime and
4 KiB stack; it is a separate test ELF, not part of the implementation budget.

The completed RV32I run passed **2,644/2,644**, with **878,484** instructions
for the worst measured call, on `14325671111111`. The sum over the measured
calls was **1,015,620,513** instructions. This exhausts the farthest layer; it
does not claim to execute every lower-distance state on the target.

The script snapshots the four assembly sources before assembling them,
records SHA-256 hashes, and saves the ELF, stdout, source versions and a JSON
summary. It uses a separate output directory, so a concurrent official build
cannot replace the tested object. The observed 3.749-second execution duration
is a single validation run, not a median wall-clock performance comparison.
The CSR counts are executed guest instructions, not physical CPU cycles.

## Fixed point: exact host reference and error bound

`fixed_error.c` compares the actual portable C `q_mul` with exact host signed
64-bit multiplication and explicit floor division. The target implementation
has no multiplication instruction or 64-bit arithmetic helper. The independent
host reference does use those operations, making the two implementations
structurally different.

It checks both signs of every one of the 2^20 possible product remainders, a
16 by 16 boundary grid, 65,536 full-width pseudorandom pairs, and 65,536
nonwrapping pseudorandom pairs. The completed run made **2,228,480 comparisons
with zero mismatches**: 2,163,188 nonwrapping and 65,292 wrapping cases.

A Q11.20 value is a signed 32-bit integer A divided by 2^20. Its range is
[-2048, 2048-2^-20] and its resolution is 2^-20. Let M=2^20, P=a*b, and
r=P mod M in [0,M-1]. The rounded, unwrapped integer is
`floor((P+M/2)/M)`. Its real-value error is -r/M^2 below the halfway point
and (M-r)/M^2 at or above it. Therefore its exact nonwrapping error interval is

`[-524287, 524288] / 2^40`.

Both bounds are attained by the actual implementation. The maximum absolute
nonwrapping error is **2^-21 = 0.000000476837158203125**, half an output ULP.
Negative ties also round toward positive infinity. The modulo-2^32 result
wraps when the rounded integer does not fit: the half-ULP bound does not apply
to the final wrapped real value. The JSON reports wrapping cases separately.
No saturation was substituted for the specified wrap-around behavior.

## BFloat16: host reference and domain-specific error

`bf16_error.c` decodes exact BF16/binary32 bit patterns into host `long double`
and uses `sqrtl` as the square-root reference. Build and run it with:

```sh
mkdir -p build/mytests
cc -std=c11 -O2 -Wall -Wextra -Werror mytests/bf16_error.c \
    src/bf16/bf16.c -lm -o build/mytests/bf16_error
./build/mytests/bf16_error > ../evidence/bf16_error.json
```

The completed run checks all 65,536 square-root inputs and reports zero
special-case mismatches. Positive normals have a maximum relative error of
**0.388350981649547%**. All 127 positive subnormals return zero, so their
relative error is **100%**. A global 0.4% error claim would therefore be wrong.
The JSON records absolute-error maxima, representative bit patterns, signed
error extrema, and all input-class counts separately.

For multiplication, the complete 128 by 128 normalized-significand grid at
ordinary scale matches an independent truncating reference. Its maximum
relative error is **0.769184180243474%**. A further 147,456 normal-operand
cases cross the normal/subnormal result boundary. The contract removes the
implicit leading one before shifting the fraction, so its subnormal behavior
is not ordinary gradual underflow. For example, `0x0080 * 0x3F00` has the exact
product `2^-127` and would quantize to `0x0040`, but the required routine returns
`0x0000`: **100% relative error**. Overflow is also excluded from the finite
normal-result error bound.

Narrowing is checked against an independent comparison-based RN-even oracle
on 327,680 boundary samples, with zero mismatches. For a normal binary32 input
whose narrowed result remains finite, the standard relative bound is `2^-8`,
and the tight maximum is `1/257`. Below the smallest normal magnitude, the
absolute bound is `2^-134`; relative error can reach 100%. Representative tie,
underflow, overflow, and NaN-payload cases are included. In particular, the
specified top-half-only handling of exponent 255 maps `0x7F800001` to infinity.
The analysis does not claim to enumerate all binary32 values or all 2^32
possible multiplication pairs.

## Rubik: all-input static instruction bound

`rubik_bound.py` reproduces the source-specific static accounting in
`RUBIK_ASSEMBLY_BOUND.md`:

```sh
python3 mytests/rubik_bound.py --prefix riscv-none-elf- \
    --out ../evidence/rubik_instruction_bound.json
```

The audited assembly has at most 451 B8 probes, 462 prefix frames, and 531
move calls. Counting expanded RV32I instructions with justified loop bounds
gives a conservative call-tree bound of 3,891,850. The deliberately rounded
published bound is **5,632,744 instructions**, below the required 7,500,000
for every input under the API's valid-memory preconditions. This argument
uses the actual control flow and checked immutable-table invariants; it does
not extrapolate from the distance-11 measurement. The solver stack bound is
**672 bytes**, including the depth-zero recursive frame.

The script pins SHA-256 hashes of the four assembly sources, assembles an
independent object, verifies all call sites and backedges, validates the data
invariants, and emits a JSON account. It refuses changed sources until the
loop proof is reviewed. This is a proof for the assembly call tree, not for
the C implementation, whole-test execution time, or physical CPU timing.
