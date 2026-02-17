import sys
import re
import signal

signal.signal(signal.SIGINT, lambda *_: None)

ticks = []
try:
    for line in sys.stdin:
        m = re.search(r'hh tick (\d+)', line)
        if m:
            ticks.append(int(m.group(1)))
except KeyboardInterrupt:
    pass

if len(ticks) < 2:
    print("Need at least 2 tick values")
    sys.exit(1)

deltas = [ticks[i+1] - ticks[i] for i in range(len(ticks)-1)]
mean = sum(deltas) / len(deltas)
min_d = min(deltas)
max_d = max(deltas)
jitter = max_d - min_d

expected = 0.25 * (60.0 / 130) * 48000

drift_per_tick = mean - expected
total_drift = drift_per_tick * len(deltas)

print(f"\nticks:          {len(ticks)}")
print(f"expected delta: {expected:.2f}")
print(f"mean delta:     {mean:.2f}")
print(f"min delta:      {min_d}")
print(f"max delta:      {max_d}")
print(f"jitter:         {jitter} samples ({jitter/48:.3f} ms)")
print(f"drift/tick:     {drift_per_tick:+.2f} samples")
print(f"total drift:    {total_drift:+.1f} samples ({total_drift/48:+.3f} ms)")
