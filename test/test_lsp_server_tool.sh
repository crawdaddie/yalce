#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LSP_SERVER="$ROOT_DIR/build/tools/ylc_lsp_server"

if [[ ! -x "$LSP_SERVER" ]]; then
  echo "missing lsp server binary: $LSP_SERVER" >&2
  exit 1
fi

cat > /tmp/DefMod.ylc <<'YLC'
let exported = 41
YLC

make_msg() {
  local body="$1"
  printf 'Content-Length: %d\r\n\r\n%s' "${#body}" "$body"
}

payload=""
payload+="$(make_msg '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"initialized","params":{}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/test.ylc","languageId":"ylc","version":1,"text":"let Kick = module () ->\n  let trig = fn () ->\n    1\n  ;;\n;\nlet y = 2;\nlet z = y\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":2,"method":"textDocument/documentSymbol","params":{"textDocument":{"uri":"file:///tmp/test.ylc"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":3,"method":"textDocument/selectionRange","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"positions":[{"line":5,"character":0}]}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":4,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":2,"character":4}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":5,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":5,"character":1}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/completion_test.ylc","languageId":"ylc","version":1,"text":"open libs/audio_jit/DSP;\nlet x = gr\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":6,"method":"textDocument/completion","params":{"textDocument":{"uri":"file:///tmp/completion_test.ylc"},"position":{"line":1,"character":10}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/selection_repro.ylc","languageId":"ylc","version":1,"text":"open libs/audio_jit/DSP;\n\nlet chirp_buf = load_soundfile \"~/Sounds/chirp.wav\";\n\ncompile_audio_fn (\nlet x = fn () ->\n  grains 100 (array_of_buf chirp_buf) 1 (lfnoise 2. 0. 0.9) 0.05 (trig 60)\n);\nx () |> ignore\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":7,"method":"textDocument/selectionRange","params":{"textDocument":{"uri":"file:///tmp/selection_repro.ylc"},"positions":[{"line":5,"character":4}]}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":8,"method":"textDocument/definition","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":6,"character":8}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/definition_import.ylc","languageId":"ylc","version":1,"text":"open DefMod;\nlet a = exported;\nimport DefMod;\nlet b = DefMod.exported\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":9,"method":"textDocument/definition","params":{"textDocument":{"uri":"file:///tmp/definition_import.ylc"},"position":{"line":1,"character":10}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":10,"method":"textDocument/definition","params":{"textDocument":{"uri":"file:///tmp/definition_import.ylc"},"position":{"line":3,"character":16}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/change_def.ylc","languageId":"ylc","version":1,"text":"let old = 1;\nlet y = old\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didChange","params":{"textDocument":{"uri":"file:///tmp/change_def.ylc","version":2},"contentChanges":[{"text":"let fresh = 1;\nlet y = fresh\n"}]}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":11,"method":"textDocument/definition","params":{"textDocument":{"uri":"file:///tmp/change_def.ylc"},"position":{"line":1,"character":9}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":12,"method":"textDocument/rename","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":6,"character":8},"newName":"renamed"}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/rename_shadow.ylc","languageId":"ylc","version":1,"text":"let x = 1;\nlet outer = x;\nlet f = fn x ->\n  x\n;;\nlet again = x\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":13,"method":"textDocument/rename","params":{"textDocument":{"uri":"file:///tmp/rename_shadow.ylc"},"position":{"line":1,"character":12},"newName":"top"}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":14,"method":"textDocument/rename","params":{"textDocument":{"uri":"file:///tmp/definition_import.ylc"},"position":{"line":1,"character":10},"newName":"other"}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":15,"method":"textDocument/references","params":{"textDocument":{"uri":"file:///tmp/test.ylc"},"position":{"line":6,"character":8},"context":{"includeDeclaration":true}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":16,"method":"textDocument/references","params":{"textDocument":{"uri":"file:///tmp/rename_shadow.ylc"},"position":{"line":1,"character":12},"context":{"includeDeclaration":false}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/module_member_refs.ylc","languageId":"ylc","version":1,"text":"let Audio = extern fn (T: (Double ... -> Double)) -> T;\nlet Decor = module () ->\n  let member = @Audio fn x ->\n    x\n  ;;\n;\nlet out = Decor.member 1.\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":17,"method":"textDocument/references","params":{"textDocument":{"uri":"file:///tmp/module_member_refs.ylc"},"position":{"line":2,"character":8},"context":{"includeDeclaration":true}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///tmp/syntax_error.ylc","languageId":"ylc","version":1,"text":"let ok = 1;\nlet bad = )\nlet after = 2\n"}}}')"
payload+="$(make_msg '{"jsonrpc":"2.0","id":18,"method":"shutdown","params":null}')"

output="$(printf '%s' "$payload" | YLC_BASE_DIR="$ROOT_DIR" "$LSP_SERVER" | tr -d '\r')"

echo "$output"

