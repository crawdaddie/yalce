open Yalce;;

init_yalce ()

let clicks () =
  let open Synth_.Make (struct
      let inputs = [ 10.; 0.01 ]
    end) in
  let clicks =
    winblit 10. 0.01
    |> pipe_sig_to_idx 0 (chain_input_sig 0)
    |> pipe_sig_to_idx 1 (chain_input_sig 1)
    |> tanh 10.
  in

  (* let clicks = freeverb clicks in *)
  finish clicks
;;

(* let reverb nodes = *)
(*   let ( ~> ), finish, _ = synth_new () in *)
(*   let s = ~>(sumn nodes) in *)
(*   let s = ~>(mul_scalar 0.5 s) in *)
(*   let r = ~>(freeverb_node s) in *)
(*   finish r *)
(* in *)

let c = clicks ()

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
;;

let rand_float min mx = min +. Random.float (mx -. min);;

while true do
  let fo = Messaging.get_block_offset () in
  Messaging.set_node_scalar_at c fo 0 (rand_choice [| 10.; 20. |]);
  Messaging.set_node_scalar_at c fo 1 (rand_float 0.0001 0.02);
  Thread.delay 0.25
done
