#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 OUTPUT_DIRECTORY" >&2
    exit 2
fi
task_out="$(python3 -c 'import os,sys;print(os.path.abspath(sys.argv[1]))' "$1")"
task_repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$task_out"
cd "$task_repo"
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I. \
    mytests/fixed_error.c src/fixed/fixed.c -o "$task_out/fixed_error"
"$task_out/fixed_error" | tee "$task_out/fixed_error.json"
