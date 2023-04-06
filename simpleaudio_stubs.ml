external start_audio: unit -> unit = "caml_start_audio"
external oscilloscope: unit -> unit = "caml_oscilloscope"

external kill_audio: unit -> unit = "caml_kill_audio" 

external add_node: unit -> unit = "caml_add_node" 
external chain_nodes: unit -> unit = "caml_chain_nodes" 

type node 
external play_sine: float -> node = "caml_play_sin"
external set_freq: node -> float -> unit = "set_freq"
external play_pm: float -> float -> float -> node = "caml_play_pmsin"
external play_sq: float -> node = "caml_play_sq"
external play_sq_detune: float -> node = "caml_play_sq_detune"
external play_imp: float -> node = "caml_play_impulse"
external play_saw: float -> node = "caml_play_poly_saw"
external play_hoover: float -> node = "caml_play_hoover"
external play_pulse: float -> float -> node = "caml_play_pulse"

type t
external abs_get: float -> int -> char -> t = "wrapping_ptr_ml2c"
external prunit_t: t -> unit = "dump_ptr"
external free_t: t -> unit = "free_ptr"

external set_d: t -> float -> unit = "set_d"
