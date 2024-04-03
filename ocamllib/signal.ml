open Ctypes
open Foreign

type signal_struct

let signal_struct : signal_struct structure typ = structure "Signal"
let signal_buf = field signal_struct "buf" (ptr double)
let signal_size = field signal_struct "size" int
let signal_layout = field signal_struct "layout" int
let () = seal signal_struct

type signal

let signal = ptr signal_struct
let buf signal = getf !@signal signal_buf
let size signal = getf !@signal signal_size
let layout signal = getf !@signal signal_layout

let to_list signal =
  let buf_ptr = buf signal in
  let len = size signal in
  let carr = CArray.from_ptr buf_ptr len in
  CArray.to_list carr
;;

(* int read_file(const char *filename, Signal *signal, int *sf_sample_rate) { *)
let read_file = foreign "read_file" (ptr char @-> signal @-> ptr int @-> returning int)
let get_sig = foreign "get_sig" (int @-> returning signal)
let get_sig_of_soundfile = foreign "get_sig_of_soundfile" (ptr char @-> returning signal)
let buf_alloc = foreign "buf_alloc_to_sig" (ptr char @-> returning signal)

let of_sf filename =
  let filename_ptr = CArray.start (CArray.of_string filename) in
  buf_alloc filename_ptr
;;
