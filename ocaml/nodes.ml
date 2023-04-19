open Stubs;;

module StringMap = Map.Make(String)
type param_map = int StringMap.t

type node = { param_map: param_map; node_ptr: node_ptr }

type node_param = 
  | Float of float
  | Node of node

let make_param_map keys =
  let rec aux counter acc = function
    | [] -> acc
    | hd :: tl ->
        let acc' = StringMap.add hd counter acc in
        aux (counter + 1) acc' tl
  in
  aux 0 StringMap.empty keys

let play node = 
  play_node node.node_ptr

let set_param node key value =
  let updated_param_map = StringMap.add key value node.param_map in
  { node with param_map = updated_param_map }


let node_param_idx node param_name = match param_name with
  | "mul" -> -1
  | "add" -> -2
  | _ -> StringMap.find param_name node.param_map


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


let set param_name value node = 
  match value with
  | Float f -> set_float node param_name f
  | Node n -> set_node node param_name n

let make_node node_ptr param_map =
  {param_map = param_map; node_ptr = node_ptr;}

let chain_nodes a b ?(p=0) =
  let node_ptr = caml_chain_nodes a.node_ptr b.node_ptr p in
  (
    make_node node_ptr a.param_map,
    make_node b.node_ptr b.param_map
  )
;;


let chain_nodes_idx a ?(p=0) b =
  caml_chain_nodes a.node_ptr b.node_ptr p |> ignore;
  b
;;

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


(* Define the chaining operator => *)
let (=>) a b = chain_nodes a b ~p:0
let (=>.) a p = (chain_nodes_idx a ~p:(p))

(* external extern_set_list_float: node_ptr -> unit = "caml_set_list_float" *)

(* let set_list node mapping_list = *)
(*   let f el = *)
(*     let (p, v) = el in *)
(*     (node_param_idx node p, v); *)
(*  *)
external extern_set_list_float: node_ptr -> int -> float list -> int list -> node_ptr = "caml_set_list_float"

type arg_pair = string * float

let set_list (param_list: arg_pair list) node =  
  let keys, vals = List.split param_list in 
  let indices = List.map (node_param_idx node) keys in 
  let node_ptr = extern_set_list_float node.node_ptr (List.length param_list) vals indices in
  { param_map = node.param_map; node_ptr = node_ptr }



