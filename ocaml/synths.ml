open Stubs;;
open Nodes;;
open Osc;;
open Fx;;

let sqq x = 
  let node_ptr = osc_sq_detune x in
  node_ptr
    |> lpf 100. 0.2
    |> fx_delay 0.25 0.7;

  let cont = play_node node_ptr in
  { param_map = make_param_map ["freq"]; node_ptr = cont}

