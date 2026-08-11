# YLC CLAP

A small CLAP audio/MIDI plugin shim for experimenting with a script/JIT runtime behind REAPER's native CLAP support on Linux.

The scaffold keeps the host-facing API in the CLAP plugin and routes audio/event work through a prepared runtime program:

```text
REAPER host
  -> CLAP plugin shim
      -> script/JIT runtime
          -> on_process(audio, frames)
          -> on_note_on/off(...)
          -> on_param(...)
          -> on_midi(...)
          -> on_transport(...)
```

The current runtime is a bypass/gain placeholder. It is structured so a future compiler can build a `ylc_program_t` on a non-audio thread, then atomically publish it for the next process block.

The plugin also has a process-global runtime service owned by the loaded `.clap` shared object. `clap_entry.init()` initializes it, each plugin instance acquires a unique instance id and increments the service refcount, and `destroy()` releases it. This is the intended home for a shared language VM/compiler/module cache later, while each plugin instance keeps its own script path, state, debug stream, and prepared program pointer.

## Build

Fetch/update vendored SDK headers:

```sh
make deps
```

```sh
make
```

The plugin binary is:

```text
build/ylc_script.clap
```

## Install Locally

```sh
make install
```

Then rescan CLAP plugins in REAPER.

## REAPER UI

Open the plugin's normal FX UI in REAPER. The plugin exposes a small X11 CLAP GUI with:

- a script path text field
- an `Open in nvim` button

The script path defaults to:

```text
$HOME/.config/ylc_clap/script.ylc
```

Scripts should explicitly open the CLAP DSP bindings they use:

```ylc
open libs/ylc_clap/DSP;

let tone = @Audio fn () ->
  sin_osc 220.
;;

tone () |> play_node;
```

You can override startup defaults before launching REAPER:

```sh
export YLC_SCRIPT_PATH="$HOME/src/my-script.ylc"
export YLC_TERMINAL=kitty
export YLC_EDITOR=nvim
reaper
```

CLAP parameters are numeric values, so the script path is intentionally not exposed as a fake parameter. The plugin exposes 32 fixed generic parameters named `Param 01` through `Param 32`, each in the range `0.0..1.0` and marked automatable/modulatable. `Param 01` currently drives the placeholder passthrough gain so the scaffold still has an audible control.

The script path is saved through CLAP plugin state, so REAPER should restore the per-instance path when the project is reopened. `YLC_SCRIPT_PATH` is only used as the initial default for new plugin instances.

The plugin also registers a Linux `inotify` watcher for the selected script path's parent directory. When the selected file is saved or replaced, the plugin sets an internal `script_reload_pending` flag and the UI status changes to `Script changed: reload pending`. No recompilation happens yet; that flag is the intended handoff point for a later main/background-thread compile step.

The script's parent directory must already exist for the watcher to attach.

Debug output is routed through an internal nonblocking pipe, not stdout/stderr. Plugin code can write newline-delimited messages to the plugin's `debug_stream` with `fprintf`; the main/UI thread drains the pipe and renders the most recent lines in the plugin UI's debug panel. Do not write debug output from the audio thread.

## Validate

If `clap-validator` is installed:

```sh
clap-validator validate build/ylc_script.clap
```

## Real-Time Boundary

Keep these operations outside `process()`:

- JIT compilation
- heap allocation
- file I/O
- `dlopen`
- `mprotect`
- logging
- locks
- unbounded script execution

Prepare script state before activation or on a background/main thread, then publish the new `ylc_program_t *` with an atomic pointer swap at a block boundary.
