open Base
open Stdio

let parse_msg status data1 data2 =
  let msg_code = Int32.bit_and status (Int32.of_int_exn 0xF0) in
  let channel = Int32.bit_and status (Int32.of_int_exn 0x0F) in
  printf "debug code: %ld chan :%ld\n" msg_code (Int32.( + ) channel (Int32.of_int_exn 1));
  printf "data1: %ld\n" data1;
  printf "data2: %ld\n" data2
;;

let msg_callback msg =
  parse_msg
    (Portmidi.message_status msg)
    (Portmidi.message_data1 msg)
    (Portmidi.message_data2 msg);
  ()
;;

let extract_cc_value msg =
  let open Int32 in
  let status_byte = msg lsr 24 in
  (* let command = status_byte land of_int_exn 0xF0 in *)
  let channel = status_byte land 0x0Fl in

  let data_byte1 = (msg lsr 16) land 0x7Fl in
  let data_byte2 = ((msg + 1l) lsr 8) land 0x7Fl in
  channel, data_byte1, data_byte2
;;

let code_chan status =
  let msg_code = Int32.bit_and status (Int32.of_int_exn 0xF0) in
  let channel = Int32.bit_and status (Int32.of_int_exn 0x0F) in
  msg_code, Int32.to_int_exn channel
;;

module Listener = struct
  type cc_callback = int32 -> int32 -> unit

  let cc_chan_callbacks : cc_callback array = Array.create ~len:129 (fun _ _ -> ())

  let midi_listener () =
    let () =
      match Portmidi.initialize () with
      | Ok () -> ()
      | Error _e -> failwith "error initializing portmidi"
    in
    let device_id = 1 in
    match Portmidi.open_input ~device_id ~buffer_size:512l with
    | Ok stream ->
      printf "device %d successfully opened for input!\n" device_id;
      let rec loop () =
        match Portmidi.read_input ~length:10 stream with
        | Error err ->
          failwith
          @@ Printf.sprintf
               "failed to read 1: %s"
               (Portmidi.get_error_text err |> Option.value ~default:"null")
        | Ok [] -> loop ()
        | Ok lst ->
          (* printf "got %d records\n" (List.length lst); *)
          List.iter lst ~f:(fun pme ->
            let msg = pme.Portmidi.Portmidi_event.message in

            let cb = cc_chan_callbacks.(128) in
            match extract_cc_value msg with
            | _chan, d1, d2 -> cb d2 d1
            (* | _ -> () *));
          loop ()
      in
      loop ()
    | Error err ->
      printf
        "device %d failed to open for input: %s\n"
        device_id
        (Option.value ~default:"null" @@ Portmidi.get_error_text err)
  ;;

  let start_midi () = Thread.create midi_listener ()
  let register_cc_callback ?(cc = 128) func = cc_chan_callbacks.(cc) <- func
end
