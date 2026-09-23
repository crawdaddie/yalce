#!/usr/bin/env python3
import struct
import sys
import pathlib
import os

THRESHOLD = 0.1
EXPECTED_PERIOD = float(os.environ.get("YLC_TIMING_PERIOD", "513"))


def read_left(path):
    raw = pathlib.Path(path).read_bytes()
    assert raw[:4] == b"RIFF"
    assert raw[8:12] == b"WAVE"

    offset = 12
    channels = None
    sample_rate = None
    data = None

    while offset + 8 <= len(raw):
        chunk_id = raw[offset:offset + 4]
        chunk_size = struct.unpack_from("<I", raw, offset + 4)[0]
        chunk = raw[offset + 8:offset + 8 + chunk_size]
        offset += 8 + chunk_size + (chunk_size & 1)

        if chunk_id == b"fmt ":
            format_id, channels, sample_rate, _, _, bits = struct.unpack(
                "<HHIIHH", chunk[:16]
            )
            assert format_id == 3
            assert bits == 32
        elif chunk_id == b"data":
            data = chunk

    assert channels is not None
    assert sample_rate == 48000
    assert data is not None

    values = struct.unpack("<%df" % (len(data) // 4), data)
    return values[0::channels]


def find_onsets(samples):
    onsets = []
    was_high = False

    for i, value in enumerate(samples):
        is_high = abs(value) > THRESHOLD
        if is_high and not was_high:
            onsets.append(i)
        was_high = is_high

    return onsets


def main(path):
    onsets = find_onsets(read_left(path))
    if len(onsets) < 2:
        raise AssertionError("expected at least two onsets")

    relative = [onset - onsets[0] for onset in onsets]
    expected = [int(i * EXPECTED_PERIOD) for i in range(len(onsets))]
    if relative != expected:
        intervals = [b - a for a, b in zip(relative, relative[1:])]
        raise AssertionError(
            "onsets differ from cumulative ticks: %s (expected %s)"
            % (intervals, [b - a for a, b in zip(expected, expected[1:])])
        )

    print("first onset: %d" % onsets[0])
    print("onsets: %d" % len(onsets))
    print("period: %.6f samples" % EXPECTED_PERIOD)


if __name__ == "__main__":
    main(sys.argv[1])
