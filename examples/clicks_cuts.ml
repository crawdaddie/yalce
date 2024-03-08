open Yalce;;

init_yalce ()

let clicks () =
  let open Synth.Make (struct
      let inputs = [ 100. ]
    end) in
  let clicks = blit 10. in
  let clicks = pipe_sig_to_idx 0 (chain_input_sig 0) clicks |> tanh 10. in

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

while true do
  let fo = Messaging.get_block_offset () in
  Messaging.set_node_scalar_at c fo 0 (rand_choice [| 10.; 20.; 40. |]);
  Thread.delay 0.25
done
