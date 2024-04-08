open Yalce
open Lwt;;

init_yalce ();

let del_seq =
  [| 0.25, 1.
   ; 0.25, 0.5
   ; 0.5, 0.5
   ; 0.25, 0.5
   ; 0.5, 0.5
   ; 0.5, 0.5
   ; 0.25, 0.5
   ; 0.5, 0.5
   ; 0.25, 0.5
   ; 0.25, 0.5
   ; 0.5, 0.2
     (* ; 0.125, 0.5 *)
     (* ; 0.125, 0.5 *)
     (* ; 0.125, 0.5 *)
     (* ; 0.125, 0.5 *)
  |]
in

let rand_choice arr =
  let i = Random.int (Array.length arr) in
  arr.(i)
in

let rec proc i =
  (* let del, accent = del_seq.(i) in *)
  let fo = Messaging.get_block_offset () in
  (* if i = 0 then Messaging.schedule_add_node (Drums.kick (rand_choice [| 55. |]) 1.) fo; *)
  Messaging.schedule_add_node
    (Drums.snare ~freq:(rand_choice [| 300.; 500. |]) (rand_choice [| 0.5; 0.25; 1. |]))
    fo;

  let%lwt () = Lwt_unix.sleep (0.125 *. (60. /. 80.)) in
  proc ((i + 1) mod Array.length del_seq)
in

proc 0 |> Lwt_main.run
