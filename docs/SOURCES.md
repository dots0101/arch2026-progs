# Sources, revisions, and evidence register

This register distinguishes instructor specifications, implementation provenance, and measurements produced during local preparation. The source snapshots were inspected on **2026-11-12 UTC**. A URL identifies the publisher; a revision or SHA-256 identifies the exact material inspected. Hashes alone do not imply that a source will remain available at its live URL. The complete official handout and AI policy are not reproduced in this deliverable.

## Official assignment and AI policy

The Assignment 2 handout governs the requested work. Its current deadline at retrieval was **November 18, 2026, 23:59 GMT+8**; the course schedule retained an earlier date. The HW2 handout still required CodeTrial. The AI policy is separately relevant to personal authorship and disclosure.

| Source | Retrieval time, UTC | SHA-256 of downloaded Markdown |
|---|---|---|
| [Assignment 2: Complete Applications](https://hackmd.io/@sysprog/2026-arch-homework2) | `2026-11-12T09:58:00.115999+00:00` | `426ba6277a03b2eab855d883495ebeea1927613def78900087bd657161a44cb1` |
| [Computer Architecture AI guidelines](https://hackmd.io/@sysprog/arch2026-ai-guidelines) | `2026-11-12T09:58:06.123965+00:00` | `b9a923083cfbe846901daecd14695e55fb9b79bc0e3404818f83258d81f40ccf` |
| [Lab2: RISC-V simulator and system emulator](https://hackmd.io/@sysprog/Sko2Ja5pel) | `2026-11-12T09:58:12.464038+00:00` | `c8a5f5e5fd6e380439cf03e82b17dc826abc975bdc05435676972759939d3e36` |

The snapshots were obtained from the corresponding URL with `/download` appended. Hashes apply to those response bytes, not to a browser-rendered HTML page. The primary course schedule is [sysprog21.github.io/arch](https://sysprog21.github.io/arch/), and the handout links the [submission form](https://forms.gle/dUzhxC6CZhHkYjDeA). The live handout is the authority for subsequent changes.

## Official framework and contracts

Upstream: [sysprog21/arch2026-progs](https://github.com/sysprog21/arch2026-progs), inspected `origin/main` revision **`fd2d7c17db223807c342e3f210c769d23d1865b1`**. The contract headers take precedence over background quiz fragments for special cases, rounding, overflow, error returns, and budgets. The report cites the headers and grader at this pinned revision.

| File at inspected upstream revision | Purpose | SHA-256 of Git blob bytes |
|---|---|---|
| [README.md](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/README.md) | Build, grading, and repository workflow | `e16ec6010fabacd06b662b03fc05ddf30b898e7d602c5b674a42724991e09000` |
| [Makefile](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/Makefile) | Compiler flags, object isolation, tests and measurements | `fe15c81314b4e839bb29d9cbbab40ff77a061efa6b4d24e5fb70881dbead6731` |
| [mk/toolchain.mk](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/mk/toolchain.mk) | RV32I/Zicsr target and ILP32 ABI | `29971e1d82edddc4085d83a067383d3e9aafbae09570a64e0486389a15de4fe6` |
| [src/rubik/rubik.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/src/rubik/rubik.h) | Rubik state, moves, errors, optimality and resource limits | `d2377fbf0505198117176d8b2801b72beb6217a29016f759cea85d7c163c87aa` |
| [src/bf16/bf16.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/src/bf16/bf16.h) | BF16 exact bit behavior and instruction limits | `ee0d86f25b0dd2326340a38785d1c7e16db65444d9b26b84323e4f3ef2841888` |
| [src/gcd/gcd.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/src/gcd/gcd.h) | Unsigned GCD behavior and instruction limits | `599be6f3ef55f179fe777e866ee3b3c80f9fe90ca8ebaa61919eee0048a886d2` |
| [src/fixed/fixed.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/src/fixed/fixed.h) | Observable carry loop, average, Q11.20 and limits | `36735c258ed62fe8e62d17902d7fe6bd223677f6529cb23f33dc19bdf0e0ce73` |
| [scripts/driver.py](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/scripts/driver.py) | Automated score and same-ELF speed gate | `1658993575b6446185527e0d4ad52dd95b11021a6d50655853c40ac33bd7493d` |
| [scripts/cycles.py](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/scripts/cycles.py) | Measurements, CSV, ratios and section tables | `0bfee79630f8e33fdc5a89988dfed1d1c72446d8f2084b05cc0b908f48c1237a` |
| [scripts/benchlib.py](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/scripts/benchlib.py) | ELF execution, parsing and footprint accounting | `6aa463f666ac80ad3c60b3c3d2d232dc113c8048da18fc00fb892e499baea36e` |
| [scripts/rvcheck.py](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/scripts/rvcheck.py) | Instruction and external-symbol checks | `ba04d173a1ef4d7beaf3777a5d5a4d0c56246ff61007e8fe141e8733127d7192` |
| [common/bench.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/common/bench.h) | Benchmark loop and baseline measurement | `8a49413447475c178eaee6d42e31e8f8e4b31ddae8d7e5877c9540cc7396a73f` |
| [common/perf.S](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/common/perf.S) | Counter read implementation | `4c3254d5be1624e180b40c9b4cbc3e41c5a40073369bdf56eb6c67685ec62346` |
| [common/linker.ld](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/common/linker.ld) | Load address and fixed runtime stack | `4986b5a8f345a634b237d43769d02bc2b46a6363a1353b26c80e9eb95ae2f936` |
| [tests/tperf.h](https://github.com/sysprog21/arch2026-progs/blob/fd2d7c17db223807c342e3f210c769d23d1865b1/tests/tperf.h) | Per-call instruction-budget measurement | `748652c9a820d891a0a783f33dd89b94e0dfc04fe298c6f8f090e56c02fb75de` |

Background quiz links, supplied by the handout: [Quiz 1](https://hackmd.io/@sysprog/arch2026-quiz1), [Quiz 2](https://hackmd.io/@sysprog/arch2026-quiz2), and [Quiz 3](https://hackmd.io/@sysprog/arch2026-quiz3). Their contents are not used to override the pinned contracts.

## Compiler, emulator, and Lab2 playground

- [xPack GNU RISC-V Embedded GCC release v15.2.0-1](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/tag/v15.2.0-1). Archive: `xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz`; SHA-256: `aaaa8060c914851a3e5ee1ba82cc3d6f80972f90638a05c6e823a37557a33758`. The compiler reports GCC 15.2.0. The archive checksum was checked against the published `.sha` file during setup.
- [rv32emu at the tested revision](https://github.com/sysprog21/rv32emu/tree/9dbff65c4d6a40d64d3bbb38f4c5a20d6e885e24): `9dbff65c4d6a40d64d3bbb38f4c5a20d6e885e24`. Both the main grading environment and the separate Lab2 run used system emulation with ELF loading and interpreter execution; M/A/F/C were disabled.
- [Official playground gist](https://gist.github.com/jserv/5f682ac880773cab69e3564f4f20d60a), inspected revision `914f7715f8d7f8678892a9fd91f78b40de351fc6`. Lab2 support logs preserve compiler identification, both builds, all test results, disassemblies, and a reproduction script.
- The Lab2 cycle reference is the official Lab2 handout in the snapshot table above. The observed O2 ChaCha20 cycle/instret difference is preserved as measured, not silently changed to match another counter.

## Prior Rubik work

The user already had **`RUBIK_N2_H48_ASSIGNMENT_SAFE_FINAL_20261003.zip`**, SHA-256 **`ce6a4e322acd1c9efdcf4b9bed28dcf407e02e212962794446b9f5a104b76cb4`**. No public URL is available and none is inferred. The archive supplies `assignment_safe/n2_solver.c` and the H48/B8 transformation, orbit, and distance tables. Their reuse is explicitly disclosed; they are not presented as newly invented in this session or as proof of the student's earlier personal contribution.

The new port adapts its public coordinates, interface, validation, output capacity handling, and RV32I implementation to the current assignment. The new assembly canonicalization preserves the scalar model while simplifying selected operations. A new independent BFS oracle derives the official coordinate graph from the current published move tables and verifies the port against that graph. The old archive's earlier timing claims are not substituted for this assignment's rv32emu measurements.

## CodeTrial

Upstream: [sysprog21/codetrial](https://github.com/sysprog21/codetrial), source-build revision **`64fbc228b411d59698d9bd27703f56263f18ac16`**. The README and task-mode instructions change; the preparation script supports refreshing and rebuilding them before the learner's actual attempt.

| Inspected file | Purpose | SHA-256 |
|---|---|---|
| [README.md](https://github.com/sysprog21/codetrial/blob/64fbc228b411d59698d9bd27703f56263f18ac16/README.md) | Lifecycle dependencies and current startup flow | `a4d8278bbe285b62ab35b691d0b47fbcfea68261bcc8f1f640163d2766cc23ac` |
| [docs/development.md](https://github.com/sysprog21/codetrial/blob/64fbc228b411d59698d9bd27703f56263f18ac16/docs/development.md) | Source-build and development details | `94300704d2c1b3408c70d4a745e123238701c00affb716ff41851ae716d9da64` |
| [docs/task-mode.md](https://github.com/sysprog21/codetrial/blob/64fbc228b411d59698d9bd27703f56263f18ac16/docs/task-mode.md) | Task behavior, proctoring, recording, result export | `6eb392d915014f1f3589cc40b8537e8e6cad0a3f77e7fd38f50fce529ad7af54` |
| [rust-toolchain.toml](https://github.com/sysprog21/codetrial/blob/64fbc228b411d59698d9bd27703f56263f18ac16/rust-toolchain.toml) | Pinned Rust toolchain | `ea8000ae718d3c924b119842025f03b9585b69cd924331bcd78dc0e7f27c2e2b` |

The delivery's CodeTrial support folder records the successful source build, version, asset checksums, 25 passing offline tests, and unconfigured HTTP smoke test. Its scripts automate setup, source building, and checks. Running that workflow on the learner's computer requires access to that device; building is not inherently a human-only task. The support material contains no Gemini or LiveKit credentials, real interview feedback, synthetic recording, or fabricated learner performance. These local setup checks establish readiness of the inspected source build, not completion of the personal course interview.

## This implementation and measured evidence

The report identifies the exact code-and-evidence commit. This register and its relative evidence links are included in that commit; its [implementation](../src/) and [supplementary checks](../mytests/) can be inspected alongside the evidence. The sources and report disclose ChatGPT's contribution; no AI-authored assembly is represented as the student's personally hand-written work.

| Evidence file | What it establishes | Main limitation |
|---|---|---|
| [grade_final_local.log](evidence/grade_final_local.log) | Official local driver: 100.0/100 | Does not grade personal authorship, report quality, or the interview |
| [grade_trusted.log](evidence/grade_trusted.log) | Fresh trusted official-framework rebuild: 100.0/100, all original gates retained | Automated grading remains separate from personal assessment |
| [grade_initial.log](evidence/grade_initial.log), [grade_solved_exit.log](evidence/grade_solved_exit.log) | Actual Rubik speed-gate failures before the final change | Historical local measurements, not final performance |
| [cycles.csv](evidence/cycles.csv), [cycles.md](evidence/cycles.md) | Every function at all four levels, checksums, baselines, same-ELF comparisons | Executed guest instructions on the documented inputs, not physical CPU time |
| [footprint.csv](evidence/footprint.csv) | Allocated implementation section sizes, harness excluded | Stack is analyzed separately |
| [sanitize.log](evidence/sanitize.log) | Host ASan/UBSan runs completed with `LSAN_OPTIONS=detect_leaks=0 make sanitize` | Only leak detection disabled for tracing; ASan/UBSan active; no implementation allocates |
| [canoncheck.log](evidence/canoncheck.log) | 19,040 target differential checks of H48 kernels | Random complete-state checks plus exhaustive permutation metadata, not all complete states |
| [rubik_oracle.json](evidence/rubik_oracle.json) | Entire state graph and exact radius-eight predicate; all farthest C solutions replayed | Full-state graph exists only in the host verifier |
| [rubik_d11.json](evidence/rubik_d11.json) | All 2,644 distance-11 inputs solved and replayed by target assembly; worst 878,484 | Does not execute every lower-distance state on target |
| [rubik_instruction_bound.json](evidence/rubik_instruction_bound.json), [proof](../mytests/RUBIK_ASSEMBLY_BOUND.md) | All-input static assembly call-tree upper bound: 5,632,744, below 7,500,000 | Specific to four recorded source hashes and valid-memory API preconditions; excludes C and shared harness |
| [bf16_error.json](evidence/bf16_error.json) | Exhaustive square-root classes, complete significand grid, explicit narrowing/underflow samples | Domain-specific error figures; not an IEEE-compliance claim |
| [fixed_error.json](evidence/fixed_error.json) | 2,228,480 exact comparisons and attained nonwrapping error endpoints | Wrapping real values have no half-ULP guarantee |
| [benchmarks_before_qmul_optimization.json](evidence/benchmarks_before_qmul_optimization.json) | Actual first assembly multiplication measurement of 236 instructions/call | A recorded unsuccessful intermediate version |

Independent verification programs are versioned under `mytests/`. Their reproducible outputs include source hashes and, for the target farthest-state run, an exact assembly snapshot. Larger oracle arrays and test ELFs are retained in the delivery's extended evidence rather than committed as implementation data. The report distinguishes exact mathematical derivations, exhaustive checks, sampled experiments, observed maxima, and operational setup checks.
