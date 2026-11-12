# Assignment 2: Complete Applications on RV32I

## Scope, result, and authorship

This report covers the four required applications: an optimal 2×2×2 Rubik solver, three BFloat16 operations, unsigned binary GCD, and four integer/fixed-point operations. Every function has a portable C implementation and an independently assembled RV32I implementation. Both the local driver and a **fresh trusted official-framework rebuild** report **100.0/100**, with all four programs passing at `-O1`, `-O2`, `-O3`, and `-Os`. The trusted run preserved the instructor's harness and every grading gate. These are automated results; the course assesses the report, personal contribution, and CodeTrial separately. Evidence includes [the trusted grade](evidence/grade_trusted.log), [the final local grade](evidence/grade_final_local.log), [the full measurements](evidence/cycles.md), and the [source/evidence register](SOURCES.md).

**Authorship disclosure.** ChatGPT prepared the new C and assembly implementations, mathematical analysis, supplementary test programs, local experiments, and this report. The assembly was written directly as assembly source, including explicit assembler macros, rather than obtained by compiling C. That describes how the code was produced; it does **not** establish that the student personally wrote the course's hand-written component. The Rubik symmetry model and distance tables were adapted from the user's existing `RUBIK_N2_H48_ASSIGNMENT_SAFE_FINAL_20261003.zip`, whose provenance is recorded in [SOURCES](SOURCES.md#prior-rubik-work). No personal learning diary, independent student decisions, interview, or recording is claimed. The [course AI guidelines](https://hackmd.io/@sysprog/arch2026-ai-guidelines) require disclosure and personal contribution, so this distinction must remain visible when the material is used.

The code discussed below is pinned at commit `dad5375173708e3a9874f5803429040169ddf6fb` in [the project repository]({{REPO_URL}}). Implementation links use that commit, so the report does not silently follow future source changes.

## 1. Experimental environment and interpretation

The cross compiler is **xPack RISC-V GCC 15.2.0-1**, reporting GCC 15.2.0. The official framework revision inspected was `fd2d7c17db223807c342e3f210c769d23d1865b1`. The emulator is `sysprog21/rv32emu` revision `9dbff65c4d6a40d64d3bbb38f4c5a20d6e885e24`, configured for system emulation, ELF loading, and the interpreter, with the M, A, F, and C extensions disabled. The framework uses `-march=rv32i_zicsr -mabi=ilp32 -ffreestanding -fno-pic -fno-stack-protector`; Zicsr is needed by the measurement harness. The implementation objects themselves contain only permitted RV32I instructions and do not read counters. The runtime supplies a 4 KiB stack and no allocator or general-purpose C library. These settings and the allowed helper symbols come from the [official framework and contracts](SOURCES.md#official-framework-and-contracts).

The benchmark measures C and assembly on identical inputs in the same executable. It subtracts a separately measured baseline for the loop, input loading, and counter reads, then divides by the call count:

$$I_{\mathrm{call}}=\frac{I_{\mathrm{measured}}-I_{\mathrm{baseline}}}{n}.$$

Fractional results arise from this division and the surrounding compiled caller. The assembly object is identical across optimization levels, but its measured amortized cost can differ slightly because its caller changes. Accordingly, comparisons against the fastest C build use the assembly measurement **from that same ELF**. The official summary's general assembly column uses the `-O2` ELF. The performance gate allows assembly at most two additional instructions per call over the fastest C level. Section sizes are measured on implementation objects, excluding the shared harness. [Raw CSV](evidence/cycles.csv) retains totals, baseline, call count, instruction count, and checksums.

rv32emu's `cycle` and `instret` counters count executed guest instructions. The results describe instruction work, not pipeline latency, cache misses, branch prediction, or a physical processor's wall-clock speed. The four optimization levels also demonstrate that a higher optimization level is not necessarily fastest for this particular RV32I workload. [The official measurement implementation](SOURCES.md#official-framework-and-contracts) defines these conventions.

Host tests used AddressSanitizer and UndefinedBehaviorSanitizer. The actual invocation was `LSAN_OPTIONS=detect_leaks=0 make sanitize`: only leak detection was disabled because LeakSanitizer refuses the traced execution environment. AddressSanitizer and undefined-behavior instrumentation remained active. The separate `LSAN_OPTIONS` variable matters because the official Makefile sets `ASAN_OPTIONS` itself. None of the submitted implementations allocates heap memory. Supplementary tests live under `mytests/`, outside the submitted implementation directories. The instructor's headers, test inputs, budgets, and grading logic were preserved.

### Lab2 prerequisite

The separate official playground was built from revision `914f7715f8d7f8678892a9fd91f78b40de351fc6`. A clean rebuild was performed before changing from `-O0` to `-O2`. Both runs passed **8/8** checks. The recorded cycle values are:

| Playground interval | `-O0` | `-O2` |
|---|---:|---:|
| ChaCha20 test | 6,797 | 4,322 |
| BF16 addition | 432 | 162 |
| BF16 subtraction | 373 | 112 |
| BF16 multiplication | 464 | 111 |
| BF16 division | 624 | 211 |
| BF16 special cases | 239 | 117 |

These reproduce the [Lab2 reference table](https://hackmd.io/@sysprog/Sko2Ja5pel). The actual `-O2` ChaCha20 `instret` value was 4,323, one more than its cycle interval because the two counters have separate read endpoints. The intervals include test setup, checking, and output, so they are not isolated cipher timings. Complete logs and the executed reproduction script accompany the delivery's Lab2 support material.

## 2. Rubik: exact search within a small distance perimeter

### State, interface, and coordinate conversion

The [official Rubik contract](SOURCES.md#official-framework-and-contracts) represents the seven movable corners as seven permutation digits followed by seven orientation digits. The permutation must contain each of `1..7` exactly once. Subtracting one from each orientation digit gives twists in `{0,1,2}` whose sum is zero modulo three. The fixed eighth corner removes whole-cube rotation ambiguity. Consequently the legal state count is

$$7!\,3^6=5,040\times729=3,674,160.$$

The solved string is `12345671111111`. The nine moves are `R`, `R2`, `R'`, `B`, `B2`, `B'`, `D`, `D2`, and `D'`; each costs one move in the specified half-turn metric. The return value is the shortest move count, not the string length. Invalid input returns `-1` without changing the output. Insufficient capacity returns `-2` and clears the first output byte when capacity is nonzero. A successful output has single spaces, no trailing space, and a terminating NUL.

The previous H48 solver uses another corner numbering. The [adapter]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/rubik/rubik.c) applies `[3,1,5,2,0,4,6]` to both positions and piece labels, leaving twists unchanged. Official move indices map to internal indices `[2,1,0,8,7,6,5,4,3]`. Applying the same relabeling to positions and labels is necessary: changing only one would alter the solved state and invalidate every distance. The independent checker verifies the resulting move conjugacy for every permutation and every orientation component under all nine moves.

Both implementations validate the complete input before touching output. They compute the complete solution before testing its required string capacity. An empty solution requires one byte; a nonempty solution requires twice the move count plus one extra byte for every `2` or prime suffix. This accounts for separators and the final NUL without producing a partially written solution.

### Canonical keys and B8 storage

The [C search]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/rubik/h48_solver.c) retains an exact radius-eight distance perimeter, called B8, compressed by the previous solver's H48 symmetry machinery. It stores distances, not predetermined solution moves. On the target, every prefix choice and every descent move is computed from the current state.

A seven-piece Lehmer rank indexes 5,040 permutation metadata entries. Small rank multiplications are implemented by shifts and additions. Metadata identifies a permutation orbit and a canonicalizing symmetry. Common orbits use direct base offsets; exceptional orbits use orientation ranks reduced by their stabilizer actions. The key routine constructs eight transformed representatives using the compact `T` transformation and six underlying symmetries. It first finds the smallest permutation-orbit index, then evaluates the corresponding orientation ranks and selects the smallest quotient index. The original tables supply the transformation semantics; exhaustive independent distance checking supplies additional evidence that their use in this port is correct.

The B8 table contains **24,793** compressed entries and **1,197** buckets. A key's upper bits select a bucket; its lower nine bits are binary-searched in that bucket. Each 16-bit entry packs the nine key bits and four distance bits. The distance entries occupy 49,586 bytes and the 1,198 16-bit bucket boundaries occupy 2,396 bytes. Symmetry and rank tables add storage, so the complete implementation footprint, rather than those two arrays alone, is the quantity compared with the 128 KiB limit. No full 3,674,160-state distance array, host solution policy, or distance-11 path list is linked into the submitted target solver.

### Why a three-move prefix yields an optimal solution

For a state already in B8, the exact distance is available. The solver tries neighbors until it finds one with distance smaller by one, records that move, and repeats. This reaches the goal in exactly the recorded distance.

For a state outside B8, the solver tries prefixes of lengths one, two, and three, in that order. Consecutive moves of the same face are omitted: two such moves compose into one allowed turn or cancel, so no shortest path requires that repetition. The first move has nine choices and each later move has six. The candidate counts are therefore 9, 54, and 324.

The independent breadth-first search establishes diameter eleven. Let the input's exact distance be $d>8$. A shortest path reaches distance eight after $d-8\le3$ moves. If any shorter prefix reached B8, appending its at-most-eight-move suffix would contradict the definition of $d$. At the first successful prefix length $d-8$, a suffix shorter than eight would create the same contradiction. Thus the first successful depth plus exact B8 descent yields a shortest solution. This argument depends on an exact radius-eight predicate, which was separately checked over the entire state space.

The simple enumeration bound is at most $1+9+54+324=388$ perimeter probes before suffix reconstruction, followed by at most $8\times9=72$ neighbor probes. This is a conservative structural bound; repeated states and early hits reduce actual work. It is not an instruction count.

### Assembly choices and real optimization iterations

The [direct assembly search]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/rubik/rubik_search.inc) uses aligned 16-byte work states, copied with four word loads and stores. Its bounded recursive prefix routine has at most four active frames, including its depth-zero leaf. The deepest stack path inside the solver uses **672 bytes**: root 128, prefix $4\times80$, perimeter probe 16, canonical key 192, and `T` 16. Shared startup and test-harness frames are separate. Callee-saved registers and the return address are restored, and stack alignment is maintained.

The [canonicalization assembly]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/rubik/rubik_canon.inc) specializes the identity symmetry after verifying its tables. The compact `T` kernel stores doubled independent twists and their sum in positions directly selected by the original control words, removing a conditional from each orientation digit. All 105 control selectors were checked before that representation change. These optimizations preserve the original transformation rather than restricting accepted inputs.

The first working assembly passed correctness but failed the fastest-C gate. The preserved measurements show the actual sequence:

| Rubik assembly revision | Instructions/call in comparison ELF | Outcome against best C, 84,019.0 |
|---|---:|---|
| Initial complete solver | 86,539.3 | Failed |
| Return immediately for solved input | 85,921.3 | Still failed |
| Also compare directly with goal at remaining distance one | 68,509.3 | Passed |

The first two complete driver runs scored 75/100 because the failed Rubik program also reduced the scaled common items. The final run scored 100/100. See [initial](evidence/grade_initial.log), [intermediate](evidence/grade_solved_exit.log), and [final](evidence/grade_final_local.log) logs.

The decisive change follows from the distance invariant: when one move remains, the only acceptable successor is the literal solved state. Comparing its four words avoids canonicalizing and probing each final neighbor. The measured raw final interval is 411,083, the baseline is 27, and there are six calls, giving $(411083-27)/6=68,509.3333$. This is approximately **20.83% fewer instructions than the initial assembly** and **1.226×** the performance of the fastest C build on these inputs. It is an algorithm-aware shortcut with a precise precondition, not a benchmark-specific answer table.

### Independent verification and limits

The [host oracle]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/mytests/rubik_oracle.c) constructs its graph directly from the published R/B/D source and twist tables, independently of H48. It verifies rank/decode round trips, move inverses, and the coordinate conversion, then traverses all 33,067,440 directed edges. Its exact distance distribution is:

