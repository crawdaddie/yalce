open Yalce
open Ctypes;;

init_yalce ()

let rand_float min mx = min +. Random.float (mx -. min)

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let synth freq dec =
  let sq1 = Osc.sq freq in
  let sq2 = Osc.sq (freq *. 1.01) in
  let ev = Env.autotrig [ 0.; 1.; 0. ] [ 0.01; dec ] in
  let sm = sum2 sq1 sq2 in
  let out = sm *~ ev in
  [ sq1; sq2; sm; ev; out ]
;;

let compile_synth arr =
  let g = group_new () in
  let rec aux g arr =
    match arr with
    | [] -> g
    | x :: [] ->
      let _ = add_to_dac x in
      group_add_tail g x;
      aux g []
    | x :: rest ->
      group_add_tail g x;
      aux g rest
  in

  aux g arr
;;

(* let _ = ctx_add g in *)
(* add_to_dac g *)

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

while true do
  (* let del = rand_choice [| 0.25; 0.5; 1.5; 0.125 |] in *)
  let del = 0.25 in

  let g = del *. 2. |> synth (rand_choice freq_choices /. 2.) |> compile_synth in
  let fo = Messaging.get_block_offset () in
  let () = Messaging.schedule_add_node g fo in
  Thread.delay del
done
