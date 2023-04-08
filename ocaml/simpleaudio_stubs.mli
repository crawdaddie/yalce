external start_audio : unit -> unit = "caml_start_audio"
external oscilloscope : unit -> unit = "caml_oscilloscope"
external kill_audio : unit -> unit = "caml_kill_audio"
external add_node : unit -> unit = "caml_add_node"
external chain_nodes : unit -> unit = "caml_chain_nodes"
type node_ptr
external play_sin : float -> node_ptr = "caml_play_sin"
module StringMap :
  sig
    type key = String.t
    type 'a t = 'a Map.Make(String).t
    val empty : 'a t
    val is_empty : 'a t -> bool
    val mem : key -> 'a t -> bool
    val add : key -> 'a -> 'a t -> 'a t
    val update : key -> ('a option -> 'a option) -> 'a t -> 'a t
    val singleton : key -> 'a -> 'a t
    val remove : key -> 'a t -> 'a t
    val merge :
      (key -> 'a option -> 'b option -> 'c option) -> 'a t -> 'b t -> 'c t
    val union : (key -> 'a -> 'a -> 'a option) -> 'a t -> 'a t -> 'a t
    val compare : ('a -> 'a -> int) -> 'a t -> 'a t -> int
    val equal : ('a -> 'a -> bool) -> 'a t -> 'a t -> bool
    val iter : (key -> 'a -> unit) -> 'a t -> unit
    val fold : (key -> 'a -> 'b -> 'b) -> 'a t -> 'b -> 'b
    val for_all : (key -> 'a -> bool) -> 'a t -> bool
    val exists : (key -> 'a -> bool) -> 'a t -> bool
    val filter : (key -> 'a -> bool) -> 'a t -> 'a t
    val filter_map : (key -> 'a -> 'b option) -> 'a t -> 'b t
    val partition : (key -> 'a -> bool) -> 'a t -> 'a t * 'a t
    val cardinal : 'a t -> int
    val bindings : 'a t -> (key * 'a) list
    val min_binding : 'a t -> key * 'a
    val min_binding_opt : 'a t -> (key * 'a) option
    val max_binding : 'a t -> key * 'a
    val max_binding_opt : 'a t -> (key * 'a) option
    val choose : 'a t -> key * 'a
    val choose_opt : 'a t -> (key * 'a) option
    val split : key -> 'a t -> 'a t * 'a option * 'a t
    val find : key -> 'a t -> 'a
    val find_opt : key -> 'a t -> 'a option
    val find_first : (key -> bool) -> 'a t -> key * 'a
    val find_first_opt : (key -> bool) -> 'a t -> (key * 'a) option
    val find_last : (key -> bool) -> 'a t -> key * 'a
    val find_last_opt : (key -> bool) -> 'a t -> (key * 'a) option
    val map : ('a -> 'b) -> 'a t -> 'b t
    val mapi : (key -> 'a -> 'b) -> 'a t -> 'b t
    val to_seq : 'a t -> (key * 'a) Seq.t
    val to_rev_seq : 'a t -> (key * 'a) Seq.t
    val to_seq_from : key -> 'a t -> (key * 'a) Seq.t
    val add_seq : (key * 'a) Seq.t -> 'a t -> 'a t
    val of_seq : (key * 'a) Seq.t -> 'a t
  end
type param_map = int StringMap.t
type node = { param_map : param_map; node_ptr : node_ptr; }
val set_param : node -> StringMap.key -> int -> node
val sin : float -> node
val node_param_idx : node -> StringMap.key -> int
external set_sig : node_ptr -> int -> float -> unit = "caml_set_sig"
val node_set : node -> StringMap.key -> float -> unit
external set_freq : node_ptr -> float -> unit = "caml_set_freq"
external play_sq : float -> node_ptr = "caml_play_sq"
external play_sq_detune : float -> node_ptr = "caml_play_sq_detune"
external play_imp : float -> node_ptr = "caml_play_impulse"
external play_saw : float -> node_ptr = "caml_play_poly_saw"
external play_hoover : float -> node_ptr = "caml_play_hoover"
external play_pulse : float -> float -> node_ptr = "caml_play_pulse"
external pulse : float -> float -> node_ptr = "caml_play_blip"
external stop : node_ptr -> unit = "caml_kill_node"
external dump_nodes : unit -> unit = "caml_dump_nodes"
