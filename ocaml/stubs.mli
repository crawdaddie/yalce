type node_ptr
type signal
external start_audio : unit -> unit = "caml_start_audio"
external oscilloscope : unit -> unit = "caml_oscilloscope"
external kill_audio : unit -> unit = "caml_kill_audio"
external add_node : unit -> unit = "caml_add_node"
external set_sig_float : node_ptr -> int -> float -> unit
  = "caml_set_sig_float"
external set_sig_node : node_ptr -> int -> node_ptr -> unit
  = "caml_set_sig_node"
external caml_set_mul_float : node_ptr -> float -> unit = "caml_set_mul"
external caml_set_mul_node : node_ptr -> node_ptr -> unit
  = "caml_set_mul_node"
external caml_set_add_float : node_ptr -> float -> unit = "caml_set_add"
external caml_set_add_node : node_ptr -> node_ptr -> unit
  = "caml_set_add_node"
external caml_chain_nodes : node_ptr -> node_ptr -> int -> node_ptr
  = "caml_chain_nodes"
external stop : node_ptr -> unit = "caml_kill_node"
external dump_nodes : unit -> unit = "caml_dump_nodes"
external osc_sin : float -> node_ptr = "caml_sin"
external play_node : node_ptr -> node_ptr = "caml_play_node"
external set_freq : node_ptr -> float -> unit = "caml_set_freq"
external osc_sq : float -> node_ptr = "caml_sq"
external osc_impulse : float -> node_ptr = "caml_impulse"
external osc_sq_detune : float -> node_ptr = "caml_sq_detune"
external osc_saw : float -> node_ptr = "caml_poly_saw"
external osc_hoover : float -> node_ptr = "caml_hoover"
external osc_pulse : float -> float -> node_ptr = "caml_pulse"
external fx_delay : float -> float -> node_ptr -> node_ptr
  = "caml_simple_delay"
external lpf : float -> float -> node_ptr -> node_ptr = "caml_biquad_lpf"
external load_sndfile : string -> signal = "caml_load_soundfile"
external dump_sndfile : signal -> unit = "dump_soundfile_data"
external bufplayer : float -> signal -> node_ptr = "caml_bufplayer"
external bufplayer_timestretch :
  float -> float -> float -> signal -> node_ptr
  = "caml_bufplayer_timestretch"
external register_midi_handler_name : int -> int -> string -> unit
  = "caml_register_midi_handler"
external write_log : string -> unit = "caml_write_log"
