#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source=$(mktemp --suffix=.ylc)
output=$(mktemp)
trap 'rm -f "$source" "$output"' EXIT
cat > "$source" <<'YLC'
import Json;
let dtype = fn object ->
  match Json.get "dtype" object with
  | Some value -> (
      let text = Json.string value;
      if text == "BF16" then 2 else 1
    )
  | None -> 0
;;
let object = Json.parse "{\"dtype\":\"BF16\"}";
for i = 0 .. 16 in (print `{dtype object}\n`);
YLC
./build/ylc "$source" > "$output"
python3 - "$output" <<'PY'
from pathlib import Path
import sys
assert Path(sys.argv[1]).read_text().splitlines() == ['2'] * 16
PY
