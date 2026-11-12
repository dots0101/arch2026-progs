# Cycle analysis

- compiler: riscv-none-elf-gcc 15.2.0, -march=rv32i_zicsr -mabi=ilp32 -ffreestanding
- rv32emu: 9dbff65 (system emulation, ELF_LOADER, interpreter)
- timed region: n calls on identical inputs; loop baseline subtracted

rv32emu has no timing model: `cycle` and `instret` are one counter of executed guest instructions. The figures below are instruction counts per call, with the loop control, input load and counter reads removed.

## Cycles per call

| Function | -O1 | -O2 | -O3 | -Os | asm | best C | asm vs best C |
|---|---|---|---|---|---|---|---|
| bf16.from_f32 | 11.1 | 11.2 | 12.8 | 11.2 | 11.2 | 11.1 | 0.98x |
| bf16.mul | 90.6 | 81.5 | 82.9 | 85.5 | 53.1 | 81.5 | 1.53x |
| bf16.sqrt | 54.9 | 51.6 | 39.3 | 49.8 | 36.3 | 39.3 | 1.04x |
| fixed.op1 | 24.8 | 22.1 | 24.2 | 24.7 | 22.1 | 22.1 | 1.00x |
| fixed.op2 | 4.0 | 4.0 | 5.9 | 4.0 | 4.0 | 4.0 | 1.00x |
| fixed.op3 | 7.0 | 7.0 | 8.9 | 7.0 | 7.0 | 7.0 | 1.00x |
| fixed.q_mul | 212.7 | 251.1 | 253.1 | 239.9 | 208.0 | 212.7 | 1.02x |
| gcd.gcd | 104.8 | 126.9 | 129.1 | 131.6 | 84.2 | 104.8 | 1.24x |
| rubik.solve | 130529.0 | 123960.0 | 84019.0 | 133781.8 | 68506.8 | 84019.0 | 1.23x |

## Ratio to -O2

| Function | -O1 | -O2 | -O3 | -Os | asm |
|---|---|---|---|---|---|
| bf16.from_f32 | 0.99 | 1.00 | 1.15 | 1.00 | 1.01 |
| bf16.mul | 1.11 | 1.00 | 1.02 | 1.05 | 0.65 |
| bf16.sqrt | 1.07 | 1.00 | 0.76 | 0.96 | 0.70 |
| fixed.op1 | 1.12 | 1.00 | 1.10 | 1.12 | 1.00 |
| fixed.op2 | 1.00 | 1.00 | 1.48 | 1.00 | 1.00 |
| fixed.op3 | 1.00 | 1.00 | 1.28 | 1.00 | 1.00 |
| fixed.q_mul | 0.85 | 1.00 | 1.01 | 0.96 | 0.83 |
| gcd.gcd | 0.83 | 1.00 | 1.02 | 1.04 | 0.66 |
| rubik.solve | 1.05 | 1.00 | 0.68 | 1.08 | 0.55 |

## Footprint (bytes, `size -A`, harness excluded)

| Program | Variant | .text | .rodata | .data | .bss |
|---|---|---|---|---|---|
| rubik | asm | 4280 | 87860 | 0 | 0 |
| rubik | -O1 | 5440 | 87910 | 0 | 0 |
| rubik | -O2 | 5340 | 87898 | 0 | 0 |
| rubik | -O3 | 15288 | 87878 | 0 | 0 |
| rubik | -Os | 4296 | 87898 | 0 | 0 |
| bf16 | asm | 760 | 0 | 0 | 0 |
| bf16 | -O1 | 676 | 0 | 0 | 0 |
| bf16 | -O2 | 628 | 0 | 0 | 0 |
| bf16 | -O3 | 776 | 0 | 0 | 0 |
| bf16 | -Os | 564 | 0 | 0 | 0 |
| gcd | asm | 156 | 0 | 0 | 0 |
| gcd | -O1 | 168 | 0 | 0 | 0 |
| gcd | -O2 | 176 | 0 | 0 | 0 |
| gcd | -O3 | 176 | 0 | 0 | 0 |
| gcd | -Os | 148 | 0 | 0 | 0 |
| fixed | asm | 168 | 0 | 0 | 0 |
| fixed | -O1 | 244 | 0 | 0 | 0 |
| fixed | -O2 | 236 | 0 | 0 | 0 |
| fixed | -O3 | 236 | 0 | 0 | 0 |
| fixed | -Os | 172 | 0 | 0 | 0 |
