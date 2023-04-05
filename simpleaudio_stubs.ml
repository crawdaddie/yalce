external start_audio: unit -> int = "caml_start_audio"

external kill_audio: unit -> int = "caml_kill_audio" 

external add_node: unit -> int = "caml_add_node" 
external sq: unit -> int = "caml_sq" 
external chain_nodes: unit -> int = "caml_chain_nodes" 
external play_sine: float -> int = "caml_play_sin"

external play_sq: float -> int = "caml_play_sq"

external play_sq_detune: float -> int = "caml_play_sq_detune"
