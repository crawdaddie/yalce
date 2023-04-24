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


let amen filename rate = 
  let node_ptr = load_sndfile filename |> bufplayer rate in
  let cont = play_node node_ptr in
  { param_map = make_param_map ["buf"; "rate"; "trig"; "start_pos"]; node_ptr = cont}

let amen_ts filename rate ps trig_freq = 
  let node_ptr = load_sndfile filename |> bufplayer_timestretch rate ps trig_freq in
  let cont = play_node node_ptr in
  { param_map = make_param_map ["buf"; "pitch_shift"; "trig_freq"; "speed";]; node_ptr = cont}

let get_bufplayer filename = 
  let buf = load_sndfile "assets/fat_amen_mono_48000.wav" in
  fun rate -> bufplayer rate buf;;

let amenretrig () = 
  let imp = osc_impulse 8. in
  let bp = (get_bufplayer "assets/fat_amen_mono_48000.wav") 1.0 in
  let chain = (imp =>.2) bp in
  let container = play_node chain in
  { param_map = make_param_map [
      "buf";
      "pitch_shift";
      "trig_freq";
      "rate";
    ];
    node_ptr = container}


let trig_amen sp rate nod = 
  set "start_pos" (Float sp) nod
    |> set "rate" (Float rate)
    |> set "trig" (Float 1.)
    |> ignore;

  nod

