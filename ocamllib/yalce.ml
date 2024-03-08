open Ctypes
open Foreign
module Midi = Midi
module Messaging = Messaging
module Signal = Signal
module Node = Node
module Osc = Osc
module Filter = Filter

type audio_ctx = unit ptr

let audio_ctx : audio_ctx typ = ptr void
let start_audio = foreign "start_audio" (void @-> returning void)
let get_audio_ctx = foreign "get_audio_ctx" (void @-> returning audio_ctx)
let maketable_sin = foreign "maketable_sin" (void @-> returning void)
let maketable_sq = foreign "maketable_sq" (void @-> returning void)
let pipe_output = foreign "pipe_output" (Node.node @-> Node.node @-> returning Node.node)
let pipe_sig = foreign "pipe_sig" (Signal.signal @-> Node.node @-> returning Node.node)

let pipe_output_to_idx =
  foreign "pipe_output_to_idx" (int @-> Node.node @-> Node.node @-> returning Node.node)
;;

let pipe_sig_to_idx =
  foreign "pipe_sig_to_idx" (int @-> Signal.signal @-> Node.node @-> returning Node.node)
;;

let rand_choice_ =
  foreign "rand_choice_node" (double @-> int @-> ptr double @-> returning Node.node)
;;

let bufplayer_node_ = foreign "bufplayer_node" (ptr char @-> returning Node.node)

(* let bufplayer_pitchshift_node_ = *)
(*   foreign "bufplayer_pitchshift_node" (ptr char @-> returning Node.node) *)
(* ;; *)

let mul = foreign "mul_node" (Node.node @-> Node.node @-> returning Node.node)
let mul_scalar = foreign "mul_scalar_node" (double @-> Node.node @-> returning Node.node)
let add_scalar = foreign "add_scalar_node" (double @-> Node.node @-> returning Node.node)
let lag_sig = foreign "lag_sig" (double @-> Signal.signal @-> returning Node.node)
let scale = foreign "scale_node" (double @-> double @-> Node.node @-> returning Node.node)

let scale2 =
  foreign "scale2_node" (double @-> double @-> Node.node @-> returning Node.node)
;;

let bufplayer_node filename =
  let filename_ptr = CArray.start (CArray.of_string filename) in
  bufplayer_node_ filename_ptr
;;

(* let bufplayer_pitchshift_node filename = *)
(*   let filename_ptr = CArray.start (CArray.of_string filename) in *)
(*   bufplayer_pitchshift_node_ filename_ptr *)
(* ;; *)

let sum_nodes_ =
  foreign "sum_nodes" (int @-> Node.node @-> Node.node @-> returning Node.node)
;;

let sum_nodes_arr = foreign "sum_nodes_arr" (int @-> ptr Node.node @-> returning Node.node)

let mix_nodes_arr =
  foreign "mix_nodes_arr" (int @-> ptr Node.node @-> ptr double @-> returning Node.node)
;;

let ctx_add = foreign "ctx_add" (Node.node @-> returning Node.node)
let add_to_chain = foreign "add_to_chain" (Node.node @-> Node.node @-> returning Node.node)
let add_to_dac = foreign "add_to_dac" (Node.node @-> returning Node.node)
let chain_new = foreign "chain_new" (void @-> returning Node.node)

let chain_set_out =
  foreign "chain_set_out" (Node.node @-> Node.node @-> returning Node.node)
;;

let chain_with_inputs =
  foreign "chain_with_inputs" (int @-> ptr double @-> returning Node.node)
;;

let node_set_input_signal =
  foreign
    "node_set_input_signal"
    (Node.node @-> int @-> Signal.signal @-> returning Node.node)
;;

let list_to_carr l =
  let carr = CArray.of_list double l in
  CArray.start carr
;;

let synth_new ?(ins = []) _ =
  let chain =
    match ins with
    | [] -> chain_new ()
    | ins -> chain_with_inputs (List.length ins) (list_to_carr ins)
  in

  let add x = add_to_chain chain x in

  let fin out =
    let _ = add_to_dac chain in
    let _ = ctx_add chain in
    chain_set_out chain out
  in
  add, fin, chain
;;

let rand_choice freq choices =
  let len = List.length choices in
  let choices_carr = CArray.of_list double choices in
  rand_choice_ freq len @@ CArray.start choices_carr
;;

let node_list_to_carr_ptr nodes =
  let len = List.length nodes in
  len, CArray.start (CArray.of_list Node.node nodes)
;;

let sumn nodes =
  let len, ptr = node_list_to_carr_ptr nodes in
  sum_nodes_arr len ptr
;;

let mix nodes scalars =
  let len, ptr = node_list_to_carr_ptr nodes in
  let scalars_ptr = CArray.of_list double scalars |> CArray.start in
  mix_nodes_arr len ptr scalars_ptr
;;

let pipe_input a b = pipe_output b a
let ( => ) = pipe_output
let ( =< ) = pipe_input
let sum2 x y = sumn [ x; y ]
let ( +~ ) = sum2
let ( *~ ) = mul

module type SynthSignature = sig
  val inputs : float list
end

module Synth = struct
  module Make (X : SynthSignature) = struct
    let chain =
      match X.inputs with
      | [] -> ref @@ chain_new ()
      | ins -> ref @@ chain_with_inputs (List.length ins) (list_to_carr ins)
    ;;

    let chain_wrap f = add_to_chain !chain f
    let sq f = Osc.sq f |> chain_wrap
    let saw f = Osc.blsaw f 100 |> chain_wrap
    let blit f = Osc.blit f 1000 |> chain_wrap
    let winblit f t = Osc.winblit f 800 t |> chain_wrap

    let sum2 x y =
      let len, arr = node_list_to_carr_ptr [ x; y ] in
      chain_wrap (sum_nodes_arr len arr)
    ;;

    let ( +~ ) = sum2
    let bufplayer filename = bufplayer_node filename |> chain_wrap
    let sumn nodes = sumn nodes |> chain_wrap
    let lag_sig t s = lag_sig t s |> chain_wrap
    let mul_scalar f x = mul_scalar f x |> chain_wrap

    let finish out =
      let _ = add_to_dac !chain in
      let _ = ctx_add !chain in
      chain_set_out !chain out
    ;;

    let chain_input_sig idx = Node.in_sig idx !chain

    let set_input src_idx dest_idx node =
      node_set_input_signal node dest_idx (chain_input_sig src_idx)
    ;;

    let freeverb input = Filter.freeverb_node input |> chain_wrap
    let tanh x input = Filter.tanh_node x input |> chain_wrap

    (* let pipe_sig_to_idx = *)
    (*   foreign "pipe_sig_to_idx" (int @-> Signal.signal @-> Node.node @-> returning Node.node) *)
    (* ;; *)

    let chorus_list list input f =
      List.map
        (fun freq_mul ->
          let freq = lag_sig 0.012 input |> mul_scalar freq_mul in
          let n = f () in
          pipe_output freq n)
        list
      |> sumn
      |> mul_scalar @@ (1. /. (Int.to_float @@ List.length list))
    ;;
  end
end

let init_yalce () =
  maketable_sin ();
  maketable_sq ();
  start_audio ()
;;

(* let start_audio = foreign "start_audio" (void @-> returning void) *)

let create_window = foreign "create_window" (void @-> returning void)

let create_spectrogram_window =
  foreign "create_spectrogram_window" (void @-> returning void)
;;
