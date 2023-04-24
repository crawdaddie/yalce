type event =
    [ `Controller of int * float
    | `Nop
    | `Note_off of int
    | `Note_on of int * float
    | `Pitch_bend of int * float
    | `Program_change of int * int ]
type t = {
  mutex : Mutex.t;
  handlers : (int -> int -> int -> unit) list ref;
  dbg : bool ref;
}
val log_midi_error :
  (string -> string, unit, string) format ->
  Portmidi.Portmidi_error.t -> unit
val log_midi_msg : int32 -> int -> unit
val create : int -> t
val register : t -> (int -> int -> int -> unit) -> unit
