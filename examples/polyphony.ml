open Yalce
open Ctypes;;

init_yalce ()

let rand_float min mx = min +. Random.float (mx -. min)

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let synth freq dec =
  let sq = Osc.sq freq in
  let sq2 = Osc.sq (freq *. 1.01) in
  let ev =
    env_node
      2
      ([ 0.; 1.; 0. ] |> CArray.of_list double |> CArray.start)
      ([ rand_choice [| 0.01; 0.2 |]; dec ] |> CArray.of_list double |> CArray.start)
  in
  let sm = sum2 sq sq2 in
  let out = sm *~ ev in
  [ sq; sq2; sm; ev; out ]
;;

let comp arr =
  let g = group_with_inputs 1 ([ 0. ] |> CArray.of_list double |> CArray.start) in
  let rec aux g arr =
    match arr with
    | [] -> g
    | x :: [] ->
      group_add_tail g x;
      let _ = add_to_dac x in
      let _ = ctx_add g in
      add_to_dac g
    | x :: rest ->
      group_add_tail g x;
      aux g rest
  in
  aux g arr
;;

let choices =
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
  comp @@ synth (rand_choice choices /. 4.) (del *. 2.);
  print_ctx ();
  Thread.delay del
done
