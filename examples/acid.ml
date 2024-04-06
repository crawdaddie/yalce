open Yalce
open Ctypes
open Foreign;;

init_yalce ()

let compile_synth arr =
  let _ = add_to_dac @@ List.hd arr in
  let out_layout = 1 in
  let g = group_new out_layout in
  let rec aux g arr =
    match arr with
    | [] -> g
    | x :: rest ->
      group_add_head g x;
      aux g rest
  in
  aux g arr
;;

module type SynthSignature = sig
  val inputs : float list
  val layout : int
end

let scale_ =
  foreign
    "scale_node_dyn"
    (Signal.signal @-> Signal.signal @-> Node.node @-> returning Node.node)
;;

module Synth = struct
  module Make (X : SynthSignature) = struct
    let chain =
      match X.inputs with
      | [] -> ref @@ group_new X.layout
      | ins -> ref @@ group_with_inputs X.layout (List.length ins) (list_to_carr ins)
    ;;

    let chain_wrap n =
      let () = group_add_tail !chain n in
      n
    ;;

    let scale mn mx n = scale_ mn mx n |> chain_wrap
    let sum2 a b = sum2 a b |> chain_wrap
    let ( +~ ) = sum2
    let mul_ a b = mul a b |> chain_wrap
    let ( *~ ) = mul_
    let mul_scalar s n = mul_scalar s n |> chain_wrap
    let sq f = Osc.sq f |> chain_wrap
    let saw f = Osc.blsaw f 100 |> chain_wrap
    let env_trig l t = Envelope.trig l t |> chain_wrap
    let env_autotrig l t = Envelope.autotrig l t |> chain_wrap
    let biquad f r n = biquad_lp_dyn f r n |> chain_wrap
    let in_sig i = Node.in_sig i !chain
    let map_input src dest node = pipe_sig_to_idx dest (in_sig src) node
    let map_sig src_sig dest_input node = pipe_sig_to_idx dest_input src_sig node
    let mul_scalar_sig_to_node d isig = mul_scalar_sig_node d isig |> chain_wrap
    let mul_sig mul_val isig = mul_scalar_sig_to_node mul_val isig |> Node.out
    let lag_sig t s = lag_sig t s |> chain_wrap
    let tanh gain n = Filter.tanh_node gain n |> chain_wrap

    let output n =
      let _ = add_to_dac n in
      !chain
    ;;
  end
end

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let synth freq dec =
  let open Synth.Make (struct
      let inputs = [ 100. (* osc freq *); 150. (* filter freq *); 0. (* trig *) ]
      let layout = 1
    end) in
  let freq = lag_sig 0.02 (in_sig 0) in
  let sq1 = sq 100. |> map_sig (Node.out freq) 0 in
  let freq2_sig = Node.out (mul_scalar 1.01 freq) in
  let sq2 = sq 100. |> map_sig freq2_sig 0 in
  let s = sq1 +~ sq2 in

  (* let s = sq1 in *)
  let filt_env =
    env_trig [ 1.; 0. ] [ 0.5 ]
    |> pipe_sig_to_idx 0 (in_sig 2)
    |> scale (in_sig 1) (mul_sig 4. (in_sig 1))
  in

  let filt = biquad 100. 10. s |> map_sig (Node.out filt_env) 1 (* |> tanh 1.2 *) in

  let out =
    env_trig [ 1.; 0.5; 0. ] [ 0.2; 0.5 ] |> map_sig (in_sig 2) 0 |> ( *~ ) filt
  in

  output out
;;

(* let sq1 = Osc.sq freq in *)
(* let sq2 = Osc.sq (freq *. 1.01) in *)
(* let sq_sum = sq1 +~ sq2 in *)
(* let lp = biquad_lp_dyn 1000. 10. sq_sum in *)
(* let ev = Envelope.trig [ 0.; 1.; 0.2; 0. ] [ 0.01; 0.1; dec ] in *)
(* let o = lp *~ ev in *)
(* compile_synth [ o; ev; lp; sq_sum; sq2; sq1 ] *)

let freq_choices =
  [| 220.0
   ; 246.94165062806206
   ; 261.6255653005986
   ; 293.6647679174076
   ; 329.6275569128699
   ; 349.2282314330039
   ; 391.99543598174927
   ; 880.0
  |]
;;

let g = synth 100. 0.2 in
let fo = Messaging.get_block_offset () in
let () = Messaging.schedule_add_node g fo in

let cc_cb cc v =
  let open Messaging in
  (* Printf.printf "cc: %ld v: %ld\n" cc v *)
  match cc with
  | 21l -> set_node_scalar g 1 (50. +. (1000. *. Int32.to_float v /. 127.))
  | _ -> Printf.printf "%ld %ld\n" cc v
in
let _midi_thread = Midi.Listener.start_midi () in
Midi.Listener.register_cc_callback cc_cb;

while true do
  let fo = Messaging.get_block_offset () in
  let () = Messaging.set_node_trig_at g fo 2 in
  let () = Messaging.set_node_scalar_at g fo 0 (rand_choice freq_choices /. 8.) in
  Thread.delay @@ (60. /. 145. *. rand_choice [| 0.25 |])
done
