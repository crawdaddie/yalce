#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LSP_SERVER="$ROOT_DIR/build/tools/ylc_lsp_server"

if [[ ! -x "$LSP_SERVER" ]]; then
  echo "missing lsp server binary: $LSP_SERVER" >&2
  exit 1
fi

make_msg() {
  local body="$1"
  printf 'Content-Length: %d\r\n\r\n%s' "${#body}" "$body"
}

payload=""
payload+="$(make_msg '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"initialized","params":{}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/test.ylc","languageId":"ylc","version":1,"text":"let Kick = module () ->\n  let trig = fn () ->\n    1\n  ;;\n;\nlet y = 2\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":2,"method":"textDocument/documentSymbol","params":{"textDocument":{"uri":"file:///tmp/test.ylc"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":3,"method":"textDocument/selectionRange","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"positions":[{"line":5,"character":0}]}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":4,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":2,"character":4}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":5,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":5,"character":1}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/completion_test.ylc","languageId":"ylc","version":1,"text":"open DSP;\nlet x = gr\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":6,"method":"textDocument/completion","params":{"textDocument":{"uri":"file:///tmp/completion_test.ylc"},"position":{"line":1,"character":10}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/selection_repro.ylc","languageId":"ylc","version":1,"text":"open DSP;\n\nlet chirp_buf = load_soundfile \"~/Sounds/chirp.wav\";\n\ncompile_audio_fn (\nlet x = fn () ->\n  grains 100 (array_of_buf chirp_buf) 1 (lfnoise 2. 0. 0.9) 0.05 (trig 60)\n);\nx () |> ignore\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":7,"method":"textDocument/selectionRange","params":{"textDocument":{"uri":"file:///tmp/selection_repro.ylc"},"positions":[{"line":5,"character":4}]}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":8,"method":"shutdown","params":null}')"

output="$(printf '%s' "$payload" | YLC_BASE_DIR="$ROOT_DIR" "$LSP_SERVER" | tr -d '\r')"

echo "$output"

grep -q '"id":1,"result":{"capabilities":{"textDocumentSync":{"openClose":true,"change":1},"documentSymbolProvider":true,"selectionRangeProvider":true,"hoverProvider":true,"completionProvider":{"resolveProvider":false' <<<"$output"
grep -q '"triggerCharacters":\["."\]' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/test.ylc","diagnostics":\[\]}' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/completion_test.ylc","diagnostics":\[\]}' <<<"$output"
grep -q '"id":2,"result":\[{"name":"Kick","kind":2' <<<"$output"
grep -q '"name":"y","kind":13' <<<"$output"
grep -q '"id":3,"result":\[{"range":{"start":{"line":5,"character":0},"end":{"line":5,"character":9}}}\]' <<<"$output"
grep -q '"id":4,"result":{"contents":{"kind":"markdown","value":"`Int`"}' <<<"$output"
grep -q '"id":5,"result":{"contents":{"kind":"markdown","value":"`y : Int`"}' <<<"$output"
grep -q '"id":6,"result":{"isIncomplete":false,"items":\[' <<<"$output"
grep -q '"label":"grains","kind":3,"insertText":"grains","filterText":"grains","sortText":"grains"' <<<"$output"
grep -q '"id":7,"result":\[{"range":{"start":{"line":4,"character":0},"end":{"line":7,"character":2}}}\]' <<<"$output"
if grep -q '"detail":"' <<<"$output"; then
  echo "unexpected completion detail field present" >&2
  exit 1
fi
grep -q '"id":8,"result":null' <<<"$output"