| Distance | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---:|---:|---:|---:|---:|---:|
| States | 1 | 9 | 54 | 321 | 1,847 | 9,992 |

| Distance | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---:|---:|---:|---:|---:|---:|
| States | 50,136 | 227,536 | 870,072 | 1,887,748 | 623,800 | 2,644 |

For **all 3,674,160 states**, the C B8 predicate succeeded exactly when the reference distance was at most eight, with the same distance and **zero mismatches**. B8 contains 1,159,968 uncompressed states. All **2,644 distance-11 states** were then solved by the public C interface; an independent parser and move model verified eleven moves, exact distance descent after every move, and the final goal. Maximum observed C search probes were 221, and total C perimeter probes including reconstruction were 260. Those are C work counts, not exact assembly counts. [Oracle summary](evidence/rubik_oracle.json).

Assembly canonicalization separately passed **19,040 target differential checks** against the previous scalar model. Finally, a target ELF tested the assembly on **all 2,644 distance-11 inputs**, checking output replay and the per-call budget. It passed every case. The worst measured call used **878,484 instructions**, below 7,500,000, for `14325671111111`. Its source snapshot and hashes are retained with the extended evidence. This exhausts the farthest layer; it does not claim execution of every lower-distance state on the target. [Kernel log](evidence/canoncheck.log), [target summary](evidence/rubik_d11.json).

