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
type node = { param_map : param_map; node_ptr : Stubs.node_ptr; }
type node_param = Float of float | Node of node | Signal of Stubs.signal
val make_param_map : StringMap.key list -> int StringMap.t
val play : node -> Stubs.node_ptr
val set_param : node -> StringMap.key -> int -> node
val node_param_idx : node -> StringMap.key -> int
val set_float : node -> StringMap.key -> float -> node
val set_node : node -> StringMap.key -> node -> node
val set_signal : node -> StringMap.key -> node -> node
val set : StringMap.key -> node_param -> node -> node
val make_node : Stubs.node_ptr -> param_map -> node
val chain_nodes : node -> node -> ?p:int -> node * node
val chain_nodeptrs_idx :
  Stubs.node_ptr -> ?p:int -> Stubs.node_ptr -> Stubs.node_ptr
val chain_nodes_idx : node -> ?p:int -> node -> node
val chain : node -> node -> StringMap.key -> node * node
val ( => ) : node -> node -> node * node
val ( =>. ) : Stubs.node_ptr -> int -> Stubs.node_ptr -> Stubs.node_ptr
external extern_set_list_float :
  Stubs.node_ptr -> int -> float list -> int list -> Stubs.node_ptr
  = "caml_set_list_float"
type arg_pair = string * float
val set_list : arg_pair list -> node -> node
