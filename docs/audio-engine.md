# Audio Engine

YALCE includes a real-time audio engine designed for low-latency audio synthesis and processing.

## Architecture

The audio engine uses a **node graph** architecture where:

- **Nodes** are individual audio processing units (oscillators, filters, effects)
- **Graph** connects nodes together to create signal flow
- **Real-time thread** processes audio with guaranteed low latency
- **Message queue** allows safe updates from non-RT thread

### Key Components

1. **AudioGraph** - Container for audio nodes with memory pooling
2. **Node** - Basic audio processing unit with perform function
3. **Scheduling** - Message-based graph updates
4. **Ring Buffer** - RT-safe communication between threads

## Real-Time Safety

The audio engine is designed for hard real-time constraints:

- **No allocation** in audio callback (`malloc`/`free`)
- **Lock-free** data structures for inter-thread communication
- **Pre-allocated** memory pools for nodes and buffers
- **Message queue** for safe graph modifications

## Basic Usage

### Starting the Audio Engine

```ylc
import Audio;

# Initialize audio engine
Audio.init 44100 256;;  # sample_rate, buffer_size

# Start audio processing
Audio.start;;
```

### Creating Audio Nodes

```ylc
# Simple sine wave oscillator
let osc = Audio.sin_osc 440.0;;  # 440 Hz (A4)

# Connect to output
Audio.connect osc Audio.output;;

# Start playback
Audio.play osc;;
```

## Audio Nodes

### Oscillators

```ylc
# Sine wave
let sin = Audio.sin_osc frequency;;

# Sawtooth wave
let saw = Audio.saw_osc frequency;;

# Square wave
let square = Audio.square_osc frequency;;

# Triangle wave
let triangle = Audio.tri_osc frequency;;

# Noise generator
let noise = Audio.white_noise;;
```

### Filters

```ylc
# Low-pass filter
let lpf = Audio.lpf cutoff resonance;;

# High-pass filter
let hpf = Audio.hpf cutoff resonance;;

# Band-pass filter
let bpf = Audio.bpf cutoff bandwidth;;

# Example: filtered sawtooth
let saw = Audio.saw_osc 110.0;;
let filtered = Audio.lpf 1000.0 0.7 saw;;
```

### Envelopes

```ylc
# ADSR envelope
let env = Audio.adsr attack decay sustain release;;

# Example: envelope applied to oscillator
let osc = Audio.sin_osc 440.0;;
let shaped = Audio.mul osc env;;
```

### Effects

```ylc
# Delay/Echo
let delay = Audio.delay time feedback;;

# Reverb
let reverb = Audio.reverb size damping;;

# Distortion
let dist = Audio.distortion amount;;

# Chorus
let chorus = Audio.chorus rate depth;;
```

## Node Graph Operations

### Connecting Nodes

```ylc
# Serial connection (chain)
let osc = Audio.sin_osc 440.0;;
let filtered = Audio.lpf 1000.0 0.5 osc;;
let delayed = Audio.delay 0.5 0.3 filtered;;

Audio.connect delayed Audio.output;;
```

### Mixing Nodes

```ylc
# Mix multiple signals
let osc1 = Audio.sin_osc 440.0;;
let osc2 = Audio.sin_osc 554.37;;  # C#5

let mix = Audio.mix [osc1, osc2];;
Audio.connect mix Audio.output;;
```

### Modulation

```ylc
# Frequency modulation
let carrier = Audio.sin_osc 440.0;;
let modulator = Audio.sin_osc 5.0;;  # 5 Hz LFO

let fm = Audio.fm_osc carrier modulator depth;;
```

## Real-Time Updates

### Parameter Control

```ylc
# Create controllable parameter
let freq = Audio.param 440.0;;
let osc = Audio.sin_osc_param freq;;

# Update parameter (RT-safe)
Audio.set_param freq 880.0;;

# Ramp parameter over time
Audio.ramp_param freq 880.0 1.0;;  # Ramp to 880Hz over 1 second
```

