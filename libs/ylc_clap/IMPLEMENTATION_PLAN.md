# Implementation Plan: Hot-Reloaded YLC Modules for `ylc_clap`

## Summary

Implement save-triggered, debounced, asynchronous recompilation of `.ylc` scripts

Each plugin instance treats its script as a unique YLC module keyed by:

```text
(plugin_instance_id, canonical_script_path)

When the script watcher detects a save, the plugin schedules a background compile. On successful compile, the global runtime service replaces that instance’s module and atomically swaps the plugin’s active
program at a block boundary. On failure, the previous working module remains active and errors are written to the plugin UI debug log.

## Key Changes

### YLC JIT Integration

Extract a reusable JIT/runtime API from the existing YLC LLVM ORC path instead of calling the CLI-oriented orcjit() entrypoint.

The new API should:

- own the LLVM ORC session/state
- compile real .ylc source using the existing parser, type inference, MIR, and LLVM lowering pipeline
- support replacing a named module for a specific plugin instance
- expose resolved function handles for known callback names
- report parse/type/codegen/JIT errors as strings suitable for the plugin UI

The plugin should not treat scripts as C, LLVM IR, or generated shared libraries. It should compile actual YLC source through the YLC compiler pipeline.

### Runtime Service

Extend the process-global runtime_service so the .so owns one shared runtime service across all plugin instances.

The service should own:

- one shared YLC JIT/compiler session
- a compile worker thread
- a compile queue
- a module registry keyed by (instance_id, canonical_script_path)
- retired program/module objects awaiting safe reclamation
- shared logging hooks into each plugin instance’s UI debug stream

This keeps multiple plugin instances in the same REAPER process using the same language runtime while still giving each instance its own script module identity.

### File Watch and Reload Flow

Use the existing file watcher only as a reload trigger.

Flow:

1. inotify detects a script file change
2. plugin marks script_reload_pending
3. main/UI thread debounces the change
4. runtime service enqueues a compile job
5. worker thread reads and compiles the script
6. successful compile produces an immutable prepared program object
7. plugin swaps the active program pointer at a block boundary
8. old program is retired after the audio thread can no longer be using it

Do not compile, allocate heavily, read files, call LLVM, or mutate runtime state directly from the audio thread.

### Failure Behavior

On compile failure:

- keep the previous working program active
- do not replace it with bypass
- log the compiler error in the plugin UI debug panel
- leave script_reload_pending cleared after the failed attempt
- allow the next file save to retry compilation

This makes syntax/type errors non-destructive during live editing.

## Callback Model

Scripts may optionally export these hook names:

on_process
on_note_on
on_note_off
on_param
on_midi
on_transport

Missing hooks are no-ops.

Use a YLC object API at the script level. Do not expose raw CLAP structs directly to YLC scripts.

The C/CLAP shim should provide stable handles or wrapper objects for:

- audio buffers
- frame count
- sample offset
- the 32 generic parameters
- note events
- MIDI events
- transport data
- logging/debug output

on_process should receive a process/audio object that can read inputs, write outputs, inspect parameter values, and access current transport state.

Event hooks should preserve CLAP sample timing via event->header.time.

## Audio Thread Rules

clap_plugin.process() must stay real-time safe.

Allowed in process():

- read the current atomic ylc_program_t *
- dispatch already-prepared event data
- call already-resolved function pointers
- process audio spans between sample-sorted events
- read immutable or preallocated state

Forbidden in process():

- JIT compilation
- file I/O
- heap allocation
- locks
- logging with blocking I/O
- dlopen
- mprotect
- unbounded script/runtime work
- destroying old compiled code

Program replacement should happen with an atomic pointer swap at a block boundary. Old programs should be reclaimed with deferred destruction so the audio thread cannot call freed code.

## Parameter and Event Handling

Keep the existing 32 fixed generic CLAP parameters:

Param 01 .. Param 32
range: 0.0 .. 1.0
flags: automatable, modulatable

Do not dynamically add/remove CLAP parameters for v1.

Parameter value and modulation events from the host should be forwarded to the script callback layer with their sample offsets intact.

Incoming CLAP event handlers should initially log and dispatch:

- note on
- note off
- note choke
- note expression
- parameter value
- parameter modulation
- parameter gesture begin/end
- transport
- MIDI
- SysEx
- MIDI 2.0
- unknown events

Outgoing CLAP events are out of scope for the first JIT reload step unless already supported by the shim.

## State Model

Each successful script reload creates fresh script/module state.

Do not attempt to preserve script globals or runtime objects across recompiles in v1.

The CLAP plugin state should continue saving:

- script path
- generic parameter values

Compiled modules themselves should not be serialized into the REAPER project.

On project load, the plugin restores the path and schedules/permits compilation from the saved source path.

## Testing

### Build Tests

Run:

make -C /home/adam/projects/yalce
make -C /home/adam/projects/yalce/libs/ylc_clap

### Runtime Service Tests

Add focused tests or test harness coverage for:

- enqueueing a compile job
- debounce behavior for repeated save events
- successful module replacement
- failed compile keeping the old module active
- two plugin instances using the same script path getting separate modules
- destroying a plugin instance while a compile job is pending
- removing a track in REAPER without crashing

### REAPER Smoke Tests

Verify manually in REAPER:

- plugin loads
- script path can be edited and saved with the project
- project reload restores the script path
- saving the script triggers a compile attempt
- compile success appears in the UI debug panel
- syntax/type errors appear in the UI debug panel
- old program remains active after failed compile
- parameter automation/modulation still reaches callbacks with sample timing
- deleting a track containing the plugin does not crash REAPER
- rescanning plugins does not need to reload existing active instances

## Assumptions

- Active plugin project path is /home/adam/projects/yalce/libs/ylc_clap.
- YLC source files are compiled through the existing YLC compiler pipeline.
- The JIT backend is LLVM ORC.
- Reload is debounced and asynchronous.
- Compile failure keeps the old module active.
- Callback hooks are optional.
- Script state is fresh after every successful reload.
- Module identity is (plugin_instance_id, canonical_script_path).
- The 32 generic CLAP params remain fixed for now.
- REAPER-specific APIs remain optional; the plugin should still work in non-REAPER CLAP hosts.
