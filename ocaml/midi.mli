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
}
val handle_msg : int32 -> (int32 -> int32 -> int32 -> unit) -> unit
val input_cb : Portmidi.Input_stream.t -> unit
val create : unit -> t
val register : t -> (int -> int -> int -> unit) -> unit
