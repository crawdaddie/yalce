# Buffer references in synth constructors

Status: proposal. Source baseline: `bb2d916b`. No implementation changes.

## Motivation

`markov_perc.ylc` repeats sample-player bodies because each synth captures a
particular buffer. One compiled player should accept different buffers at
construction, while each playing instance keeps its own playback state:

```ocaml
kick kick_buf |> play_node;
kick snare_buf |> play_node;
```

Both calls use the same compiled synth functions. They allocate separate nodes
and playheads, sharing sample memory with the buffer registry. Choosing a buffer
must not compile another synth or copy its samples.

## Proposed interface

Introduce opaque `SF.BufferRef`, represented by a pointer at runtime, and
`SF.load_ref : String -> Option of SF.BufferRef`. A successful load registers an
immutable buffer and returns its reference. Failed loads return `None`; they do
not produce a playable empty buffer.

Proposed syntax, not currently implemented:

```ocaml
let kick = @Audio fn buf_ref ->
  let t = trig 0;
  SF.play buf_ref 1. 0. t
  |> (*) (t |> rect 0.22 |> adsr 0. 0. 1. 0.01 |> kill_on_end)
  |> (*) (Math.db_amp -6.)
  |> pan 2 0.5
;;

let hit = fn idx ->
  kick (bufs[idx]) |> play_node;
  0.25
;;
```

Here `bufs` is an array of successfully loaded references. Loading and handling
failures happens before scheduling the pattern. Each Markov event selects a
reference and constructs a voice; matrix selection and timing stay unchanged.

`SF.play buffer rate start trigger` is an audio builtin. Rate is a playback
ratio, with `1.` preserving the source rate. Start is a normalized position,
matching the existing player convention. The builtin applies the buffer's sample
rate and the engine's seconds-per-frame value. Reuse the existing playhead and
trigger behavior where compatible.

The first version returns mono: mono files pass through; stereo files are
averaged. Restrict accepted files to these channel layouts initially. This gives
one compiled output layout for mixed mono/stereo sample collections. A later
explicit stereo player can return a fixed stereo layout; runtime metadata must
not change a synth's compiled channel count.

Keep existing array-based `bufplay`, `mbufplay`, `SF.load_sf` and legacy integer
buffer APIs working. A distinct builtin avoids changing array playback or
creating reference-counted array views inside the sample loop.

## Existing implementation

| Component | Current behavior | Required change |
| --- | --- | --- |
| `engine/engine_core.c`, `load_sf_buffer` | Allocates `SfBuffer`, assigns an integer ID and links it into a registry | Reuse ownership and identity; expose a typed reference load API |
| `sf_buffer` | Traverses the registry by ID | Resolve legacy IDs outside the sample loop |
| `sf_buffer_data` | Returns an array and increments its reference count | Keep this host API; use borrowed audio access for playback |
| `audio_mir_collect_ctor_params` | Preserves inferred argument types | Classify resource arguments separately from signals |
| `audio_mir_value_as_node` | Treats `Ptr` as an existing node; converts other values into scalar nodes | Never send a buffer reference through this function |
| `audio_mir_emit_constructor` | Connects every argument as an audio input | Store resources and connect only signals |
| `audio_mir_emit_frame_adapter` | Reads every input as sample data | Load resources from instance storage with their actual type |
| `audio_mir_apply_audio_synth` | Passes nested arguments into kernels with lane conversion | Preserve references and broadcast them without numeric conversion |
| Voice-array allocator | Creates default scalar inputs for every argument | Require resources to be bound before allocation |

These compiler functions are in `libs/audio_jit/audio_jit.c`. The engine frame
ABI is declared in `engine/node.h`. The bundle constant-folding pass is currently
a stub; this design must work without cross-function constant propagation.

## Reference representation and identity

Keep three concepts separate:

- **Buffer ID:** stable registry identity, useful for lookup and diagnostics.
- **Buffer reference:** opaque pointer to an immutable engine-owned descriptor.
- **Sample pointer:** descriptor-owned sample storage, borrowed during playback.

The existing registry allocates descriptors separately. Adding another entry
does not move previous descriptors or sample storage. Successfully registered
buffers currently have no removal path. Preserve that stability across synth
instances and JIT modules using the same engine registry.

