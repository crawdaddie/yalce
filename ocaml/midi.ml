open Portmidi

type t = {
  mutex: Mutex.t;
  handlers : (int32 -> int32 -> int32 -> unit ) list ref;
  dbg: bool ref;
  name: string;
}

let log_midi_error fmt err =
  Printf.sprintf fmt (Option.value ~default:"Unknown error" (get_error_text err)) |> Stubs.write_log 

let log_midi_msg msg src_device =
  let status = message_status msg in
  let num = message_data1 msg in
  let value = message_data2 msg in
  match (Int32.logand status 0xF0l) with
    | 0xB0l -> Printf.sprintf "[%s] cc: %ld %ld %ld\n" src_device (Int32.logand status 0x0Fl) num value |> Stubs.write_log
    | 0x90l -> Printf.sprintf "[%s] note: %ld %ld %ld\n" src_device (Int32.logand status 0x0Fl) num value |> Stubs.write_log

let get_device_name device_id =
  match get_device_info device_id with
    | Some device_info -> (match device_info.name with
      | None -> Printf.sprintf "%d" device_id 
      | Some name -> name
    )
    | None -> Printf.sprintf "%d" device_id



let create device_id =
  let mutex = Mutex.create () in
  let handlers = ref [] in
  initialize () ;
  let buffer_size = Int32.of_int default_sysex_buffer_size in
  let dbg = ref false in

  let name = get_device_name device_id in 

  let t = match open_input ~device_id:device_id ~buffer_size:buffer_size with
    | Ok input_stream ->
        Printf.sprintf "opened midi stream from device [%s]\n" name |> Stubs.write_log;
      let handle_msg message  =
        let c = Int32.logand (message_status message) 0x0Fl in
        let cc = message_data1 message in
        let v = message_data2 message in

        if !dbg then log_midi_msg message name;

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
    dbg;
    name;
  }

let register midi h =
  midi.handlers := h :: !(midi.handlers)


let connect_all_sources () = 
  initialize () ;
  let num_devices = count_devices () in
  List.init num_devices (fun x -> x) |> List.map (fun id -> create id)
