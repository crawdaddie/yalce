external start_audio: unit -> unit = "caml_start_audio"
external oscilloscope: unit -> unit = "caml_oscilloscope"

external kill_audio: unit -> unit = "caml_kill_audio" 

external add_node: unit -> unit = "caml_add_node" 
external chain_nodes: unit -> unit = "caml_chain_nodes" 

type node_ptr

external play_sin: float -> node_ptr = "caml_play_sin"
module StringMap = Map.Make(String)
type param_map = int StringMap.t


type node = { param_map: param_map; node_ptr: node_ptr }

let set_param node key value =
  let updated_param_map = StringMap.add key value node.param_map in
  { node with param_map = updated_param_map }

let sin x =
  let node_ptr = play_sin x in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

let node_param_idx node param_name =
  StringMap.find param_name node.param_map

external set_sig: node_ptr -> int -> float -> unit = "caml_set_sig"

let node_set node param_name value = 
  let param_idx = node_param_idx node param_name in 
  set_sig node.node_ptr param_idx value


external set_freq: node_ptr -> float -> unit = "caml_set_freq"
external play_sq: float -> node_ptr = "caml_play_sq"

let sq x =
  let node_ptr = play_sq x in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external play_sq_detune: float -> node_ptr = "caml_play_sq_detune"

let sq_detune x =
  let node_ptr = play_sq_detune x in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external play_impulse: float -> node_ptr = "caml_play_impulse"

let impulse x =
  let node_ptr = play_impulse x in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external play_saw: float -> node_ptr = "caml_play_poly_saw"

let saw x =
  let node_ptr = play_saw x in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external play_hoover: float -> node_ptr = "caml_play_hoover"
external play_pulse: float -> float -> node_ptr = "caml_play_pulse"

let pulse f pw =
  let node_ptr = play_pulse f pw in
  let param_map = StringMap.add "freq" 0 (StringMap.empty
    |> StringMap.add "pw" 1
    |> StringMap.add "amp" 2
  ) in
  {
    param_map = param_map;
    node_ptr = node_ptr; 
  }

external play_blip: float -> float -> node_ptr = "caml_play_blip"


external stop: node_ptr -> unit = "caml_kill_node"
external dump_nodes: unit -> unit = "caml_dump_nodes"