### Static instruction bound for every input

The measured farthest-layer maximum is supplemented by a separate static bound for the exact assembly snapshot. Under the API's valid-memory preconditions, every call tree retires at most **5,632,744 instructions**, leaving **1,867,256** below the 7,500,000 budget. This covers every legal state and the bounded error paths; it is not extrapolated from the 2,644 measured cases. The [complete proof]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/mytests/RUBIK_ASSEMBLY_BOUND.md) and [machine-readable accounting](evidence/rubik_instruction_bound.json) record its scope and assumptions.

Across all three attempted prefix depths there are at most 387 leaf probes, 462 prefix frames, and 459 move edges. Reconstruction tries at most 72 neighbor moves, but at distance one uses the literal-goal test, so only 63 suffix probes remain. Including the initial root probe gives at most 451 probes and 531 total move calls. Table invariants bound orientation arithmetic and searches; every stored B8 distance is at most eight. A failed search also terminates within these bounds.

The checker counts expanded RV32I instructions, verifies call sites and loop backedges, and charges conservative local repetition limits. Rounded component ceilings give:

$$451(10000+1000)+512(1024)+1024(128)+16384=5,632,744.$$

The terms cover canonicalization plus probe work, prefix frames, moves, and wrapper work respectively. The checker is tied to SHA-256 hashes of all four assembly source/include files; a source change requires reviewing the proof again. This bound concerns the solver's assembly call tree, excluding the C solver and shared harness. It does not describe physical cycles or host runtime. The **878,484** figure remains the separately observed maximum over the distance-11 target experiment.

The solver illustrates how exact compressed distances can support bounded online search when a full state table exceeds memory. The same design tradeoff is relevant to finite-state planning and puzzle search: spend computation to recover paths while retaining only enough information to prove optimality.

## 3. BFloat16: exact contract, limited numerical precision

### Representation and binary32 narrowing

BFloat16 has one sign bit, eight exponent bits with bias 127, and seven stored fraction bits. For $1\le E\le254$, the value is $(-1)^s2^{E-127}(1+F/128)$. The minimum positive normal is $2^{-126}$, the subnormal step is $2^{-133}$, and the maximum finite value is $(2-2^{-7})2^{127}$. The [contract](SOURCES.md#official-framework-and-contracts) also defines special-case behavior that must be distinguished from a general IEEE floating-point implementation.

The [C implementation]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/bf16/bf16.c) converts binary32 bit patterns without floating-point arithmetic. For finite inputs it uses this short core:

```c
f32bits += 0x7FFFu + ((f32bits >> 16) & 1u);
return (uint16_t) (f32bits >> 16);
```

The discarded half below `0x8000` rounds down, above it rounds up, and exactly `0x8000` increments only when the retained least-significant bit is odd. Thus ties select an even result. Exponent-255 inputs bypass the addition and retain their upper half exactly. A binary32 NaN whose payload exists only in the discarded half can consequently become infinity, for example `0x7F800001 -> 0x7F80`; preserving a NaN in that case would change the required contract.

For normal finite inputs that do not overflow on conversion, round-to-nearest gives the usual relative-error bound $2^{-8}=0.390625\%$. In the tiny-value domain, the useful bound is absolute: half a subnormal step, $2^{-134}$. Very small nonzero inputs can round to zero and have 100% relative error. Large finite inputs can round to infinity, which is classified separately.

### Multiplication and exceptional behavior

Nonzero subnormal inputs are normalized by at most seven shifts. Each normalized significand is an integer in `[128,255]`. Their product fits in 16 bits. A product at least `0x8000` is shifted by eight with an exponent increment; otherwise it is shifted by seven. The low bits are truncated, not rounded to nearest. If $P$ is this integer product and $k$ is the selected shift, truncation discards less than $2^k$ while $P\ge128\,2^k$. Thus the relative magnitude error is less than $2^{-7}$ when the final result remains normal and finite; the measured significand grid below gives the tighter attained maximum.

Exceptional classification is ordered. A NaN in the first operand is returned unchanged; an infinite first operand times zero returns `0x7FC0`, and otherwise its infinity branch takes precedence even over a NaN second operand. Only after handling that branch is the second operand classified. Signed zeros, infinities, and payload preservation follow this exact ordering.

There is an especially significant underflow rule: the implementation removes the implicit leading one **before** shifting the stored seven-bit fraction into a subnormal result. For instance, `0x0080 * 0x3F00` returns zero although the exact product is representable as `0x0040`. This is the specified result, not correctly rounded gradual underflow. An error claim restricted to normal results must therefore not be extended to the whole domain.

On RV32I, the C multiplication operator uses the permitted software `__mulsi3` helper. The [assembly]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/bf16/bf16.S) instead uses the known eight-bit width and leading one: it initializes the product with `ma << 7` and adds the seven possible remaining partial products. Explicit `.rept` blocks remove loop control; they expand directly written instructions. The routine makes no helper call and uses no stack.

### Square-root derivation

For a positive normal input, the biased exponent determines whether the integer significand $m=128+F$ must be doubled. The specified search predicate is

$$\left\lfloor\frac{q^2}{128}\right\rfloor\le m
\quad\Longleftrightarrow\quad q^2\le128m+127.$$

Therefore the required significand is exactly $q=\lfloor\sqrt{128m+127}\rfloor$. The radicand fits in 16 bits and has a known leading radix-four root digit. After that initial step, seven restoring-root steps test powers `4096,1024,256,64,16,4,1`. Each step shifts the scaled root and conditionally subtracts a trial value from the remainder. This computes the predicate's largest valid integer without repeated multiplication or a lookup table. The result exponent is `(E + 127) >> 1`, avoiding a right shift of a negative signed exponent.

The `+127` comes from the contract's integer predicate. It means the result is not simply a downward rounding of the mathematical square root; some normal results lie above it. Both zeros and every positive subnormal return positive zero. Negative nonzero inputs, including negative infinity, return `0x7FC0`; NaN patterns and positive infinity are preserved.

### Numerical experiment and compiler comparison

