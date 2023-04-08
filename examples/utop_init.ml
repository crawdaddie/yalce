open Simpleaudio_stubs;;
open Unix;;

let fd = Unix.openfile "mylog.txt" [Unix.O_WRONLY; Unix.O_APPEND; Unix.O_CREAT] 0o644;;

let log_message fd msg = 
  let out_channel = Unix.out_channel_of_descr fd in
  Printf.fprintf out_channel "%s\n" msg;
  flush out_channel;;

start_audio () ;;