grep -q '"id":1,"result":{"capabilities":{"textDocumentSync":{"openClose":true,"change":1},"documentSymbolProvider":true,"selectionRangeProvider":true,"hoverProvider":true,"definitionProvider":true,"referencesProvider":true,"renameProvider":true,"completionProvider":{"resolveProvider":false' <<<"$output"
grep -q '"triggerCharacters":\["."\]' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/test.ylc","diagnostics":\[\]}' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/completion_test.ylc","diagnostics":\[\]}' <<<"$output"
grep -q '"id":2,"result":\[{"name":"Kick","kind":2' <<<"$output"
grep -q '"name":"y","kind":13' <<<"$output"
grep -q '"id":3,"result":\[{"range":{"start":{"line":5,"character":0},"end":{"line":6,"character":0}}}\]' <<<"$output"
grep -q '"id":4,"result":{"contents":{"kind":"markdown","value":"`Int`"}' <<<"$output"
grep -q '"id":5,"result":{"contents":{"kind":"markdown","value":"`y : Int`"}' <<<"$output"
grep -q '"id":6,"result":{"isIncomplete":false,"items":\[' <<<"$output"
grep -q '"label":"grains","kind":3,"insertText":"grains","filterText":"grains","sortText":"grains"' <<<"$output"
grep -q '"id":7,"result":\[{"range":{"start":{"line":4,"character":0},"end":{"line":7,"character":2}}}\]' <<<"$output"
grep -q '"id":8,"result":\[{"uri":"file:\\/\\/\\/tmp\\/test.ylc","range":{"start":{"line":5,"character":4},"end":{"line":5,"character":5}}}\]' <<<"$output"
grep -q '"id":9,"result":\[{"uri":"file:\\/\\/\\/tmp\\/DefMod.ylc","range":{"start":{"line":0,"character":4},"end":{"line":0,"character":12}}}\]' <<<"$output"
grep -q '"id":10,"result":\[{"uri":"file:\\/\\/\\/tmp\\/DefMod.ylc","range":{"start":{"line":0,"character":4},"end":{"line":0,"character":12}}}\]' <<<"$output"
grep -q '"id":11,"result":\[{"uri":"file:\\/\\/\\/tmp\\/change_def.ylc","range":{"start":{"line":0,"character":4},"end":{"line":0,"character":9}}}\]' <<<"$output"
grep -q '"id":12,"result":{"changes":{"file:\\/\\/\\/tmp\\/test.ylc":\[{"range":{"start":{"line":5,"character":4},"end":{"line":5,"character":5}},"newText":"renamed"},{"range":{"start":{"line":6,"character":8},"end":{"line":6,"character":9}},"newText":"renamed"}\]}}}' <<<"$output"
grep -q '"id":13,"result":{"changes":{"file:\\/\\/\\/tmp\\/rename_shadow.ylc":\[{"range":{"start":{"line":0,"character":4},"end":{"line":0,"character":5}},"newText":"top"},{"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":13}},"newText":"top"},{"range":{"start":{"line":5,"character":12},"end":{"line":5,"character":13}},"newText":"top"}\]}}}' <<<"$output"
grep -q '"id":14,"result":null' <<<"$output"
grep -q '"id":15,"result":\[{"uri":"file:\\/\\/\\/tmp\\/test.ylc","range":{"start":{"line":5,"character":4},"end":{"line":5,"character":5}}},{"uri":"file:\\/\\/\\/tmp\\/test.ylc","range":{"start":{"line":6,"character":8},"end":{"line":6,"character":9}}}\]' <<<"$output"
grep -q '"id":16,"result":\[{"uri":"file:\\/\\/\\/tmp\\/rename_shadow.ylc","range":{"start":{"line":1,"character":12},"end":{"line":1,"character":13}}},{"uri":"file:\\/\\/\\/tmp\\/rename_shadow.ylc","range":{"start":{"line":5,"character":12},"end":{"line":5,"character":13}}}\]' <<<"$output"
grep -q '"id":17,"result":\[{"uri":"file:\\/\\/\\/tmp\\/module_member_refs.ylc","range":{"start":{"line":2,"character":6},"end":{"line":2,"character":12}}},{"uri":"file:\\/\\/\\/tmp\\/module_member_refs.ylc","range":{"start":{"line":6,"character":16},"end":{"line":6,"character":22}}}\]' <<<"$output"
grep -q '"method":"textDocument\\/publishDiagnostics","params":{"uri":"file:\\/\\/\\/tmp\\/syntax_error.ylc","diagnostics":\[{"range":{"start":{"line":1,"character":11},"end":{"line":1,"character":12}},"severity":1,"source":"ylc","message":"syntax error near '\'''\)'\''"}\]}' <<<"$output"
if grep -q '"detail":"' <<<"$output"; then
  echo "unexpected completion detail field present" >&2
  exit 1
fi
grep -q '"id":18,"result":null' <<<"$output"
