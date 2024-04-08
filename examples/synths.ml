open Yalce

let acid_mono freq dec =
  let open Synth.Make (struct
      let inputs =
        [ freq (* osc freq *)
        ; 150. (* filter freq *)
        ; 0. (* trig *)
        ; 10. (* filter res*)
        ]
      ;;

      let layout = 1
    end) in
  let freq = lag_sig 0.03 (in_sig 0) in
  let trig_sig = in_sig 2 in
  let osc_src = sq in

  let sq1 = osc_src 100. |> map_sig (Node.out freq) 0 in
  let freq2_sig = Node.out (mul_scalar 1.01 freq) in
  let sq2 = osc_src 100. |> map_sig freq2_sig 0 in
  let s = sq1 +~ sq2 in

  let filt_env =
    env_trig [ 0.; 1.; 0.2; 0. ] [ 0.001; 0.1; 0.5 ]
    |> pipe_sig_to_idx 0 trig_sig
    |> scale (in_sig 1) (mul_sig 5. (in_sig 1))
  in

  let filt =
    biquad_lp 100. 10. s |> map_sig (Node.out filt_env) 1 |> map_sig (in_sig 3) 2
    (* |> tanh 0.8 *)
  in

  let out =
    env_trig [ 0.; 1.; 0.5; 0. ] [ 0.01; 0.2; 0.5 ] |> map_sig trig_sig 0 |> ( *~ ) filt
    (* |> comb (0.5 *. 60. /. 145.) 1. 0.5 *)
  in

  output out
;;