`BufferRef` must remain distinct from both `Double` and generic `Ptr` through
inference and MIR argument classification. Give it a compiler-recognized opaque
representation, lowered to LLVM `ptr`. A plain alias to `Ptr` is insufficient:
existing audio code interprets `Ptr` as a node. Register the representation in
type equality, layout, serialization and FFI lowering as needed; do not recognize
it by an unqualified type-name string alone.

Use the target data layout for size and alignment. A pointer occupies 64 bits
on the current target, but numeric conversion to `Double` does not preserve
arbitrary 64-bit values. Neither floating-point casts nor pointer bit patterns
belong in signal samples. Existing small integer IDs may remain compatible with
legacy APIs, but the new path never transports references as doubles.

Do not embed process addresses in serialized MIR or cached object files. MIR
contains a typed value or a runtime load. The registry ID and reference become
available when the sample is loaded; compilation need not execute the loader.

## Parameter metadata

Add one parameter plan shared by the synth symbol and its bundle builders.
Separate source arity from engine input count; `num_inputs` currently serves both
roles in several paths.

Each entry records:

| Field | Purpose |
| --- | --- |
| Source index and type | Preserve application order and diagnostics |
| Kind enum | `SIGNAL` or `BUFFER_REF` |
| Kernel argument index | Bind the body to its typed MIR parameter |
| Signal input index | Dense graph-input index, only for signals |
| Resource state offset | Pointer-aligned storage, only for references |

For `player buffer rate gain`, source arity is three and signal input count is
two. Buffer arguments do not consume `MAX_INPUTS` capacity. Preserve existing
numeric-only input numbering. For mixed signatures, input setters address the
dense signal indices: rate is input 0, gain is input 1. Document this explicitly;
a buffer reference cannot be changed through `set_input_scalar`.

Compute this plan once and use it for exported constructors, dynamic node
construction, nested applications, partial applications and voice arrays.
Reject unsupported aggregate resource parameters with a type error initially;
do not silently cast them to numbers.

## MIR bundle and constructor

The following is illustrative MIR, not parser syntax. Keep buffer parameters in
the kernel signature so nested calls can forward them directly:

```text
cons(buffer: BufferRef, rate: Double) -> NodePtr
init(state: Ptr) -> Void
kernel(node: Ptr, state: Ptr, frame: Int, spf: Double,
       buffer: BufferRef, rate: Double) -> Double
frame(node: Ptr, state: Ptr, inputs: Ptr, frame: Int, spf: Double) -> Void
```

The signatures above show scalar construction. Preserve the existing mechanism
that permits audio nodes as signal arguments; the parameter plan must not remove
signal modulation or stereo signal inputs.

Constructor lowering:

```text
node = create_node(frame_fn, signal_count, output_lanes, state_bytes)
state = node_state(node)
initialize_resource_storage(state)
store BufferRef buffer at state + buffer_offset
call init(state)
rate_node = value_as_node(rate)
connect_input(node, rate_input_index, rate_node)
return node
```

Use existing MIR pointer-offset, store, load and call operations. A new general
MIR instruction is unnecessary unless the opaque type cannot use those
operations. Propagate representation metadata through MIR-to-LLVM lowering;
never use a primitive numeric cast to repair a type mismatch.

The returned value remains the ordinary node accepted by `play_node`.
Reference storage happens before publishing the node to the audio scheduler.

### Frame adapter

```text
buffer = load BufferRef from state + buffer_offset
rate = read_signal(inputs, rate_input_index, frame)
sample = call kernel(node, state, frame, spf, buffer, rate)
write_output(node, frame, sample)
```

There is no buffer signal node, floating-point conversion or ID lookup here.
One typed load per reference is sufficient. LLVM may optimize invariant loads,
but correctness must not depend on that optimization.

### Reset and initialization

Separate resource slots from mutable DSP state in the layout. The constructor
initializes resource slots once. `init(state)` resets only DSP storage and
preserves resources, including when called later through `Node.state_init`.

The existing `audio_mir_emit_init` zeros each registered state slot. Exclude
resource slots from those stores and from zeroed nested regions. Represent the
DSP reset region explicitly or mark slots by initialization policy. Apply the same rule recursively to nested
state blocks. Use target alignment when placing pointer slots and DSP regions.

## Buffer playback lowering

`SF.play` receives a typed reference plus numeric controls. Its audio emitter
forwards the reference to a dedicated precompiled playback kernel. The engine
buffer layer exposes an internal borrowed view containing data, frame count,
channel count and sample rate. Compiler code does not reproduce `SfBuffer`
layout or traverse the registry.

