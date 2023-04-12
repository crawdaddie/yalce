external start_audio: unit -> unit = "caml_start_audio"
external oscilloscope: unit -> unit = "caml_oscilloscope"

external kill_audio: unit -> unit = "caml_kill_audio" 


external add_node: unit -> unit = "caml_add_node" 

type node_ptr

external osc_sin: float -> node_ptr = "caml_sin"
module StringMap = Map.Make(String)
type param_map = int StringMap.t

let make_param_map keys =
  let rec aux counter acc = function
    | [] -> acc
    | hd :: tl ->
        let acc' = StringMap.add hd counter acc in
        aux (counter + 1) acc' tl
  in
  aux 0 StringMap.empty keys
type node = { param_map: param_map; node_ptr: node_ptr }

type node_param = 
  | Float of float
  | Node of node

let set_param node key value =
  let updated_param_map = StringMap.add key value node.param_map in
  { node with param_map = updated_param_map }

let sin x =
  let node_ptr = osc_sin x in
  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

let node_param_idx node param_name =
  StringMap.find param_name node.param_map


external set_sig_float: node_ptr -> int -> float -> unit = "caml_set_sig_float"
external set_sig_node: node_ptr -> int -> node_ptr -> unit = "caml_set_sig_node"
external caml_set_mul_float: node_ptr -> float -> unit = "caml_set_mul" 
external caml_set_mul_node: node_ptr -> node_ptr -> unit = "caml_set_mul_node" 
external caml_set_add_float: node_ptr -> float -> unit = "caml_set_add" 
external caml_set_add_node: node_ptr -> node_ptr -> unit = "caml_set_add_node" 
let set_float node param_name f = 
  (match param_name with
  | "mul" -> caml_set_mul_float node.node_ptr f
  | "add" -> caml_set_add_float node.node_ptr f
  | _ -> 
    let param_idx = node_param_idx node param_name in 
    set_sig_float node.node_ptr param_idx f);
  node



let set_node node param_name n = 
  (match param_name with
  | "mul" -> caml_set_mul_node node.node_ptr n.node_ptr
  | "add" -> caml_set_add_node node.node_ptr n.node_ptr
  | _ -> 
    let param_idx = node_param_idx node param_name in 
    set_sig_node node.node_ptr param_idx n.node_ptr);
  node


let set node param_name value = 
  match value with
  | Float f -> set_float node param_name f
  | Node n -> set_node node param_name n
  



external set_freq: node_ptr -> float -> unit = "caml_set_freq"
external osc_sq: float -> node_ptr = "caml_sq"

let sq x =
  let node_ptr = osc_sq x in

  let param_map = make_param_map ["freq"; "pw"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external osc_sq_detune: float -> node_ptr = "caml_sq_detune"

let sq_detune x =
  let node_ptr = osc_sq_detune x in
  let param_map = (
    StringMap.empty
    |> StringMap.add "freq" 0 
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external osc_impulse: float -> node_ptr = "caml_impulse"

let impulse x =
  let node_ptr = osc_impulse x in

  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external osc_saw: float -> node_ptr = "caml_poly_saw"

let saw x =
  let node_ptr = osc_saw x in
  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external osc_hoover: float -> node_ptr = "caml_hoover"
external osc_pulse: float -> float -> node_ptr = "caml_pulse"

let pulse f pw =
  let node_ptr = osc_pulse f pw in

  let param_map = make_param_map ["freq"; "pw"] in

  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }


external fx_delay: float -> float -> node_ptr = "caml_simple_delay"
let delay t f =
  let node_ptr = fx_delay t f in 

  let param_map = make_param_map ["time"; "fb"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external caml_chain_nodes: node_ptr -> node_ptr -> int -> node_ptr = "caml_chain_nodes" 

external stop: node_ptr -> unit = "caml_kill_node"
external dump_nodes: unit -> unit = "caml_dump_nodes"

let make_node node_ptr param_map =
  {param_map = param_map; node_ptr = node_ptr;}

let chain_nodes a b ?(p=0) =
  let node_ptr = caml_chain_nodes a.node_ptr b.node_ptr p in
  (
    make_node node_ptr a.param_map,
    make_node b.node_ptr b.param_map
  )
;;

external play_node: node_ptr -> node_ptr = "caml_play_node"
let play node = 
  play_node node.node_ptr

let chain a b p =
  let node_ptr = caml_chain_nodes a.node_ptr b.node_ptr (node_param_idx b p) in
  ({
    param_map = a.param_map;
    node_ptr = node_ptr; 
  }, 
  {
    param_map = b.param_map;
    node_ptr = b.node_ptr; 
  })
;;


(* Define the chaining operator => *)
let (=>) a b = chain_nodes a b ~p:0
let (=>.) a p b = chain_nodes a b ~p:(node_param_idx b p)

(* external extern_set_list_float: node_ptr -> unit = "caml_set_list_float" *)

(* let set_list node mapping_list = *)
(*   let f el = *)
(*     let (p, v) = el in *)
(*     (node_param_idx node p, v); *)
(*  *)
(*   extern_set_list_float node.node_ptr (List.map f mapping_list) *)



