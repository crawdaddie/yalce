open Ctypes
open Foreign

let tanh_node = foreign "tanh_node" (double @-> Node.node @-> returning Node.node)
let freeverb_node = foreign "freeverb_node" (Node.node @-> returning Node.node)

let comb =
  foreign
    "comb_dyn_node"
    (double @-> double @-> double @-> Node.node @-> returning Node.node)
;;

let op_lp = foreign "op_lp_dyn_node" (double @-> Node.node @-> returning Node.node)
let op_lp_scalar = foreign "op_lp_node_scalar" (double @-> double @-> returning Node.node)

let biquad_lp =
  foreign "biquad_lp_node" (double @-> double @-> Node.node @-> returning Node.node)
;;

let biquad_lp_dyn =
  foreign "biquad_lp_dyn_node" (double @-> double @-> Node.node @-> returning Node.node)
;;

let biquad_hp_dyn =
  foreign "biquad_hp_dyn_node" (double @-> double @-> Node.node @-> returning Node.node)
;;

let butterworth_hp_dyn =
  foreign "butterworth_hp_dyn_node" (double @-> Node.node @-> returning Node.node)
;;
