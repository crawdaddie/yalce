#!/usr/bin/env python3
"""Compare tracker grammars without initializing audio."""
import difflib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
YLC = ROOT / 'build/ylc'
MALFORMED_ROW = '                  ;0           ;05'
INVALID_INSTRUMENT_CHANGE = (
    'G[N(11,-1,1,1,);N(60,5,1,2,);];',
    'G[N(-1,-1,1,1,);N(60,5,1,2,);];',
)


def source(name):
    text = (ROOT / f'{name}.ylc').read_text()
    # Playback handlers have an independent recursive-Atom LLVM type mismatch.
    parser = text[:text.index('let handle_ev =')]
    parser = parser.replace('let play_pattern = extern fn Double -> T -> Ptr;', '')
    tests = text[text.index('let test = module () ->'):]
    # Parenthesization in the original test body currently prevents inference.
    tests = tests.replace(
        '    note == 73 && instr == 1 &&\n'
        '    let n0, v0 = params[0];\n    let n1, v1 = params[1];\n',
        '    let n0, v0 = params[0];\n    let n1, v1 = params[1];\n'
        '    note == 73 && instr == 1 &&\n')
    tests = tests.replace(
        '      instr == 5 &&\n      let name, value = plist[0];\n',
        '      let name, value = plist[0];\n      instr == 5 &&\n')
    return parser, tests


def run(path, text, *flags):
    path.write_text(text)
    result = subprocess.run(
        [str(YLC), '--verify-ir', '--base', str(ROOT), *flags, str(path)],
        cwd=ROOT, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    assert not result.stderr, result.stderr
    return result.stdout


DUMP = r'''
let dump_params = fn params ->
  for i = 0 .. array_size params in (
    let name, value = params[i];
    print `{name}:{value},`
  ); ()
;;
let dump_atoms = fn items ->
  for item_idx = 0 .. array_size items in (
  match items[item_idx] with
  | Rest -> print "R"
  | Note (note, instr, params, dur, col) -> (
    print `N({note},{instr},{dur},{col},`;
    dump_params params;
    print ")"
  )
  | ParamSet (col, params) -> (
    print `P({col},`; dump_params params; print ")"
  )
  | JumpRel (steps, chance) -> print `J({steps},{chance})`
  | JumpAbs (target, chance) -> print `A({target},{chance})`
  | Group atoms -> (
    print "G[";
    dump_atoms atoms;
    print "]"
  )
  | Choice atoms -> (
    print "C[";
    dump_atoms atoms;
    print "]"
  ); print ";"
  ); ()
;;
'''


def main():
    corpus = (ROOT / 'vordhosbn.csv').read_text().splitlines()
    cases = [
        '', '_', '; 01c#5', '00; _; 01c#5', '01c#5 m80 V0f',
        '05 r02', 'JB04 m00', 'JF02', 'JA02 m80', 'ja00 ; ;04',
        '(00;(01;02));03', '00;', '00;;01', '00C_4', '00A-4',
    ]
    cases += [line for line in corpus if line and not line.startswith('#')]
    cases = list(dict.fromkeys(cases))
    calls = '\n'.join(
        f'dump_atoms [| parse_line {json.dumps(line)} |]; print "\\n";'
        for line in cases)
    outputs = {}
    with tempfile.TemporaryDirectory(prefix='tracker-combinators-') as temp:
        for name in ('Tracker', 'TrackerCombinators'):
            parser, tests = source(name)
            path = Path(temp) / f'{name}.ylc'
            result = run(path, parser + tests, '--test')
            totals = re.search(r'(\d+) / (\d+) passed', result)
            assert totals and totals[1] == totals[2], result
            print(f'{name}: {totals.group()}')
            outputs[name] = run(path, parser + DUMP + calls)
    expected, actual = outputs.values()
    # The original parses padding spaces as parameters with garbage values.
    expected = re.sub(r' :-?\d+,', '', expected)
    checked = []
    for case, before, after in zip(cases, expected.splitlines(), actual.splitlines()):
        if case == MALFORMED_ROW:
            assert (before, after) == INVALID_INSTRUMENT_CHANGE, (before, after)
            checked.append(after)
        else:
            checked.append(before)
    assert len(checked) == len(cases) == len(actual.splitlines())
    assert checked == actual.splitlines(), ''.join(difflib.unified_diff(
        checked, actual.splitlines(),
        fromfile='Tracker', tofile='TrackerCombinators'))
    print(f'Compared {len(cases)} rows, including vordhosbn.csv: '
          'matching ASTs except padding effects and one malformed instrument')


if __name__ == '__main__':
    main()
