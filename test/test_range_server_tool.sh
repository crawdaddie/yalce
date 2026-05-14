#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RANGE_SERVER="$ROOT_DIR/build/tools/ylc_range_server"

if [[ ! -x "$RANGE_SERVER" ]]; then
  echo "missing range server binary: $RANGE_SERVER" >&2
  exit 1
fi

output="$(
  cat <<'EOF' | "$RANGE_SERVER"
{"id":1,"method":"update","path":"/tmp/bad.ylc","text":"let Kick = module () ->\n  let buf = ;\n"}
{"id":2,"method":"top_level_at","path":"/tmp/bad.ylc","line":1,"column":0}
{"id":3,"method":"update","path":"/tmp/good.ylc","text":"let x = 1;\nlet y = 2"}
{"id":4,"method":"top_level_at","path":"/tmp/good.ylc","line":2,"column":0}
EOF
)"

echo "$output"

grep -q '"id":2,"ok":false,"error":"no top-level node at line"' <<<"$output"
grep -q '"id":4,"ok":true' <<<"$output"
grep -q '"start_line":2' <<<"$output"

import_output="$(
  cat <<'EOF' | "$RANGE_SERVER"
{"id":10,"method":"update","path":"/home/adam/projects/yalce/cool_beats_jit.ylc","text":"open DSP;\nopen engine/bindings/Synth; \nopen engine/bindings/Sched; \nimport std/Math;\n\nlet q = 60. / 130.;\n\nplay_pattern q (\n  yield 0.25\n)\n"}
{"id":11,"method":"top_level_at","path":"/home/adam/projects/yalce/cool_beats_jit.ylc","line":8,"column":0}
EOF
)"

echo "$import_output"

grep -q '"id":11,"ok":true' <<<"$import_output"
grep -q '"start_line":8' <<<"$import_output"
