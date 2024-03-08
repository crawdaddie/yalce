open Yalce

let sin_input n =
  let sq_node = sin 200. in
  pipe_output n sq_node
;;

let _sq_input n =
  let sq_node = sq 200. in
  pipe_output n sq_node
;;

maketable_sin ();
maketable_sq ();
start_audio ();

let __synth2 () =
  let ( ~> ), finish, _ = synth_new () in
  let sin1 = ~>(sin 100.) +~ ~>(sin 200.) in
  let sin2 = ~>(sin 101.) in
  let sin3 = ~>(sin 8000.) in
  let summed = ~>(sumn [ sin1; sin2; sin3 ]) in
  let summed = ~>(mul sin1 summed) in
  let summed = ~>(mul_scalar 0.2 summed) in
  finish summed
in

let _tanh_node_ gain input =
  let x = tanh_node 2. input in
  pipe_output_to_idx 1 gain x
in

let bass () =
  let choices = [ 110.0; 165.0; 330.0; 110.0; 261.63; 349.23 (* ; 391.99 *) ] in

  let ( ~> ), finish, _ = synth_new () in
  let r = ~>(rand_choice 1.25 @@ List.map (fun x -> x *. 0.5) choices) in
  let sin1 = ~>(sin_input r) in
  let sin1 = ~>(tanh_node 3.0 sin1) in
  finish sin1
in

let sq_detune ~freq =
  let ( ~> ), finish, chain = synth_new ~ins:[ freq ] () in

  let freqlag = ~>(lag_sig 0.008 (Node.in_sig 0 chain)) in

  let sq1 = ~>(sq 110.) in
  let sq1 = pipe_output freqlag sq1 in

  let sq2freq = ~>(mul_scalar 1.007 freqlag) in
  let sq2 = ~>(sq 110.) in
  let sq2 = pipe_output sq2freq sq2 in

  let s = ~>(sq1 +~ sq2) in

  (* let lp = ~>(op_lp 100.0 sq1) in *)
  (* let lp = node_set_input_signal lp 1 (Node.in_sig 1 chain) in *)

  (* let hp = ~>(sumn [ sq1; ( ~> ) (mul_scalar (-1.) lp) ]) in *)
  (* let amp_mod = ~>(sin 2.) in *)
  (* let sq1 = ~>(mul sq1 amp_mod) in *)
  (* finish hp *)
  let s = ~>(comb 0.25 1.0 0.7 s) in
  finish s
in

let synth1 ~freq =
  let ( ~> ), finish, chain = synth_new ~ins:[ freq ] () in
  let s = ~>(sq_detune ~freq) in
  let s = node_set_input_signal s 0 (Node.in_sig 0 chain) in
  finish s
in

let reverb nodes =
  let ( ~> ), finish, _ = synth_new () in
  let s = ~>(sumn nodes) in
  let s = ~>(mul_scalar 0.5 s) in
  let r = ~>(freeverb_node s) in
  finish r
in

let s1 = sq_detune ~freq:110. in
let _ = reverb [ s1; bass () ] in
let rec proc choices =
  let open Messaging in
  let i = Random.int (Array.length choices) in

  let freq = Array.get choices i in
  (* Printf.printf "set scalar\n"; *)
  set_node_scalar s1 0 freq;

  (* set_node_scalar s1 1 (20.0 +. Random.float 5000.0); *)

  (* Printf.printf "iter %d\n" i; *)
  let tchoices = [| 0.125; 0.125; 0.125; 0.25; 0.5 |] in
  let i = Random.int (Array.length tchoices) in
  let t = Array.get tchoices i in
  Thread.delay t;
  proc choices
in

let cc_cb cc v =
  let open Messaging in
  Printf.printf "cc: %ld v: %ld\n" cc v;
  match cc with
  | 21l -> set_node_scalar s1 1 (100. +. (8000. *. Int32.to_float v /. 127.))
  | _ -> Printf.printf "%ld %ld\n" cc v
in

(* let _midi_thread = Midi.Listener.start_midi () in *)
(* Midi.Listener.register_cc_callback cc_cb; *)
let x = Thread.create proc [| 110.0; 165.0; 330.0; 261.63; 349.23; 391.99 |] in
Thread.join x
