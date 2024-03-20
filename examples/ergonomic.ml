open Yalce;;

maketable_sin ();
maketable_sq ();
start_audio ()

let sq_synth () =
  let open Synth.Make (struct
      let inputs = [ 100. ]
    end) in
  let ffreq = 500. in
  let res = 0.1 in
  (* let filt = biquad_lp ffreq res in *)
  let s = chorus_list [ 1.; 1.01; 1.02 ] (chain_input_sig 0) (fun _ -> sq 100.) in
  (* let s = s |> filt in *)
  finish s
;;

let saw_synth () =
  let open Synth.Make (struct
      let inputs = [ 100.; 0. ]
    end) in
  let s =
    chorus_list_lag [ 1.; 1.01; 1.02 ] (chain_input_sig 0) (fun _ -> saw 100.) 0.05
  in
  let ev = env [ 0.; 1.; 0. ] [ 0.05; 1. ] in
  let ev = node_set_input_signal ev 0 (chain_input_sig 1) in
  let s = ev *~ s in
  finish s
;;

let amen () =
  let open Synth.Make (struct
      let inputs = [ 0.; 0.; 1. ]
    end) in
  let b =
    bufplayer "assets/fat_amen_mono_48000.wav"
    |> set_input 0 2
    |> set_input 1 3
    |> set_input 2 1
  in

  finish b
;;

let x = saw_synth ()
let freq_x fo f = Messaging.set_node_scalar_at x fo 0 f
let trig_x fo = Messaging.set_node_trig_at x fo 1
let z = amen ()
let trig_z fo = Messaging.set_node_trig_at z fo 0
let start_pos_z fo p = Messaging.set_node_scalar_at z fo 1 p
let rate_z fo p = Messaging.set_node_scalar_at z fo 2 p

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let tempo_scale = 164. /. 120.

let rec proc () =
  let open Messaging in
  let fo = get_block_offset () in
  (* if Random.int 3 >= 2 *)
  (* then ( *)
  trig_x fo;
  freq_x fo (rand_choice [| 100.; 150.; 180.; 175.; 190.; 300. |]);
  (* ); *)
  let pos = rand_choice [| 0.; 0.25; 0.5; 0.75 |] in

  (* start_pos_z fo pos; *)
  let repeats = rand_choice [| 1; 1; 1; 2; 4; 8 |] in
  let i = ref repeats in
  (* let del = rand_choice [| 0.0625; 0.125; 0.25; 0.5 |] *. (164. /. 120.) in *)
  let rate = ref 1. in

  let pitch_up = rand_choice [| 1.; 1.02; 0.9 |] in
  while !i > 0 do
    let fo = get_block_offset () in
    if pos = 0.25 || pos = 0.75 then rate := !rate *. pitch_up;

    (* trig_x fo; *)
    trig_z fo;
    rate_z fo !rate;
    Thread.delay (tempo_scale *. 0.25 /. Int.to_float repeats);
    i := !i - 1
  done;

  proc ()
;;

let () = proc ()
