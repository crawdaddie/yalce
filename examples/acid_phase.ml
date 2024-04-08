open Yalce
open Ctypes
open Foreign
open Lwt;;

init_yalce ()

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let midi_ratio midinote = 2. ** (Float.of_int midinote /. 12.) in

let freq_choices1 =
  [| 0; 2; 7; 8; 10; 2; 0; 8; 7; 2; 8; 7 |]
  |> Array.map (fun midinote -> 110. *. midi_ratio midinote)
in

let freq_choices2 =
  [| 0; 0; 0; 12; 8; 7; 0; 0; 0 |]
  |> Array.map (fun m -> m - 5)
  |> Array.map (fun midinote -> 110. *. midi_ratio midinote)
in

let g = Synths.acid_mono 100. 0.2 in
let k = Drums.kick (55. *. midi_ratio 0) 0.5 in
(* let r = Filter.freeverb_node g in *)
let fo = Messaging.get_block_offset () in

let () = Messaging.schedule_add_node g fo in
let () = Messaging.schedule_add_node k fo in

(* let () = Messaging.schedule_add_node r fo in *)
let cc_scale mn mx v = mn +. ((mx -. mn) *. Int32.to_float v /. 127.) in

let _midi_thread = Midi.Listener.start_midi () in
let cc_cb cc v =
  let open Messaging in
  match cc with
  | 21l -> set_node_scalar g 1 @@ cc_scale 50. 1000. v
  | 22l -> set_node_scalar g 3 @@ cc_scale 5. 50. v
  | _ -> ()
in

let () = Midi.Listener.register_cc_callback cc_cb in

let freq_choices = ref freq_choices1 in
let note_cb n v =
  match n with
  | 40l -> freq_choices := freq_choices1
  | 41l -> freq_choices := freq_choices2
  | _ -> ()
in

let () = Midi.Listener.register_noteon_callback note_cb in

(* let subdivs = [| [| 0.25; 0.25 |]; *)
(*   [| 0.125; 0.125; 0.125; 0.125 |]; [| 0.5 |] |] in *)
(* let subdivs = [| [| 0.25; 0.25 |]; [| 0.125; 0.125; 0.125; 0.125 |]; [| 0.5 |] |] in *)
(* let subdivs = [| [| 0.5 |]; [| 0.25; 0.25 |] |] in *)
let subdivs = [| [| 0.5 |] |] in
let tempo = 400. in

let rec process_loop subd freqs_idx =
  let freq = freq_choices1.(freqs_idx) in
  let fmod12 = freqs_idx mod 12 in
  let fo = Messaging.get_block_offset () in
  if fmod12 = 0 || fmod12 = 3 || fmod12 = 6
  then Messaging.schedule_add_node (Drums.kick (rand_choice [| 55.; 55. *. 1.5 |]) 0.5) fo;
  let rec aux i l =
    match l with
    | [] -> Lwt.return_unit
    | del :: res ->
      let fo = Messaging.get_block_offset () in

      let () = Messaging.set_node_trig_at g fo 2 in
      let () =
        Messaging.set_node_scalar_at g fo 0 (freq *. rand_choice [| 0.5; 1.; 2. |])
      in
      let%lwt () = Lwt_unix.sleep (del *. 2. *. (60. /. tempo)) in
      aux (i + 1) res
  in
  let%lwt () = aux 0 @@ Array.to_list subd in
  process_loop (rand_choice subdivs) ((freqs_idx + 1) mod Array.length freq_choices1)
in

process_loop subdivs.(0) 0 |> Lwt_main.run
