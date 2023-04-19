open Stubs;;
open Nodes;;

let delay t f n =
  let node_ptr = fx_delay t f n in 

  let param_map = make_param_map ["time"; "fb"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }
