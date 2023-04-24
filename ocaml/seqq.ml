
(* let rec sequence_events events f = *)
(*   match events with *)
(*   | [] -> Lwt.return_unit *)
(*   | event :: rest -> *)
(*       Lwt.bind (Lwt_unix.sleep 0.125) (fun () -> *)
(*         f event; *)
(*         sequence_events rest f *)
(*       ) *)

(* let rec sequence_events times seq f = *)
(*   match times with *)
(*   | Seq.Nil -> Lwt.return_unit *)
(*   | Seq.Cons (time, rest_times) -> match seq with *)
(*     | Seq.Nil -> Lwt.return_unit *)
(*     | Seq.Cons (v, rest_values) ->  *)
(*         Lwt.bind (Lwt_unix.sleep time) (fun () -> *)
(*           f v; *)
(*           sequence_events (rest_times ()) (rest_values ()) f *)
(*         ) *)
(*  *)
(*  *)

let rec sequence_events times f =
  f ();
  match times () with
  | Seq.Nil -> Lwt.return_unit
  | Seq.Cons (time, rest_times) ->
      Lwt.bind (Lwt_unix.sleep time) (
        fun () ->
          f ();
          sequence_events rest_times f
        )

let run times f =
  Lwt_preemptive.detach (sequence_events times) f