The supplementary [error program]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/mytests/bf16_error.c) decodes bit patterns into host `long double`, which had 64 significand bits, and uses `sqrtl` as the square-root reference. It reports **zero semantic-check failures**. Its numerical domains are deliberately separated:

| Operation and domain | Cases | Observed maximum relative error |
|---|---:|---:|
| Square root, all positive normals | 32,512 | 0.38835098165% |
| Square root, all positive subnormals | 127 | 100% |
| Multiply, all normalized significand pairs at exponents 127 | 16,384 | 0.76918418024% |
| Multiply, normal-input underflow boundary grid | 147,456 | 100% |
| Narrowing, finite normal boundary samples without overflow | 325,114 | 0.38910505837% |

All **65,536** square-root input patterns were classified and checked. The largest normal relative error occurs at `0x0081`, returning `0x2000`. Among normal results, 1,016 are exact, 20,447 lie below the reference, and 11,049 lie above. The maximum normal absolute error, approximately $5.1000612\times10^{16}$, occurs at a different large input, `0x7F47`; absolute and relative maxima should not be conflated.

The multiplication significand grid matches independent round-toward-zero quantization in every case. Scaling by powers of two preserves its relative error while the output remains finite and normal. Its maximum is exactly $127/16511$, obtained from `0x3F9E * 0x3FD1`, whose exact value is 2.0155029296875 and result is 2. The underflow experiment uses exponent pairs $(1,119..127)$ and every significand pair. It finds 114,688 differences from proper round-toward-zero quantization, demonstrating the consequence of the contract's discarded leading bit. Narrowing checks all upper halves with five lower-half boundaries, **327,680 samples**, with zero independent rounding-oracle mismatches. This is not an exhaustive enumeration of all binary32 values or multiplication operand pairs. [Complete numerical evidence](evidence/bf16_error.json).

Official host tests passed 17/17, and target tests passed 42/42 at each level. Observed assembly maxima were 14, 124, and 69 instructions for narrowing, multiplication, and square root, against budgets 60, 400, and 1,500. Multiplication improved from the fastest C measurement 81.5 to 53.125 instructions/call, approximately 1.534×. Square root improved from 39.3125 to 37.75, approximately 1.041×. Narrowing measured 11.25 against C's 11.0625, so it is **slightly slower**, within the official allowance. No unmeasured earlier BF16 version is presented as a historical baseline.

BFloat16's exponent range is useful when range matters more than fine precision, such as storing approximate numerical features. This implementation demonstrates integer arithmetic on a machine without floating-point instructions, but its underflow and NaN rules must be preserved and documented if reused; it is unsuitable as an unqualified substitute for a standard floating-point library.

## 4. Binary GCD without division

Both [C]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/gcd/gcd.c) and [assembly]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/gcd/gcd.S) implement Stein's algorithm on unsigned 32-bit integers. Zero returns the other operand, including `gcd(0,0)=0`. The algorithm removes and remembers the common power of two, makes the remaining operands odd, subtracts the smaller from the larger, removes factors of two from the difference, and repeats. A normalized operand equal to one permits immediate termination.

Correctness follows from three identities:

$$\gcd(2a,2b)=2\gcd(a,b),\qquad
\gcd(2a,b)=\gcd(a,b)\ \text{for odd }b,\qquad
\gcd(a,b)=\gcd(a-b,b)\ \text{for }a\ge b.$$

For positive odd $x>y$, replacing $x$ by $(x-y)/2^r$, with $r\ge1$, reduces the operand sum to at most half its former value. Thus the number of subtraction stages is bounded by word width rather than operand magnitude. Shifts remove positive factors of two; a terminal zero is checked before another normalization loop. Subtraction is performed only in the nonnegative direction, and restoring the remembered factor cannot overflow because the resulting GCD is no larger than the original operands.

The C version swaps operands before subtraction. Assembly branches directly to the larger operand's subtraction path and avoids that swap. Because the difference of two odd operands is even, it performs the first right shift immediately. The function uses only caller-saved registers, no stack, no multiplication, division, remainder, or helper.

Official tests include all 65,536 pairs below 256, powers of two, edge values, deterministic random pairs, and demanding sequences. Host tests passed 13/13; each target level passed 34/34. The largest observed assembly call used 290 instructions against a 1,500-instruction budget. The fastest C mean was 104.75 instructions/call at `-O1`, while assembly was approximately 84.2, giving about 1.24× improvement. The observed maximum is a test result, not a claim to enumerate all $2^{64}$ pairs. GCD is exact under its contract, so there is no rounding-error analysis. Its practical uses include rational normalization and reducing integer ratios on small processors without a divider.

## 5. Integer operations and Q11.20 multiplication

### Observable bitwise behavior

