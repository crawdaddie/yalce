open Yalce
open Ctypes

let kick freq dec =
  let open Synth.Make (struct
      let inputs = [ freq ]
      let layout = 1
    end) in
  let freq_env = env_autotrig [ 2000.; freq *. 1.8; freq; freq ] [ 0.01; 0.2; 1. ] in

  let osc = sin freq |> map_sig (Node.out freq_env) 0 |> tanh 1.2 in

  let noise_env = env_autotrig [ 0.; 1.; 0.; 0. ] [ 0.001; 0.07; 0.01 ] in
  let noise_burst = (noise () |> biquad_lp 2000. 1.) *~ noise_env |> mul_scalar 0.2 in

  (* let noise_burst = noise_burst |> comb 0.1 0.5 0.2 in *)
  let osc = osc +~ noise_burst in

  let env = env_autotrig [ 0.0; 1.0; 0.5; 0. ] [ 0.01; dec; 0.2 ] in

  let osc = env *~ osc in

  output osc
;;

let snare ?(freq = 300.) ?(accent = 1.) () =
  let open Synth.Make (struct
      let inputs = []
      let layout = 1
    end) in
  let resonance = noise () |> biquad_hp freq 10. |> biquad_lp 5000. 1. in
  let decay =
    resonance *~ env_autotrig [ 0.; 1.; 0.1; 0.0 ] [ 0.0001; 0.05; 0.02 ]
    |> mul_scalar accent
  in

  output decay
;;
