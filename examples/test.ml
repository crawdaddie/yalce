open Yalce
open Stdio;;

let cc_cb cc v = Printf.printf "cc: %ld v: %ld\n" cc v in

let _midi_thread = Midi.Listener.start_midi () in
Midi.Listener.register_cc_callback cc_cb;

while true do
  Thread.delay 1.
done
