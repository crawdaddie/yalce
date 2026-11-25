#!/usr/bin/env python3
"""Compare Tracker representations inside timed, instrumented JIT workloads."""
import csv
import os
from pathlib import Path
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
ITERATIONS = 3000
SAMPLES = 5
PARAM_COUNTS = (0, 1, 4, 8)
OUTPUT = Path(sys.argv[1] if len(sys.argv) > 1 else
              ROOT / 'test/mir_artifacts/tracker_params_bench_O3').resolve()
OUTPUT.mkdir(parents=True, exist_ok=True)

# glibc interposition counts requested allocation bytes without allocating itself.
helper = OUTPUT / 'measure.c'
library = OUTPUT / 'measure.so'
helper.write_text('''
#include <stddef.h>
#include <time.h>
extern void *__libc_malloc(size_t size);
extern void __libc_free(void *ptr);
static int active, allocations, releases, bytes;
static struct timespec started;
void *malloc(size_t size) {
  if (active) {
    allocations++;
    bytes += size;
  }
  return __libc_malloc(size);
}
void free(void *ptr) {
  if (active && ptr) {
    releases++;
  }
  __libc_free(ptr);
}
void bench_begin(void) {
  allocations = releases = bytes = 0;
  clock_gettime(CLOCK_MONOTONIC, &started);
  active = 1;
}
double bench_end(void) {
  struct timespec ended;
  active = 0;
  clock_gettime(CLOCK_MONOTONIC, &ended);
  return (ended.tv_sec - started.tv_sec)
       + (ended.tv_nsec - started.tv_nsec) * 1e-9;
}
int bench_allocs(void) { return allocations; }
int bench_frees(void) { return releases; }
int bench_bytes(void) { return bytes; }
''')
subprocess.run(['cc', '-O2', '-shared', '-fPIC', str(helper), '-o', str(library)],
               cwd=ROOT, check=True)