The kernel reads metadata without allocation, locking or reference-count
updates. The registry owns both descriptor and samples for the entire call.
Rate becomes `source_samplerate * spf * rate`. Playback reads interleaved samples
using frame and channel counts, never treating stereo samples as a longer mono
recording. Mono folding is part of this kernel's fixed output contract.

Expose only the internal engine interface needed by the audio runtime; keep
registry structures private. Source-level metadata accessors can be added as
borrowed audio builtins if needed, but are not required for the reusable player.

## Nested synths and partial application

A nested call passes `BufferRef` directly to the child's kernel and allocates
only its normal DSP state. It does not need a duplicate resource slot when the
parent already supplies the typed reference each frame. Exclude resources from
lane-count calculation; broadcast the same reference across expanded signal
lanes, with a separate playhead per lane.

Local synth captures use the same type classification. A captured buffer keeps
its reference type rather than becoming a lane or scalar signal.

Partial application records the reference with its type and source position.
It does not create a scalar node. Source arity checks include resources even
though graph input counts exclude them.

Voice arrays cannot invent a default buffer. Require all reference arguments to
be bound in their voice template, then copy those references into each voice's
resource storage. Until that path is implemented, reject resource-bearing voice
arrays explicitly. Never create null reference slots and proceed with playback.

Buffer selection happens at voice construction. Switching a reference from an
audio-rate condition is outside this first version; reject it where it would
violate the constructor-resource contract.

## Lifetime and concurrency

Registry ownership lasts until engine shutdown. Dropping a language reference
or finishing one voice does not release sample memory. Destroy/reset operations
must stop audio processing and retire voices before releasing registry entries.
Loading the same path twice may create two IDs; deduplication is separate work.

Load buffers on the control thread. Fully initialize descriptors before publishing
references or scheduling voices. Readers use immutable descriptors and never
traverse the mutable registry. Serialize registry mutation and legacy lookup on
the control side. If multiple loaders are permitted later, add explicit
synchronization there rather than locks in the playback kernel.

References are valid within one engine registry and process lifetime. They are
not portable across process restarts or independent engine instances. Future
unloading requires an ownership or generation scheme; it cannot be added by
freeing a registered pointer while voices may still hold it.

## Implementation sequence

1. Add the opaque type and checked reference loader using the existing registry.
   Test its representation, type distinction and lifetime first.
2. Add shared parameter metadata and separate arity from signal-input count.
   Preserve numeric-only synth behavior.
3. Implement constructor storage, typed frame loads and reset-safe state layout.
   Cover both normal and alternate node construction paths.
4. Add borrowed buffer playback and its audio emitter. Keep old array playback
   unchanged.
5. Support nested forwarding, captures and partial application. Implement bound
   resource voice arrays or emit the explicit unsupported-case diagnostic.
6. Replace repeated players in `markov_perc.ylc` with one reusable player and
   per-slot reference/settings selection. Preserve the user's current hit routing,
   timing and matrix edits.

## Validation and acceptance

Use headless tests with tiny generated buffers containing different, known
sample values. Audio-device availability must not affect these checks.

- Construct two simultaneous voices from one synth with different references;
  assert distinct expected outputs and independent playheads.
- Construct two voices sharing a reference; assert no sample copy and independent
  playback state. Inspect bundle generation to confirm no compilation per buffer.
- Load another buffer after compiling the synth; pass it to the existing
  constructor and render correctly.
- Load many buffers and verify earlier IDs, references and data remain valid.
- Drop host references and finish other voices; remaining voices still render.
- Check mono/stereo folding, sample-rate conversion, start position and retrigger
  behavior against existing playback semantics.
- Run MIR/LLVM verification for mixed parameters. Inspect IR for pointer
  loads/stores, correct kernel signatures and signal indices. Assert no buffer
  passes through `const_sig`, signal reads or floating-point casts.
- Verify per-frame execution performs no registry lookup, allocation or RC update.
- Reset a voice and confirm its buffer survives while its playhead resets.
- Exercise nested calls, captured references, partial application, stereo lane
  expansion and the supported voice-array path.
- Reject generic pointers, numbers and audio signals where `BufferRef` is required;
  reject failed loads before scheduling. Keep existing numeric-only synth tests.

Acceptance: `kick buf_ref |> play_node` works unchanged at the call site, one
compiled player serves the Markov sample collection, and the engine owns stable
sample storage independently of every voice.
