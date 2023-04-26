type t = {
  mutex : Mutex.t;
  handlers : (int32 -> int32 -> int32 -> unit) list ref;
  dbg : bool ref;
  name : string;
}
val log_midi_error :
  (string -> string, unit, string) format ->
  Portmidi.Portmidi_error.t -> unit
val log_midi_msg : int32 -> string -> unit
val get_device_name : int -> string
val create : int -> t
val register : t -> (int32 -> int32 -> int32 -> unit) -> unit
val connect_all_sources : unit -> t list
