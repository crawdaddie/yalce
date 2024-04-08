open Ctypes
open Foreign
module Midi = Midi
module Messaging = Messaging
module Signal = Signal
module Node = Node
module Osc = Osc
module Filter = Filter
module Envelope = Envelope

type audio_ctx = unit ptr

let audio_ctx : audio_ctx typ = ptr void
let start_audio = foreign "start_audio" (void @-> returning void)
let cleanup_job = foreign "cleanup_job" (void @-> returning void)
let get_audio_ctx = foreign "get_audio_ctx" (void @-> returning audio_ctx)
let maketable_sin = foreign "maketable_sin" (void @-> returning void)
let maketable_sq = foreign "maketable_sq" (void @-> returning void)
let pipe_output = foreign "pipe_output" (Node.node @-> Node.node @-> returning Node.node)
let pipe_sig = foreign "pipe_sig" (Signal.signal @-> Node.node @-> returning Node.node)
let print_ctx = foreign "print_ctx" (void @-> returning void)

let pipe_output_to_idx =
  foreign "pipe_output_to_idx" (int @-> Node.node @-> Node.node @-> returning Node.node)
;;

let pipe_sig_to_idx =
  foreign "pipe_sig_to_idx" (int @-> Signal.signal @-> Node.node @-> returning Node.node)
;;

let rand_choice_ =
  foreign "rand_choice_node" (double @-> int @-> ptr double @-> returning Node.node)
;;

(* let bufplayer_node_ = foreign "bufplayer_node" (ptr char @-> returning Node.node) *)
let bufplayer_node = foreign "bufplayer_node" (Signal.signal @-> returning Node.node)

let bufplayer_autotrig_node =
  foreign
    "bufplayer_autotrig_node"
    (Signal.signal @-> double @-> double @-> returning Node.node)
;;

(* let bufplayer_pitchshift_node_ = *)
(*   foreign "bufplayer_pitchshift_node" (ptr char @-> returning Node.node) *)
(* ;; *)

let mul = foreign "mul_nodes" (Node.node @-> Node.node @-> returning Node.node)
let mul_scalar = foreign "mul_scalar_node" (double @-> Node.node @-> returning Node.node)

let mul_scalar_sig_node =
  foreign "mul_scalar_sig_node" (double @-> Signal.signal @-> returning Node.node)
;;

let add_scalar = foreign "add_scalar_node" (double @-> Node.node @-> returning Node.node)
let lag_sig = foreign "lag_sig" (double @-> Signal.signal @-> returning Node.node)
let scale = foreign "scale_node" (double @-> double @-> Node.node @-> returning Node.node)

let scale2 =
  foreign "scale2_node" (double @-> double @-> Node.node @-> returning Node.node)
;;

(* let bufplayer_node filename = *)
(*   let filename_ptr = CArray.start (CArray.of_string filename) in *)
(*   bufplayer_node_ filename_ptr *)
(* ;; *)

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

let add_to_chain =
  foreign "group_add_tail" (Node.node @-> Node.node @-> returning Node.node)
;;

let add_to_dac = foreign "add_to_dac" (Node.node @-> returning Node.node)
let chain_new = foreign "group_new" (void @-> returning Node.node)
let group_new = foreign "group_new" (int @-> returning Node.node)
let group_add_tail = foreign "group_add_tail" (Node.node @-> Node.node @-> returning void)
let group_add_head = foreign "group_add_head" (Node.node @-> Node.node @-> returning void)

let group_with_inputs =
  foreign "group_with_inputs" (int @-> int @-> ptr double @-> returning Node.node)
;;

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

let init_yalce () =
  maketable_sin ();
  maketable_sq ();
  start_audio ();
  cleanup_job ()
;;

(* let start_audio = foreign "start_audio" (void @-> returning void) *)

let create_window = foreign "create_window" (void @-> returning void)

let create_spectrogram_window =
  foreign "create_spectrogram_window" (void @-> returning void)
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
    let sin f = Osc.sin f |> chain_wrap
    let env_trig l t = Envelope.trig l t |> chain_wrap
    let env_autotrig l t = Envelope.autotrig l t |> chain_wrap
    let biquad_lp f r n = Filter.biquad_lp_dyn f r n |> chain_wrap
    let biquad_hp f r n = Filter.biquad_hp_dyn f r n |> chain_wrap
    let butterworth_hp f n = Filter.butterworth_hp_dyn f n |> chain_wrap
    let comb t max_t fb node = Filter.comb t max_t fb node |> chain_wrap
    let in_sig i = Node.in_sig i !chain
    let map_input src dest node = pipe_sig_to_idx dest (in_sig src) node
    let map_sig src_sig dest_input node = pipe_sig_to_idx dest_input src_sig node
    let mul_scalar_sig_to_node d isig = mul_scalar_sig_node d isig |> chain_wrap
    let mul_sig mul_val isig = mul_scalar_sig_to_node mul_val isig |> Node.out
    let lag_sig t s = lag_sig t s |> chain_wrap
    let tanh gain n = Filter.tanh_node gain n |> chain_wrap
    let impulse_const freq = Osc.trig_const freq |> chain_wrap
    let noise () = Osc.noise () |> chain_wrap

    let output n =
      let _ = add_to_dac n in
      !chain
    ;;
  end
end
