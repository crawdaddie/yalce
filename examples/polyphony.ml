open Yalce
open Ctypes;;

init_yalce ()

let rand_float min mx = min +. Random.float (mx -. min)

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

module type Monad = sig
  type 'a t

  val return : 'a -> 'a t
  val ( >>= ) : 'a t -> ('a -> 'b t) -> 'b t
end

module Synth = struct
  type 'a t = 'a * Node.node list

  let chain = ref []

  let add_to_chain x =
    chain
      := match !chain with
         | [] -> [ x ]
         | h :: rest when h = x -> !chain
         | r -> x :: r
  ;;

  let ( ~. ) x =
    add_to_chain x;
    x
  ;;

  let ( >>= ) m f =
    let x, s1 = m in
    let y, s2 = f x in
    y, List.append s1 s2
  ;;

  (* chain := x :: !chain *)

  let ( +~ ) x f =
    add_to_chain f;
    add_to_chain x;
    let y = sum2 x f in
    add_to_chain y;
    y
  ;;

  let ( *~ ) x f =
    add_to_chain x;
    add_to_chain f;
    let y = mul x f in
    add_to_chain y;
    y
  ;;

  let ( let* ) x m =
    add_to_chain x;
    x
  ;;

  let ( and* ) m x = ()

  (* let cont m f = *)
  (*   let x, s1 = m in *)
  (*   let y, s2 = ~.f in *)
  (*   y, List.append s1 s2 *)
  (* ;; *)
  (* let compile_synth ch = () *)

  let compile_synth arr =
    let out_layout = List.hd arr |> Node.out |> Signal.layout in
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

  let output x =
    add_to_chain x;
    let _ = add_to_dac x in
    compile_synth !chain
  ;;
end

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
  let ev = Envelope.autotrig [ 0.; 1.; 0.2; 0. ] [ 0.01; 0.1; dec ] in
  let o = sq_sum *~ ev in
  compile_synth [ o; ev; sq_sum; sq2; sq1 ]
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
