open Ctypes
open Foreign

let sq = foreign "sq_node" (double @-> returning Node.node)
let blsaw = foreign "blsaw_node" (double @-> int @-> returning Node.node)
let lfnoise = foreign "lfnoise" (double @-> double @-> double @-> returning Node.node)
let noise = foreign "white_noise" (void @-> returning Node.node)
let sin = foreign "sine" (double @-> returning Node.node)
let blit = foreign "blit_node" (double @-> int @-> returning Node.node)
let trig_const = foreign "trig_node_const" (double @-> returning Node.node)

let winblit =
  foreign "windowed_impulse_node" (double @-> int @-> double @-> returning Node.node)
;;
