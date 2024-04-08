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
  [| 0; 0; 0; 12; 8; 7; 0; 0; 0; -2; -12 |]
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

let freq_choices = ref freq_choices1 in

let _midi () =
  let _midi_thread = Midi.Listener.start_midi () in
  let cc_cb cc v =
    let open Messaging in
    match cc with
    | 21l -> set_node_scalar g 1 @@ cc_scale 50. 1000. v
    | 22l -> set_node_scalar g 3 @@ cc_scale 5. 50. v
    | _ -> ()
  in

  let () = Midi.Listener.register_cc_callback cc_cb in

  let note_cb n v =
    match n with
    | 40l -> freq_choices := freq_choices1
    | 41l -> freq_choices := freq_choices2
    | _ -> ()
  in

  Midi.Listener.register_noteon_callback note_cb
in

let subdivs = [| [| 0.25; 0.25 |]; [| 0.125; 0.125; 0.125; 0.125 |]; [| 0.5 |] |] in

let rec process_loop subd freqs_idx =
  let freq = !freq_choices.(freqs_idx) in
  let l = Array.length subd in
  let rec aux i =
    match i with
    | l -> ()
    | _ ->
      let del = subd.(i) in
      let fo = Messaging.get_block_offset () in
      if i = 0 then Messaging.schedule_add_node (Drums.kick 55. (0.4 *. del)) fo;
      let () = Messaging.set_node_trig_at g fo 2 in
      let () = Messaging.set_node_scalar_at g fo 0 freq in
      (* Thread.delay (del *. 2. *. (60. /. 145.)); *)
      Lwt_unix.sleep (del *. 2. *. (60. /. 145.));
      aux (i + 1)
  in
  aux 0;

  process_loop (rand_choice subdivs) ((freqs_idx + 1) mod Array.length !freq_choices)
in

process_loop subdivs.(0) 0 |> Lwt_main.run
