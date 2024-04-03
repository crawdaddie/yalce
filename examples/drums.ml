open Yalce
open Ctypes;;

init_yalce ()

let buf_sig = Signal.of_sf "assets/sor_sample_pack/Kicks/SOR_OB_Kick_Mohashe.wav"

let compile_synth g l =
  let rec aux g l =
    match l with
    | [] -> g
    | x :: [] ->
      let x = add_to_dac x in
      group_add_tail g x;
      aux g []
    | x :: rest ->
      group_add_tail g x;
      aux g rest
  in

  aux g l
;;

let synth () =
  let g = group_new 2 in
  let p = bufplayer_autotrig_node buf_sig 1. 0. in
  let ev = Envelope.autotrig [ 0.; 1.; 0. ] [ 0.001; 0.25 ] in
  let out = p *~ ev in
  compile_synth g [ p; ev; out ]
;;

(* Messaging.set_node_trig_at g fo 0; *)
while true do
  print_ctx ();
  let g = synth () in
  let fo = Messaging.get_block_offset () in
  Messaging.schedule_add_node g fo;
  Thread.delay 2.0
done
