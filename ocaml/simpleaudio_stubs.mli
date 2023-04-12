external start_audio : unit -> unit = "caml_start_audio"
external oscilloscope : unit -> unit = "caml_oscilloscope"
external kill_audio : unit -> unit = "caml_kill_audio"
external add_node : unit -> unit = "caml_add_node"
type node_ptr
external osc_sin : float -> node_ptr = "caml_sin"
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
val make_param_map : StringMap.key list -> int StringMap.t
type node = { param_map : param_map; node_ptr : node_ptr; }
type node_param = Float of float | Node of node
val set_param : node -> StringMap.key -> int -> node
val sin : float -> node
val node_param_idx : node -> StringMap.key -> int
external set_sig_float : node_ptr -> int -> float -> unit
  = "caml_set_sig_float"
external set_sig_node : node_ptr -> int -> node_ptr -> unit
  = "caml_set_sig_node"
external caml_set_mul_float : node_ptr -> float -> unit = "caml_set_mul"
external caml_set_mul_node : node_ptr -> node_ptr -> unit
  = "caml_set_mul_node"
external caml_set_add_float : node_ptr -> float -> unit = "caml_set_add"
external caml_set_add_node : node_ptr -> node_ptr -> unit
  = "caml_set_add_node"
val set_float : node -> StringMap.key -> float -> node
val set_node : node -> StringMap.key -> node -> node
val set : node -> StringMap.key -> node_param -> node
external set_freq : node_ptr -> float -> unit = "caml_set_freq"
external osc_sq : float -> node_ptr = "caml_sq"
val sq : float -> node
external osc_sq_detune : float -> node_ptr = "caml_sq_detune"
val sq_detune : float -> node
external osc_impulse : float -> node_ptr = "caml_impulse"
val impulse : float -> node
external osc_saw : float -> node_ptr = "caml_poly_saw"
val saw : float -> node
external osc_hoover : float -> node_ptr = "caml_hoover"
external osc_pulse : float -> float -> node_ptr = "caml_pulse"
val pulse : float -> float -> node
external fx_delay : float -> float -> node_ptr = "caml_simple_delay"
val delay : float -> float -> node
external caml_chain_nodes : node_ptr -> node_ptr -> int -> node_ptr
  = "caml_chain_nodes"
external stop : node_ptr -> unit = "caml_kill_node"
external dump_nodes : unit -> unit = "caml_dump_nodes"
val make_node : node_ptr -> param_map -> node
val chain_nodes : node -> node -> ?p:int -> node * node
external play_node : node_ptr -> node_ptr = "caml_play_node"
val play : node -> node_ptr
val chain : node -> node -> StringMap.key -> node * node
val ( => ) : node -> node -> node * node
val ( =>. ) : node -> StringMap.key -> node -> node * node
