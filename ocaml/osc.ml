open Nodes;;
open Stubs;;
  
let sin x =
  let node_ptr = osc_sin x in
  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

let sq x =
  let node_ptr = osc_sq x in

  let param_map = make_param_map ["freq"; "pw"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }


let sq_detune x =
  let node_ptr = osc_sq_detune x in
  let container_ptr = play_node node_ptr in

  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = container_ptr; 
  }


let impulse x =
  let node_ptr = osc_impulse x in
  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }


let saw x =
  let node_ptr = osc_saw x in
  let param_map = make_param_map ["freq"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }


let pulse f pw =
  let node_ptr = osc_pulse f pw in
  let param_map = make_param_map ["freq"; "pw"] in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }



