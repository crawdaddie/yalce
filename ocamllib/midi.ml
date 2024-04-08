open Base
open Stdio

let parse_msg status data1 data2 =
  let msg_code = Int32.bit_and status (Int32.of_int_exn 0xF0) in
  let channel = Int32.bit_and status (Int32.of_int_exn 0x0F) in
  printf "-------------\n";
  printf "debug code: %ld chan :%ld\n" msg_code (Int32.( + ) channel (Int32.of_int_exn 1));
  printf "data1: %ld\n" data1;
  printf "data2: %ld\n" data2;
  Stdio.Out_channel.flush stdout
;;

let msg_callback msg =
  parse_msg
    (Portmidi.message_status msg)
    (Portmidi.message_data1 msg)
    (Portmidi.message_data2 msg);
  ()
;;

type midi_message =
  | CC of int32 * int32 * int32
  | Note of int32 * int32 * int32

let extract_midi_message msg =
  let open Int32 in
  let status_byte = msg land 0xFFl in
  let command = status_byte land 0xF0l in
  let channel = status_byte land 0x0Fl in

  let data_byte1 = (msg lsr 16) land 0x7Fl in
  let data_byte2 = ((msg + 1l) lsr 8) land 0x7Fl in

  if command = 0x90l
  then Note (channel, data_byte2, data_byte1)
  else if command = 0x80l
  then Note (channel, data_byte2, Int32.zero)
  else CC (channel, data_byte2, data_byte1)
;;

module Listener = struct
  type cc_callback = int32 -> int32 -> unit

  let cc_chan_callbacks : cc_callback array = Array.create ~len:129 (fun _ _ -> ())
  let note_chan_callbacks : cc_callback array = Array.create ~len:129 (fun _ _ -> ())

  let midi_listener ?(debug = false) () =
    let () =
      match Portmidi.initialize () with
      | Ok () -> ()
      | Error _e -> failwith "error initializing portmidi"
    in
    let device_id = 1 in
    match Portmidi.open_input ~device_id ~buffer_size:512l with
    | Ok stream ->
      printf "device %d successfully opened for input!\n" device_id;
      Stdio.Out_channel.flush stdout;
      let rec loop () =
        match Portmidi.read_input ~length:10 stream with
        | Error err ->
          failwith
          @@ Printf.sprintf
               "failed to read 1: %s"
               (Portmidi.get_error_text err |> Option.value ~default:"null")
        | Ok [] -> loop ()
        | Ok lst ->
          List.iter lst ~f:(fun pme ->
            let msg = pme.Portmidi.Portmidi_event.message in
            if debug then msg_callback msg;

            match extract_midi_message msg with
            | CC (_channel, cc_num, cc_val) ->
              let cb = cc_chan_callbacks.(128) in
              cb cc_num cc_val
            | Note (_channel, note, velocity) when Int32.compare velocity Int32.zero > 0
              ->
              let cb = note_chan_callbacks.(128) in
              cb note velocity
            | Note (_channel, _note, _velocity) -> ());
          loop ()
      in
      loop ()
    | Error err ->
      printf
        "device %d failed to open for input: %s\n"
        device_id
        (Option.value ~default:"null" @@ Portmidi.get_error_text err)
  ;;

  let start_midi ?(debug = false) () = Thread.create (midi_listener ~debug) ()
  let register_cc_callback ?(cc = 128) func = cc_chan_callbacks.(cc) <- func
  let register_noteon_callback ?(cc = 128) func = note_chan_callbacks.(cc) <- func
end