The [fixed-point contract](SOURCES.md#official-framework-and-contracts) defines both the arithmetic result and some operational behavior. In `op1`, each pass executes

```c
uint32_t carry = (a & b) << 1;
a ^= b;
b = carry;
```

XOR adds without carries, while AND followed by a left shift carries into the next bit. Their sum preserves $a+b$ modulo $2^{32}$. Every nonzero carry moves toward a higher bit, so at most 32 passes occur. The pass count is returned through `n`; replacing the loop by `add` would produce the right sum but the wrong observable behavior. For example, `op1(0xFFFF,1)` requires seventeen passes, and a zero second input requires none.

`op2` is ordinary subtraction modulo $2^{32}$, so assembly needs a `sub` and return. `op3` computes the exact unsigned floor average without overflowing an intermediate sum:

$$\left\lfloor\frac{a+b}{2}\right\rfloor
=(a\mathbin{\&}b)+((a\mathbin{\oplus}b)\gg1).$$

This follows from the integer identity $a+b=2(a\mathbin{\&}b)+(a\mathbin{\oplus}b)$. The final average fits the unsigned range. These three functions are exact for their specified modular or floor operations, including boundary values.

### Signed product using explicit unsigned words

A Q11.20 stored integer $A$ represents $A/2^{20}$, with range $[-2048,2048-2^{-20}]$ and resolution $2^{-20}$. For signed inputs $a,b$, the contract requires the low 32 bits of

$$\left\lfloor\frac{ab+2^{19}}{2^{20}}\right\rfloor.$$

The addition of half a unit makes ties round toward positive infinity, including negative ties. Overflow wraps; it does not saturate.

Let $A,B$ be the unsigned bit patterns, and $s_a,s_b$ indicate negative inputs. Modulo $2^{64}$,

$$ab\equiv AB-s_aB2^{32}-s_bA2^{32}.$$

The [C implementation]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/fixed/fixed.c) performs shift-and-add multiplication with explicit unsigned `hi` and `lo` words. The initial low word contains $2^{19}$. When a selected multiplicand is added, a low-word carry is detected by comparing the new low word with the old one. Shifting the multiplicand transfers its top low-word bit into the high word. Signed correction subtracts $B$ or $A$ from the high word. This handles `INT32_MIN` without taking an overflowing absolute value.

Only product bits 20 through 51 survive, so the result is extracted by `(lo >> 20) | (hi << 12)`. Logical extraction gives the same low 32 bits as arithmetic shifting the signed product. Unsigned arithmetic makes all intermediate wraparound defined. The final conversion to `int32_t` relies on the supported two's-complement GCC/host implementation; the result bit pattern is the contract. No signed-overflow expression is used.

### Actual compiler and assembly revisions

An earlier C version used a `uint64_t` shift-and-add loop. GCC 15.2.0 at `-Os` recognized it as multiplication and emitted an unavailable `__muldi3`, although the other three levels linked. Representing the accumulator and multiplicand as explicit 32-bit words removed the helper at every level. This resolved the implementation under the existing allowed-symbol rules.

The first assembly measured **236.0 instructions/call**, failing against the fastest C's **212.6875**. The revised [assembly]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/src/fixed/fixed.S) initializes signed corrections in the high word, compares the new low word with the added multiplicand to avoid a copy, and combines loop exit with the backedge. It reached **208.0** in the same `-O1` ELF, a **28-instruction or 11.86% reduction**, approximately 1.023× faster than C. [Preserved initial measurements](evidence/benchmarks_before_qmul_optimization.json) document the unsuccessful version. All four assembly routines are leaves and allocate no stack.

The selected partial-product addition is exactly four instructions:

```asm
add  t0, t0, a0
sltu t5, t0, a0
add  t1, t1, t2
add  t1, t1, t5
```

Here `t1:t0` is the accumulator and `t2:a0` is the shifted multiplicand. The comparison detects a carry using the unchanged addend, so the old accumulator need not occupy another register. Inspection of the generated `-O2` listing also shows GCC computing candidate low/high sums before its multiplier-bit branch and maintaining multiple loop paths, whereas the assembly skips those four instructions when that bit is clear and uses one backedge. Conversely, the compiler's `op2` is the same `sub`/return pair as assembly, and its `op3` uses the same four arithmetic instructions with different register ordering. Those listing observations explain both the multiplication gain and the absence of a small-operation gain.

For the small operations, the compiler already emits essentially the intended work. `op2` and `op3` match the best C means, four and seven instructions/call. `op1` measures 22.125 against C's 22.0625; its fractionally higher count passes the allowance but is not a speedup. Assembly should be judged by the observed result rather than assumed superior by construction.

### Exact error bound and independent experiment

Let $M=2^{20}$, $P=ab$, and $r=P\bmod M$ in `[0,M-1]`. Before wrapping, the stored result is $R=\lfloor(P+M/2)/M\rfloor$. Compared with the exact real product $P/M^2$, the real-value error is

$$\frac{R}{M}-\frac{P}{M^2}=
\begin{cases}
-r/M^2,&r<M/2,\\
(M-r)/M^2,&r\ge M/2.
\end{cases}$$

