#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 OUTPUT_DIRECTORY" >&2
    exit 2
fi
task_out="$(python3 -c 'import os,sys;print(os.path.abspath(sys.argv[1]))' "$1")"
task_repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_prefix="${CROSS_COMPILE:-riscv-none-elf-}"
task_emu="${RV32EMU:-$task_repo/third_party/rv32emu/build/rv32emu}"
mkdir -p "$task_out/snapshot/src/rubik"
cd "$task_repo"
python3 - "$task_out" <<'PY'
import hashlib,json,sys
from pathlib import Path
out=Path(sys.argv[1]);manifest={}
for name in ['rubik.S','rubik_search.inc','rubik_canon.inc','rubik_tables.inc']:
 p=Path('src/rubik')/name;data=p.read_bytes();(out/'snapshot/src/rubik'/name).write_bytes(data)
 manifest[str(p)]=hashlib.sha256(data).hexdigest()
for p in [Path('mytests/rubik_d11_target.c'),Path('mytests/rubik_d11_cases.inc'),Path('src/rubik/rubik.h'),Path('tests/tperf.h'),*Path('common').glob('*')]:
 if p.is_file():manifest[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
(out/'source_sha256.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY
"${task_prefix}gcc" --version > "$task_out/compiler.txt"
task_cflags=(-march=rv32i_zicsr -mabi=ilp32 -O2 -g -ffreestanding -fno-pic
    -fno-stack-protector -Wall -Wextra -Werror -Wvla -iquote . -Icommon -Itests)
"${task_prefix}gcc" "${task_cflags[@]}" -c mytests/rubik_d11_target.c -o "$task_out/main.o"
for task_unit in rt print; do
    "${task_prefix}gcc" "${task_cflags[@]}" -c "common/$task_unit.c" -o "$task_out/$task_unit.o"
done
for task_unit in start perf; do
    "${task_prefix}as" -march=rv32i_zicsr -mabi=ilp32 -g "common/$task_unit.S" -o "$task_out/$task_unit.o"
done
(
    cd "$task_out/snapshot"
    "${task_prefix}as" -march=rv32i_zicsr -mabi=ilp32 -g src/rubik/rubik.S -o "$task_out/impl_asm.o"
)
"${task_prefix}ld" -T common/linker.ld --no-warn-rwx-segments \
    -o "$task_out/rubik_d11.elf" "$task_out/main.o" "$task_out/impl_asm.o" \
    "$task_out/start.o" "$task_out/perf.o" "$task_out/rt.o" "$task_out/print.o"
python3 scripts/rvcheck.py --prefix "$task_prefix" --impl asm "$task_out/impl_asm.o"
python3 scripts/rvcheck.py --prefix "$task_prefix" "$task_out/rubik_d11.elf"
python3 - "$task_out" "$task_emu" <<'PY'
import json,subprocess,sys,time
from pathlib import Path
out=Path(sys.argv[1]);start=time.monotonic();summary=None
p=subprocess.Popen([sys.argv[2],str(out/'rubik_d11.elf')],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
with (out/'run.log').open('w') as log:
 for line in p.stdout:
  log.write(line);log.flush();print(line,end='',flush=True)
  if line.startswith('@@d11 '):summary=dict(field.split('=',1) for field in line.strip().split()[1:])
rc=p.wait();result={'exit_code':rc,'wall_seconds':time.monotonic()-start,'summary':summary}
(out/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
if rc:raise SystemExit(rc)
if summary is None or summary.get('cases')!='2644' or summary.get('failed')!='0':raise SystemExit('missing/failed d11 summary')
PY
