open Yalce
open Ctypes;;

init_yalce ()

let rand_float min mx = min +. Random.float (mx -. min)

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

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

let synth freq dec =
  let sq1 = Osc.sq freq in
  let sq2 = Osc.sq (freq *. 1.01) in
  let sq_sum = sq1 +~ sq2 in
  let lp = biquad_lp 1000. 10. sq_sum in
  let ev = Envelope.autotrig [ 0.; 1.; 0.2; 0. ] [ 0.01; 0.1; dec ] in
  let o = lp *~ ev in

  compile_synth [ o; ev; lp; sq_sum; sq2; sq1 ]
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
  print_ctx ();

  let g = del |> synth (rand_choice freq_choices /. 4.) in
  let fo = Messaging.get_block_offset () in
  let () = Messaging.schedule_add_node g fo in
  Thread.delay del
done
