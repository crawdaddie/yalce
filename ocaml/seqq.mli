val sequence_events : float Seq.t -> (unit -> 'a) -> unit Lwt.t
val run : float Seq.t -> (unit -> 'a) -> unit Lwt.t Lwt.t