Its exact nonwrapping interval is therefore

$$-\frac{524287}{2^{40}}\le\mathrm{error}\le\frac{524288}{2^{40}},$$

and the maximum absolute error is $2^{-21}=0.000000476837158203125$, half an output ULP. The asymmetric endpoints follow from the tie direction. Both are attained: stored input pairs `(1,524287)` and `(1,524288)` reach the lower and upper endpoints.

The [independent host checker]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/mytests/fixed_error.c) uses signed 64-bit multiplication and explicit floor division. It checks both signs of every one of the $2^{20}$ remainders, a 16×16 boundary grid, 65,536 full-width random pairs, and 65,536 nonwrapping random pairs, with seed `0x6D2B79F5`. The result is **2,228,480 comparisons and zero mismatches**, partitioned into 2,163,188 nonwrapping and 65,292 wrapping cases. [Exact results](evidence/fixed_error.json).

The half-ULP bound does not describe the final value after overflow wraps by a multiple of 4,096 real units. For example, stored inputs `2147483646,2147483646` return `-8192`; their real error is enormous relative to half an ULP even though the required low bits are correct. A fixed-point application must therefore establish input scaling and range separately from rounding precision. These operations suit bounded signal-processing or control calculations on integer-only hardware; saturation, if desired by an application, would require a different contract.

Official fixed tests passed 15/15 on the host and 38/38 per target level. Observed assembly maxima were 169, 6, 9, and 400 instructions for `op1`, `op2`, `op3`, and `q_mul`, against budgets 600, 40, 40, and 1,200.

## 6. Consolidated measurements and reproducibility

The [complete cycle table](evidence/cycles.md) records every C optimization level and the assembly measurements. [The CSV](evidence/cycles.csv) supports recalculation without relying on rounded display values. The fastest level is selected independently for each function, and speed ratios use the corresponding assembly measurement. The normal baseline is `-O2`; a lower instruction count or smaller object at another level is reported as observed rather than treated as an optimization-level guarantee.

| Function | C `-O1` | C `-O2` | C `-O3` | C `-Os` | Assembly in `-O2` |
|---|---:|---:|---:|---:|---:|
| bf16.from_f32 | 11.0625 | 11.1875 | 12.8125 | 11.1875 | 11.2500 |
| bf16.mul | 90.5625 | 81.5000 | 82.9375 | 85.5000 | 53.1250 |
| bf16.sqrt | 54.9375 | 51.5625 | 39.3125 | 49.7500 | 36.3125 |
| fixed.op1 | 24.7500 | 22.0625 | 24.1875 | 24.6875 | 22.1250 |
| fixed.op2 | 4.0000 | 4.0000 | 5.9375 | 4.0000 | 4.0000 |
| fixed.op3 | 7.0000 | 7.0000 | 8.9375 | 7.0000 | 7.0000 |
| fixed.q_mul | 212.6875 | 251.1250 | 253.0625 | 239.8750 | 208.0000 |
| gcd.gcd | 104.7500 | 126.8750 | 129.0625 | 131.5625 | 84.1875 |
| rubik.solve | 130,529.0000 | 123,960.0000 | 84,019.0000 | 133,781.8333 | 68,506.8333 |

The following ratios use the same ELF for both implementations:

| Function | Fastest C level | C instructions/call | Assembly in that ELF | C / assembly |
|---|---|---:|---:|---:|
| bf16.from_f32 | `-O1` | 11.0625 | 11.2500 | 0.9833× |
| bf16.mul | `-O2` | 81.5000 | 53.1250 | 1.5341× |
| bf16.sqrt | `-O3` | 39.3125 | 37.7500 | 1.0414× |
| fixed.op1 | `-O2` | 22.0625 | 22.1250 | 0.9972× |
| fixed.op2 | `-O1` | 4.0000 | 4.0000 | 1.0000× |
| fixed.op3 | `-O1` | 7.0000 | 7.0000 | 1.0000× |
| fixed.q_mul | `-O1` | 212.6875 | 208.0000 | 1.0225× |
| gcd.gcd | `-O1` | 104.7500 | 84.1875 | 1.2442× |
| rubik.solve | `-O3` | 84,019.0000 | 68,509.3333 | 1.2264× |

Static footprint is recorded separately in [footprint.csv](evidence/footprint.csv), with `.text`, `.rodata`, `.data`, and `.bss` for each implementation. The assembled BF16 routines use 760 bytes of code, GCD 156, and fixed 168, with no read-only data, writable data, or BSS in those three assembly objects. BF16's expanded instruction sequences cost more code than its smallest C build; the multiplication gain therefore involves a speed/size tradeoff. Rubik's data compression and total allocated footprint are essential to its memory constraint. The smallest C build is the proper size comparison, even when another level is fastest.