def source_for(representation):
    filename = 'Tracker.ylc' if representation == 'array' else 'Tracker_list.ylc'
    tracker = (ROOT / filename).read_text()
    (OUTPUT / filename).write_text(tracker)
    assert tracker.count('play_pattern 0. (') == 1
    tracker = tracker.replace('play_pattern 0. (', 'probe_queue 0. (')
    collection = 'Arrays' if representation == 'array' else 'Lists'
    source = '''open test/fixtures/audio_jit/DSP_noinit;
import std/Arrays;
import std/Lists;
let bench_begin = extern fn () -> ();
let bench_end = extern fn () -> Double;
let bench_allocs = extern fn () -> Int;
let bench_frees = extern fn () -> Int;
let bench_bytes = extern fn () -> Int;
let idle = fn () -> yield 0.;;
let pending = [|idle ()|];
let probe_queue = fn start: (Double) co -> pending[0] := co; ();;
let Tracker = module () ->
''' + tracker + '\n;\n'
    source = f'let _ = dlopen "{library}";\n' + source
    source += f'''
let checksum = [|0|];
let handle = fn ctx dur note instr params ->
  let total = {collection}.fold (fn sum (name, value) ->
    if name == 'r' then sum + value else sum
  ) 0 params;
  checksum[0] := checksum[0] + total + 1;
  ()
;;
let play = fn pattern ->
  let co = pattern
    |> Tracker.pattern_cor_ctx (bpm: 120., spl: 1., logical_time: 0.) 0
    |> cor_map (Tracker.handle_ev_ctx handle);
  let _ = co ();
  let _ = co ();
  ()
;;
let delayed = fn pattern ->
  play pattern;
  let co = pending[0];
  let _ = co ();
  let _ = co ();
  ()
;;
'''
    for count in PARAM_COUNTS:
        params = ''.join(f'r{i:02x} ' for i in range(1, count + 1))
        text = '00c4 ' + params + ';\n'
        delayed_text = '00c4 d80 ' + params + ';\n'
        lookup = '00c4 ' + 'd01 ' * count
        # Literals contain actual newlines; Yalce backticks preserve them.
        source += f'let pattern_{count} = Tracker.parse_pattern `{text}`;\n'
        source += f'let delayed_{count} = Tracker.parse_pattern `{delayed_text}`;\n'
        source += (f'let seed_{count} = match Tracker.parse_chunk_chars `{lookup}` with '
                   '| (_, _, params) -> params;\n')
        cases = {
            'parse': (f'let parsed = Tracker.parse_pattern `{text}`; '
                      'checksum[0] := checksum[0] + array_size parsed', 1),
            'lookup': (f'let value = match Tracker.scan_for_delay seed_{count} with '
                       '| Some value -> value | None -> 0; '
                       f'let probability = match Tracker.scan_for_maybe seed_{count} with '
                       '| Some value -> value | None -> 0; '
                       'checksum[0] := checksum[0] + value + probability', int(count > 0)),
            'playback': (f'play pattern_{count}', 1 + count * (count + 1) // 2),
            'delayed': (f'delayed delayed_{count}', 1 + count * (count + 1) // 2),
        }
        for phase, (body, expected) in cases.items():
            function = f'run_{phase}_{count}'
            source += f'''let {function} = fn n ->
  for i = 0 .. n in ({body}; ());
  ()
;;
{function} 100;
'''
            for sample in range(SAMPLES):
                source += f'''checksum[0] := 0;
bench_begin ();
{function} {ITERATIONS};
let seconds = bench_end ();
let allocations = bench_allocs ();
let frees = bench_frees ();
let bytes = bench_bytes ();
print `result,{phase},{count},{sample},{{seconds}},{{allocations}},{{frees}},{{bytes}},{{checksum[0] == {ITERATIONS * expected}}}\n`;
'''
    return source

rows = []
for representation in ('array', 'list'):
    path = OUTPUT / f'{representation}.ylc'
    path.write_text(source_for(representation))
    for mode in ('timing', 'allocations'):
        environment = dict(os.environ)
        if mode == 'allocations':
            environment['LD_PRELOAD'] = str(library)
        else:
            environment.pop('LD_PRELOAD', None)
        result = subprocess.run([str(ROOT / 'build/ylc'), '-O3', '--verify-ir',
                                 '--base', str(ROOT), str(path)],
                                cwd=ROOT, env=environment, text=True,
                                capture_output=True, timeout=120)
        (OUTPUT / f'{representation}.{mode}.log').write_text(result.stdout + result.stderr)
        assert result.returncode == 0, result.stdout + result.stderr
        measurements = [line.split(',') for line in result.stdout.splitlines()
                        if line.startswith('result,')]
        assert len(measurements) == len(PARAM_COUNTS) * 4 * SAMPLES, result.stdout + result.stderr
        for _, phase, count, sample, seconds, allocations, frees, size, valid in measurements:
            assert valid == 'True', (representation, phase, count, sample, valid)
            rows.append((representation, phase, int(count), int(sample),
                         float(seconds), int(allocations), int(frees), int(size), mode))
    print(f'{representation}: workload checks passed', flush=True)

with (OUTPUT / 'results.csv').open('w') as stream:
    writer = csv.writer(stream)
    writer.writerow(('representation', 'phase', 'params', 'sample', 'seconds',
                     'allocations', 'frees', 'requested_bytes', 'mode'))
    writer.writerows(rows)

summary = [f'O3, {ITERATIONS} iterations/sample, median of {SAMPLES} warmed samples.',
           'Times exclude JIT; allocations are measured in separate instrumented runs.',
           'Delayed cases contain one additional d80 parameter.',
           'phase params array_us list_us speedup array_alloc/op list_alloc/op array_net/op list_net/op']
for phase in ('parse', 'lookup', 'playback', 'delayed'):
    for count in PARAM_COUNTS:
        groups = [[row for row in rows if row[:3] == (kind, phase, count)]
                  for kind in ('array', 'list')]
        times = [statistics.median(row[4] for row in group if row[8] == 'timing') / ITERATIONS
                 for group in groups]
        allocations = [statistics.median(row[5] for row in group if row[8] == 'allocations') / ITERATIONS
                       for group in groups]
        net = [statistics.median(row[5] - row[6] for row in group if row[8] == 'allocations') / ITERATIONS
               for group in groups]
        summary.append(f'{phase:8} {count:2} {times[0]*1e6:8.3f} {times[1]*1e6:8.3f} '
                       f'{times[0]/times[1]:6.2f}x {allocations[0]:8.2f} {allocations[1]:8.2f} '
                       f'{net[0]:8.2f} {net[1]:8.2f}')
report = '\n'.join(summary) + '\n'
(OUTPUT / 'summary.txt').write_text(report)
print(report)
print(f'Artifacts: {OUTPUT}')
