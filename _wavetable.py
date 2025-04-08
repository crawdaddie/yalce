#!/usr/bin/env -S uv --quiet run --script
# /// script
# requires-python = ">=3.13"
# dependencies = [
#     "numpy",
#     "matplotlib"
# ]
# ///
#
import sys

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.widgets import Slider


def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <saw_wave_file.csv>")
        return

    with open(sys.argv[1], "r") as f:
        cleaned_data = [float(line.strip().rstrip(",")) for line in f]
    saw_data = np.array(cleaned_data)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.subplots_adjust(bottom=0.25)  # Make room for the slider

    ax1.plot(saw_data)
    ax1.set_title("Original Saw Wave")
    ax1.grid(True)

    initial_duty_cycle = 0.5

    square_wave = create_square_wave(saw_data, initial_duty_cycle)

    (square_line,) = ax2.plot(square_wave)
    ax2.set_title(f"Square Wave (Duty Cycle: {initial_duty_cycle*100:.1f}%)")
    ax2.set_ylim(-1.2, 1.2)
    ax2.grid(True)

    ax_slider = plt.axes([0.15, 0.1, 0.7, 0.03])
    duty_slider = Slider(
        ax=ax_slider,
        label="Duty Cycle",
        valmin=0.01,
        valmax=0.99,
        valinit=initial_duty_cycle,
        valstep=0.01,
    )

    def update(val):
        duty_cycle = duty_slider.val
        square_wave = create_square_wave(saw_data, duty_cycle)
        square_line.set_ydata(square_wave)
        ax2.set_title(f"Square Wave (Duty Cycle: {duty_cycle*100:.1f}%)")
        fig.canvas.draw_idle()

    duty_slider.on_changed(update)

    plt.tight_layout()
    plt.subplots_adjust(bottom=0.2)  # Adjust again after tight_layout
    plt.show()


def create_square_wave(saw_data, duty_cycle=0.5):
    """
    Create a square wave by summing the original saw wave with a shifted, flipped copy

    Args:
        saw_data: NumPy array containing the saw wave
        duty_cycle: Value between 0 and 1 representing the duty cycle

    Returns:
        NumPy array containing the square wave
    """
    flipped_saw = -saw_data.copy()

    offset_samples = int(len(saw_data) * duty_cycle)

    offset_flipped = np.roll(flipped_saw, offset_samples)

    dc = duty_cycle if duty_cycle < 0.5 else -1.0 * (0.5 - duty_cycle)
    # square_wave = (0.5 + dc) * (saw_data + offset_flipped)
    square_wave = saw_data + offset_flipped

    return square_wave


if __name__ == "__main__":
    main()