| Program | Implementation | `.text` | `.rodata` | `.data` | `.bss` | Allocated total |
|---|---|---:|---:|---:|---:|---:|
| rubik | assembly | 4,280 | 87,860 | 0 | 0 | 92,140 |
| rubik | smallest C, `-Os` | 4,296 | 87,898 | 0 | 0 | 92,194 |
| bf16 | assembly | 760 | 0 | 0 | 0 | 760 |
| bf16 | smallest C, `-Os` | 564 | 0 | 0 | 0 | 564 |
| gcd | assembly | 156 | 0 | 0 | 0 | 156 |
| gcd | smallest C, `-Os` | 148 | 0 | 0 | 0 | 148 |
| fixed | assembly | 168 | 0 | 0 | 0 | 168 |
| fixed | smallest C, `-Os` | 172 | 0 | 0 | 0 | 172 |

Rubik assembly occupies 92,140 bytes, or 70.30% of the 131,072-byte limit. Its fastest C build, `-O3`, uses 103,166 bytes; the smallest C build, `-Os`, uses 92,194. GCD assembly saves instructions but uses eight more bytes than the smallest C object. Fixed assembly is both faster for multiplication and four bytes smaller than the smallest combined C object. Debug and attribute sections are not allocated runtime storage.

From the repository root, the official workflow is:

```sh
make toolchain
LSAN_OPTIONS=detect_leaks=0 make sanitize
make test
make dump
make cycles
make grade
```

The leak-detection setting above is specific to the traced preparation environment; ASan and UBSan remain enabled. A fresh trusted official-framework run of `scripts/grade.sh --submission ...` rebuilt the submitted sources with unchanged instructor tests, contracts, budgets, and gates, and independently returned **100.0/100**; see [grade_trusted.log](evidence/grade_trusted.log). The supplementary commands are documented in [mytests/README.md]({{REPO_URL}}/blob/dad5375173708e3a9874f5803429040169ddf6fb/mytests/README.md): `run_rubik_oracle.sh`, `run_rubik_d11_target.sh`, and `run_fixed_error.sh` each accept an evidence output directory. BF16's independent experiment is built with the host compiler and `-lm`. Its sources and recorded numerical output permit reproduction without any provider account. Full-state oracle arrays are host-only evidence, never implementation data.

The official host counts are Rubik 23, BF16 17, GCD 13, and fixed 15; target counts per optimization level are 52, 42, 34, and 38 respectively. These test counts represent suites containing loops and exhaustive subchecks, not the number of individual numerical comparisons. The supplementary exhaustive Rubik and error experiments state their own domains explicitly. A successful automated grade is evidence for these checked properties, not a replacement for understanding or the course's personal assessment.

## 7. CodeTrial preparation, device access, and personal assessment

The latest [Assignment 2 handout](https://hackmd.io/@sysprog/2026-arch-homework2), checked on November 12, requires CodeTrial; an earlier change to Assignment 1 does not cancel this requirement. The handout's current deadline is **November 18, 2026, 23:59 GMT+8**. Retrieval times and snapshot hashes are listed in [SOURCES](SOURCES.md#official-assignment-and-ai-policy).

CodeTrial revision `64fbc228b411d59698d9bd27703f56263f18ac16` was built locally from source with the pinned Rust 1.99.1 toolchain and Clang 23 bootstrap toolchain. `make build` succeeded, the executable reported `codetrial 0.1.0 (64fbc22)`, all asset checksums passed, and the upstream offline task-mode suite passed **25 tests**. A real no-credential HTTP smoke test reached the Setup page and `/api/setup`, both with status 200 and `configured:false`. The unconfigured `/healthz` returned 404. No credentials were fabricated, no provider session was started, and no interview was performed. The delivery includes the executed preparation and smoke-test scripts and their logs.

The course requires CodeTrial to run from a source build on the learner's own computer. Setup, building, asset verification, and offline checks are automatable: the supplied scripts already perform those steps. Repeating them on that computer requires access to that device and its environment, rather than inherently requiring manual human work. Configuring the live session additionally requires the learner's Gemini and LiveKit credentials and microphone/camera permissions.

The learner's own English explanation, coding decisions during assessment, voice, camera presence, practice attempt, and completed attempt must be authentic. The default Rubik task uses C, `-O1`, a 60-minute clock, and built-in hints; its guest flow needs no assignment link or PIN. The full recording must cover preparation/rules through the review after Finish, including microphone and tab audio. Recording, result, and code must be downloaded before closing the session. [Official CodeTrial instructions](SOURCES.md#codetrial).

The remaining account actions are publishing the reviewed code to the learner's fork, checking remote CI, publishing this disclosed report to the intended HackMD account, and submitting the resulting URLs through the course form. Those actions can be automated when the intended accounts, access, and authorization are available. The local code, analysis, directly authored assembly, builds, measurements, and report preparation have already been performed. Personal contribution and an authentic assessment attempt cannot be supplied by these artifacts.
