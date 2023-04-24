open Portmidi

type event =
  [ `Note_on of int * float
  | `Note_off of int
  | `Controller of int * float
  | `Pitch_bend of int * float
  | `Program_change of int * int
  | `Nop (** Do not do anything. This is useful to extend repeated patterns. *)
  ]

type t = {
  mutex: Mutex.t;
  handlers : (int -> int -> int -> unit ) list ref;
}

let log_midi_error fmt err =
  Printf.sprintf fmt (Option.value ~default:"Unknown error" (get_error_text err)) |> Stubs.write_log 


let create () =
  let mutex = Mutex.create () in
  let handlers = ref [] in
  initialize () ;
  let device_id = 0 in (* use device id 0 as default *)
  let buffer_size = Int32.of_int default_sysex_buffer_size in

  let t = match open_input ~device_id:device_id ~buffer_size:buffer_size with
    | Ok input_stream ->
      let handle_msg message  =
        let c = Int32.to_int (message_status message) land 0x0F in
        let cc = Int32.to_int (message_data1 message) in
        let v = Int32.to_int (message_data2 message) in

        Mutex.lock mutex;
        List.iter (fun f -> f c cc v) !handlers;
        Mutex.unlock mutex
      in
      let input_loop stream = 
        while true do
          match poll_input stream with
            | Error error -> log_midi_error "Error polling input stream: %s\n" error
            | Ok false -> ()
            | Ok true ->
              match read_input ~length:1 input_stream with
              | Error error -> log_midi_error "Error reading input stream: %s\n" error
              | Ok [event] -> handle_msg event.message
              | Ok _ -> ()
        done;
        close_input stream;
      in
      Lwt_preemptive.detach input_loop input_stream
  in

  {
    mutex;
    handlers;
  }

let register midi h =
  midi.handlers := h :: !(midi.handlers)

