open Ctypes
open Foreign

let sq = foreign "sq_node" (double @-> returning Node.node)
let blsaw = foreign "sq_node" (double @-> int @-> returning Node.node)
let lfnoise = foreign "lfnoise" (double @-> double @-> double @-> returning Node.node)
let sin = foreign "sine" (double @-> returning Node.node)
let blit = foreign "blit_node" (double @-> int @-> returning Node.node)

let winblit =
  foreign "windowed_impulse_node" (double @-> int @-> double @-> returning Node.node)
;;

