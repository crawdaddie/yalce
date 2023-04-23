open Portmidi
let print_cc_message message =
  let status = message_status message in
  let data1 = message_data1 message in
  let data2 = message_data2 message in
      Printf.sprintf "CC message: channel=%d control=%d value=%d\n"
        (Int32.to_int data1 land 0x0F) (Int32.to_int data1 lsr 8) (Int32.to_int data2) |> Stubs.write_log

  (* match Int32.to_int status with *)
  (* |  0xB0 ->  *)
  (* | _ -> () *)

let input_cb input_stream = 
  match poll_input input_stream with
    | Error error -> Printf.sprintf "Error polling input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log

    | Ok false -> ()
    | Ok true ->
      match read_input ~length:1 input_stream with
      | Error error -> Printf.sprintf "Error reading input stream: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log

      | Ok [event] -> print_cc_message event.message
      | Ok _ -> ()

(* match get_device_info device_id with *)
(* | None -> Printf.sprintf "Device %d not found\n" device_id *)
(* | Some device_info when not device_info.input -> *)
(*     Printf.sprintf "Device %d is not an input device\n" device_id; *)

let create () =

  let device_id = 0 in (* use device id 0 as default *)
  let buffer_size = Int32.of_int default_sysex_buffer_size in

  match initialize () with
    | Error error -> Printf.sprintf "Error initializing Portmidi: %s\n" (Option.value ~default:"Unknown error" (get_error_text error)) |> Stubs.write_log
    | Ok () -> ();

  match get_device_info device_id with
    | None -> Printf.sprintf "Device %d not found\n" device_id |> Stubs.write_log

    | Some device_info when not device_info.input ->
        Printf.sprintf "Device %d is not an input device\n" device_id |> Stubs.write_log


    | Some device_info -> match device_info.name with
      | None -> ()
      | Some name -> Printf.sprintf "Device %s found\n" name |> Stubs.write_log; 

  match open_input ~device_id:0 ~buffer_size:buffer_size with
    | Error error -> Printf.sprintf "Error opening input stream" |> Stubs.write_log
    | Ok input_stream ->
      let input_loop () = 
        while true do
          input_cb input_stream
        done;
        close_input input_stream;
      in
      Lwt_preemptive.detach input_loop ();
 ()