### Dynamic Graph Modification

```ylc
# Add node to running graph
let new_osc = Audio.sin_osc 220.0;;
Audio.add_node new_osc;;
Audio.connect new_osc Audio.output;;

# Remove node
Audio.remove_node new_osc;;
```

## Audio Buffers

### Working with Buffers

```ylc
# Create audio buffer
let buffer = Audio.buffer_create 44100;;  # 1 second at 44.1kHz

# Load audio file
let sample = Audio.load_wav "sample.wav";;

# Play buffer
let player = Audio.buffer_player sample;;
Audio.connect player Audio.output;;
```

### Buffer Operations

```ylc
# Loop buffer
let looper = Audio.loop_buffer buffer start_pos end_pos;;

# Granular synthesis
let grains = Audio.granular buffer grain_size density;;
```

## Example: Complete Synthesizer

```ylc
import Audio;

# Initialize
Audio.init 44100 256;;

# Oscillators
let osc1 = Audio.saw_osc 110.0;;  # A2
let osc2 = Audio.saw_osc 220.0;;  # A3 (octave up)

# Mix oscillators
let mix = Audio.mix [
  Audio.gain 0.5 osc1,
  Audio.gain 0.3 osc2
];;

# Filter
let cutoff = Audio.param 1000.0;;
let filtered = Audio.lpf_param cutoff 0.7 mix;;

# Envelope
let env = Audio.adsr 0.01 0.1 0.7 0.5;;
let shaped = Audio.mul filtered env;;

# Effects
let delayed = Audio.delay 0.375 0.3 shaped;;
let reverbed = Audio.reverb 0.5 0.5 delayed;;

# Output
let master = Audio.gain 0.5 reverbed;;
Audio.connect master Audio.output;;

# Control
Audio.start;;

# Modulate filter cutoff
let lfo = Audio.sin_osc 0.5;;  # 0.5 Hz
let mod_cutoff = Audio.map_range lfo 500.0 2000.0;;
Audio.connect_param mod_cutoff cutoff;;
```

## Performance Considerations

### Buffer Sizes

```ylc
# Smaller buffer = lower latency, higher CPU
Audio.init 44100 128;;  # ~3ms latency

# Larger buffer = higher latency, lower CPU
Audio.init 44100 512;;  # ~12ms latency
```

### Sample Rates

```ylc
# Standard sample rates
Audio.init 44100 256;;  # CD quality
Audio.init 48000 256;;  # Professional audio
Audio.init 96000 512;;  # High resolution
```

### CPU Usage

```ylc
# Monitor CPU usage
let usage = Audio.cpu_usage;;
print usage;;  # Percentage of available time

# Node counts
let node_count = Audio.count_nodes;;
print node_count;;
```

## Advanced Features

### Custom DSP Nodes

```ylc
# Define custom audio processing function
let my_processor = fn input_buffer output_buffer size ->
  for i in range 0 size do
    let sample = Array.get input_buffer i in
    let processed = sample * 0.5 in  # Simple gain
    Array.set output_buffer i processed;
  done
;;

# Create node from function
let custom = Audio.custom_node my_processor;;
```

### FFT Analysis

```ylc
# Frequency domain analysis
let analyzer = Audio.fft_analyzer 1024;;  # FFT size

# Get frequency bins
let spectrum = Audio.get_spectrum analyzer;;
```

### Metering

```ylc
# Peak meter
let peak = Audio.peak_meter input;;
let peak_value = Audio.get_peak peak;;

# RMS meter
let rms = Audio.rms_meter input;;
let rms_value = Audio.get_rms rms;;
```

## See Also

- [Examples](examples.md) - Audio code examples
- [API Reference](api-reference.md) - Complete API documentation
- [Language Reference](ylc-ref.md) - YALCE language guide
