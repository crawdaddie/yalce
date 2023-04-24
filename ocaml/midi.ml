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


let handle_msg message add =
  let status = message_status message in
  let data1 = message_data1 message in
  let data2 = message_data2 message in
  match (Int32.to_int status land 0xF0) with
    | 0xB0 ->
      Printf.sprintf "CC message: channel=%d control=%d value=%d\n"
        (Int32.to_int status land 0x0F) (Int32.to_int data1) (Int32.to_int data2) |> Stubs.write_log
    | _ -> ();

  add status data1 data2



let input_cb input_stream = 
  match poll_input input_stream with
    | Error error -> Printf.sprintf "Error polling input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log
    | Ok false -> ()
    | Ok true ->
      match read_input ~length:1 input_stream with
      | Error error -> Printf.sprintf "Error reading input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log
      | Ok [event] -> handle_msg event.message (fun a b c -> ())
      | Ok _ -> ()

let create () =
  let mutex = Mutex.create () in
  let handlers = ref [] in
  initialize () ;
  let device_id = 0 in (* use device id 0 as default *)
  let buffer_size = Int32.of_int default_sysex_buffer_size in

  let t = match open_input ~device_id:device_id ~buffer_size:buffer_size with
    (* | Error error -> Printf.sprintf "Error opening input stream" |> Stubs.write_log *)
    | Ok input_stream ->
      let add message  =
        let c = Int32.to_int (message_status message) land 0x0F in
        let cc = Int32.to_int (message_data1 message) in
        let v = Int32.to_int (message_data2 message) in

        Mutex.lock mutex;
        List.iter (fun f -> f c cc v) !handlers;
        Mutex.unlock mutex
      in
      let input_loop i_s = 
        while true do
          match poll_input input_stream with
            | Error error -> Printf.sprintf "Error polling input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log
            | Ok false -> ()
            | Ok true ->
              match read_input ~length:1 input_stream with
              | Error error -> Printf.sprintf "Error reading input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log
              | Ok [event] -> add event.message
              | Ok _ -> ()
        done;
        close_input i_s;
      in
      Lwt_preemptive.detach input_loop input_stream
  in

  {
    mutex;
    handlers;
  }

let register midi h =
  midi.handlers := h :: !(midi.handlers)

