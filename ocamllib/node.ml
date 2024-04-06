open Ctypes
open Foreign
module Signal = Signal

type node_type =
  | INTERMEDIATE
  | OUTPUT

let node_type_of_int = function
  | 0 -> INTERMEDIATE
  | 1 -> OUTPUT
  | _ -> raise (Invalid_argument "Unexpected C enum")
;;

let node_type_to_int = function
  | INTERMEDIATE -> 0
  | OUTPUT -> 1
;;

let node_type = Ctypes.view ~read:node_type_of_int ~write:node_type_to_int Ctypes.int

type node_struct

let node_struct : node_struct structure typ = structure "Node"
let node_killed = field node_struct "killed" bool
let node_typef = field node_struct "type" int
let node_perform = field node_struct "node_perform" (ptr void)
let node_state = field node_struct "state" (ptr void)
let node_ins = field node_struct "ins" (ptr Signal.signal)
let node_num_ins = field node_struct "num_ins" int
let node_out = field node_struct "out" Signal.signal
let () = seal node_struct

type node

let node = ptr node_struct
let get_input_sig = foreign "get_input_sig" (node @-> int @-> returning Signal.signal)
let get_output_sig = foreign "get_output_sig" (node @-> returning Signal.signal)
let ins node = getf !@node node_ins
let in_sig i node = get_input_sig node i
let out node = get_output_sig node
