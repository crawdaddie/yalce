open Ctypes
open Foreign

let set_node_scalar =
  foreign "set_node_scalar" (Node.node @-> int @-> double @-> returning void)
;;

let set_node_trig = foreign "set_node_trig" (Node.node @-> int @-> returning void)

let set_node_scalar_at =
  foreign "set_node_scalar_at" (Node.node @-> int @-> int @-> double @-> returning void)
;;

let set_node_trig_at =
  foreign "set_node_trig_at" (Node.node @-> int @-> int @-> returning void)
;;

let get_block_offset = foreign "get_block_offset" (void @-> returning int)

type msg_type =
  | NODE_ADD
  | NODE_SET_SCALAR
  | NODE_SET_TRIG

let msg_type_of_int = function
  | 0 -> NODE_ADD
  | 1 -> NODE_SET_SCALAR
  | 2 -> NODE_SET_TRIG
  | _ -> raise (Invalid_argument "Unexpected C enum")
;;

let msg_type_to_int = function
  | NODE_ADD -> 0
  | NODE_SET_SCALAR -> 1
  | NODE_SET_TRIG -> 1
;;

let get_write_ptr = foreign "get_write_ptr" (void @-> returning int)
let update_bundle = foreign "update_bundle" (int @-> returning void)

let bundle f =
  (* this is naive - can still sometimes end up with bundles getting wrong offsets *)
  let offset = get_block_offset () in
  f offset
;;

(* let trig_msg target input = *)
(*   let  *)
