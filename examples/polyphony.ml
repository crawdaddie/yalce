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
  let ev = Env.autotrig [ 0.; 1.; 0. ] [ rand_choice [| 0.01; 0.2 |]; dec ] in
  let sm = sum2 sq1 sq2 in
  let out = sm *~ ev in
  [ sq1; sq2; sm; ev; out ]
;;

let compile_synth arr =
  let g = group_with_inputs 1 ([ 0. ] |> CArray.of_list double |> CArray.start) in
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

  let g = aux g arr in
  let _ = ctx_add g in
  add_to_dac g
;;

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
  let del = rand_choice [| 0.25; 0.5; 1.5; 0.125 |] in

  let _g = del |> synth (rand_choice freq_choices /. 4.) |> compile_synth in
  Thread.delay del
done
