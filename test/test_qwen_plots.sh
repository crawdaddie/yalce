#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
plot_dir=$(mktemp -d)
trap 'rm -rf "$plot_dir"' EXIT
python3 - "$plot_dir/data.bin" <<'PYDATA'
from pathlib import Path
import struct
import sys
Path(sys.argv[1]).write_bytes(struct.pack('<6f', 0, 1, 0, 1, 0, 1))
PYDATA
cat > "$plot_dir/smoke.ylc" <<YLC
import libs/Gnuplot/Gnuplot;
Gnuplot.start ()
  |> Gnuplot.terminal_kitty 640 480
  |> Gnuplot.multiplot "Gnuplot stage plots" 2 1
  |> Gnuplot.title "Gnuplot import smoke test"
  |> Gnuplot.begin_plot
  |> Gnuplot.binary_row "$plot_dir/data.bin" 3 0
  |> Gnuplot.plot_newline
  |> Gnuplot.binary_image "$plot_dir/data.bin" 3 2
  |> Gnuplot.write "unset multiplot\n"
  |> Gnuplot.finish;
YLC
./build/ylc "$plot_dir/smoke.ylc" > "$plot_dir/graphics"
python3 - "$plot_dir/graphics" <<'PY'
from pathlib import Path
import sys
assert b'\x1b_G' in Path(sys.argv[1]).read_bytes()
PY
