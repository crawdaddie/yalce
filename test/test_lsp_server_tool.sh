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
payload+="$(make_msg '{"jsonrpc":"2.0","id":4,"method":"shutdown","params":null}')"

output="$(printf '%s' "$payload" | "$LSP_SERVER" | tr -d '\r')"

echo "$output"

grep -q '"id":1,"result":{"capabilities":{"textDocumentSync":{"openClose":true,"change":1},"documentSymbolProvider":true,"selectionRangeProvider":true}}' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/test.ylc","diagnostics":\[\]}' <<<"$output"
grep -q '"id":2,"result":\[{"name":"Kick","kind":2' <<<"$output"
grep -q '"name":"y","kind":13' <<<"$output"
grep -q '"id":3,"result":\[{"range":{"start":{"line":5,"character":0},"end":{"line":5,"character":9}}}\]' <<<"$output"
grep -q '"id":4,"result":null' <<<"$output"
