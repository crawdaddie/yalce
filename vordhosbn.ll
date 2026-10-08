loaded libs/audio_jit/libaudio_jit.so
# ylc-mir
fn $top()[2m -> Ptr @owned[0m {
  bb0 entry:
    %0 = fn_ref $$DSP_module_init
    %1 = call %0()[2m : ()[0m[2m ; ops [%0:callee/borrow#0][0m
    %2 = const.void
    %3 = fn_ref $$Math_module_init
    %4 = call %3()[2m : ()[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = const.void
    %6 = fn_ref $$Arrays_module_init
    %7 = call %6()[2m : ()[0m[2m ; ops [%6:callee/borrow#0][0m
    %8 = const.void
    %9 = fn_ref $$SF_module_init
    %10 = call %9()[2m : ()[0m[2m ; ops [%9:callee/borrow#0][0m
    %11 = const.void
    %12 = fn_ref $Arrays.map
    %13 = fn_ref $SF.load_sf
    %14 = const.string "~/Sounds/rns_samples/HiHats/Closed/dh_clhat_tight_synther.flac"
    %15 = const.string "~/Sounds/rns_samples/Kicks/ech_kick_resample.flac"
    %16 = const.string "~/Sounds/rns_samples/Snares/dh_snr_hi_grainy.flac"
    %17 = const.string "~/Sounds/rns_samples/Claps/ech_clap_vinyl909.flac"
    %18 = construct.array_literal { %14, %15, %16, %17 }[2m : Array of String[0m[2m ; ea stack#0[0m[2m ; ops [%14:element/consume#0, %15:element/consume#1, %16:element/consume#2, %17:element/consume#3][0m
    %19 = call %12 as $Arrays_map.Fn_String_Char_SF_Array_Double_Int_Int.Array_String_Char.Array_SF_Array_Double_Int_Int(%13, %18)[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%12:callee/borrow#0, %13:value/borrow#0, %18:value/borrow#1][0m
    %20 = global_store @$global.bufs, %19[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%19:value/consume#0][0m
    %21 = fn_ref $kd.cons
    %22 = fn_ref $snr.cons
    %23 = fn_ref $hh.cons
    %24 = fn_ref $hh2.cons
    %25 = fn_ref $cymbal.cons
    %26 = fn_ref $mels.cons
    %27 = fn_ref $play_node
    %28 = const.double 0
    %29 = fn_ref $cathedral.cons
    %30 = call %29(%28)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %28:value/borrow#0][0m
    %31 = call %27(%30)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %30:value/borrow#0][0m
    %32 = global_store @$global.r, %31[2m : Ptr[0m[2m ; ops [%31:value/consume#0][0m
    %33 = const.double 60
    %34 = const.double 90
    %35 = fdiv %33, %34[2m ; ops [%33:value/borrow#0, %34:value/borrow#1][0m
    %36 = global_store @$global.beat, %35[2m : Double[0m[2m ; ops [%35:value/consume#0][0m
    %37 = fn_ref $note
    %38 = fn_ref $mel
    %39 = const.double 47
    %40 = const.double 62
    %41 = const.double 59
    %42 = const.double 62
    %43 = const.double 47
    %44 = const.double 62
    %45 = const.double 59
    %46 = const.double 62
    %47 = const.double 49
    %48 = const.double 61
    %49 = const.double 49
    %50 = const.double 61
    %51 = const.double 49
    %52 = const.double 61
    %53 = const.double 53
    %54 = const.double 54
    %55 = construct.array_literal { %39, %40, %41, %42, %43, %44, %45, %46, %47, %48, %49, %50, %51, %52, %53, %54 }[2m : Array of Double[0m[2m ; ea heap#1[0m[2m ; ops [%39:element/consume#0, %40:element/consume#1, %41:element/consume#2, %42:element/consume#3, %43:element/consume#4, %44:element/consume#5, %45:element/consume#6, %46:element/consume#7, %47:element/consume#8, %48:element/consume#9, %49:element/consume#10, %50:element/consume#11, %51:element/consume#12, %52:element/consume#13, %53:element/consume#14, %54:element/consume#15][0m
    %56 = global_store @$global.notes, %55[2m : Array of Double[0m[2m ; ops [%55:value/consume#0][0m
    %57 = fn_ref $m
    %58 = fn_ref $m2
    %59 = fn_ref $m3
    %60 = fn_ref $c
    %61 = fn_ref $hhc
    %62 = const.double 3.5
    %63 = const.double 3.25
    %64 = const.double 1.75
    %65 = const.double 0.75
    %66 = const.double 5.5
    %67 = const.double 1.25
    %68 = construct.array_literal { %62, %63, %64, %65, %66, %67 }[2m : Array of Double[0m[2m ; ea heap#2[0m[2m ; ops [%62:element/consume#0, %63:element/consume#1, %64:element/consume#2, %65:element/consume#3, %66:element/consume#4, %67:element/consume#5][0m
    %69 = global_store @$global.kick_durs, %68[2m : Array of Double[0m[2m ; ops [%68:value/consume#0][0m
    %70 = fn_ref $stutter.cons
    %71 = fn_ref $fx.cons
    %72 = fn_ref $play_node
    %73 = const.double 9
    %74 = const.double 0
    %75 = const.double 1
    %76 = const.double 60
    %77 = const.double 180
    %78 = fdiv %76, %77[2m ; ops [%76:value/borrow#0, %77:value/borrow#1][0m
    %79 = const.double 1
    %80 = const.double 0
    %81 = fn_ref $fx.cons
    %82 = call %81(%73, %74, %75, %78, %79, %80)[2m : Ptr[0m[2m ; ops [%81:callee/borrow#0, %73:value/borrow#0, %74:value/borrow#1, %75:value/borrow#2, %78:value/borrow#3, %79:value/borrow#4, %80:value/borrow#5][0m
    %83 = call %72(%82)[2m : Ptr[0m[2m ; ops [%72:callee/borrow#0, %82:value/borrow#0][0m
    %84 = global_store @$global.fx_node, %83[2m : Ptr[0m[2m ; ops [%83:value/consume#0][0m
    %85 = fn_ref $set_fx_rand
    %86 = fn_ref $kick_routine
    %87 = fn_ref $snr_routine
    %88 = fn_ref $hh_routine
    %89 = fn_ref $drums
    %90 = fn_ref $fx_set_routine
    %91 = fn_ref $g
    %92 = fn_ref $g
    %93 = coro.new %92()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#3[0m[2m ; ops [%92:callee/borrow#0][0m
    %94 = fn_ref $ylc_play_pattern_start
    %95 = fn_ref $__ylc_play_pattern_step_19
    %96 = call %94(%35, %95, %93)[2m : Ptr[0m[2m ; ops [%94:callee/borrow#0, %35:value/borrow#0, %95:value/borrow#1, %93:value/consume#2][0m
    return %96[2m ; ops [%96:return/consume#0][0m
}
fn $DSP_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = const.void
    %1 = fn_ref $$DSP.DSP_common_module_init
    %2 = call %1()[2m : ()[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = const.void
    %4 = fn_ref $init_audio
    %5 = call %4()[2m : ()[0m[2m ; ops [%4:callee/borrow#0][0m
    %6 = fn_ref $play_pattern
    %7 = fn_ref $cancel_task
    %8 = fn_ref $play_node
    %9 = fn_ref $play_into
    %10 = fn_ref $DSP.~>
    %11 = fn_ref $set_input_scalar
    %12 = fn_ref $set_input_trig
    %13 = fn_ref $record_start
    %14 = fn_ref $record_stop
    %15 = const.void
    return %15[2m ; ops [%15:return/consume#0][0m
}
fn $DSP.DSP_common_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $Audio
    %1 = fn_ref $~
    %2 = fn_ref $sin_osc
    %3 = fn_ref $zero_xing_freq
    %4 = fn_ref $sq_osc
    %5 = fn_ref $saw_osc
    %6 = fn_ref $pm_osc
    %7 = fn_ref $phasor
    %8 = fn_ref $phasor_sinc
    %9 = fn_ref $sweep
    %10 = fn_ref $freq_shift
    %11 = fn_ref $rand_trig
    %12 = fn_ref $fold2
    %13 = fn_ref $trig
    %14 = fn_ref $trig_sinc
    %15 = fn_ref $tri_osc.cons
    %16 = fn_ref $changed
    %17 = fn_ref $change_up
    %18 = fn_ref $tabread
    %19 = fn_ref $tabread_samp
    %20 = fn_ref $decay
    %21 = fn_ref $scale
    %22 = fn_ref $scale_bp
    %23 = fn_ref $array_of_buf
    %24 = fn_ref $bufsize
    %25 = fn_ref $lfnoise
    %26 = fn_ref $lfnoise0
    %27 = fn_ref $kill_on_end
    %28 = fn_ref $adsr
    %29 = fn_ref $adsr_array
    %30 = fn_ref $array_env
    %31 = fn_ref $rect
    %32 = fn_ref $delay_line
    %33 = fn_ref $delay
    %34 = fn_ref $comb
    %35 = fn_ref $dl_allpass
    %36 = fn_ref $pitchshift
    %37 = fn_ref $hasher
    %38 = fn_ref $mixadd
    %39 = fn_ref $feedback_net
    %40 = fn_ref $delay_proc
    %41 = fn_ref $disperser
    %42 = fn_ref $glue
    %43 = fn_ref $bufplay
    %44 = fn_ref $mbufplay
    %45 = fn_ref $grains
    %46 = fn_ref $grains_env
    %47 = fn_ref $lag
    %48 = fn_ref $arr_choose
    %49 = fn_ref $arr_seq
    %50 = fn_ref $sah
    %51 = fn_ref $mix
    %52 = fn_ref $splay
    %53 = fn_ref $sum.cons
    %54 = fn_ref $pan
    %55 = fn_ref $to_samps
    %56 = fn_ref $$DSP.DSP_common.dsp_iter_module_init
    %57 = call %56()[2m : ()[0m[2m ; ops [%56:callee/borrow#0][0m
    %58 = const.void
    %59 = fn_ref $$DSP.DSP_common.Math_module_init
    %60 = call %59()[2m : ()[0m[2m ; ops [%59:callee/borrow#0][0m
    %61 = const.void
    %62 = fn_ref $$DSP.DSP_common.Filter_module_init
    %63 = call %62()[2m : ()[0m[2m ; ops [%62:callee/borrow#0][0m
    %64 = const.void
    %65 = fn_ref $white.cons
    %66 = fn_ref $pink.cons
    %67 = fn_ref $$DSP.DSP_common.Reverb_module_init
    %68 = call %67()[2m : ()[0m[2m ; ops [%67:callee/borrow#0][0m
    %69 = const.void
    %70 = fn_ref $dust.cons
    %71 = fn_ref $int32_parse
    %72 = fn_ref $DSP.DSP_common.parse_note
    %73 = fn_ref $DSP.DSP_common.midi_freq
    %74 = fn_ref $DSP.DSP_common.midiratio
    %75 = fn_ref $xline.cons
    %76 = const.void
    return %76[2m ; ops [%76:return/consume#0][0m
}
extern fn Audio(%0 arg0[2m: T @borrow[0m)[2m -> T @owned[0m;
extern fn ~(%0 arg0[2m: Array of T @borrow[0m)[2m -> T @owned[0m;
extern fn sin_osc(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn zero_xing_freq(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn sq_osc(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn saw_osc(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn pm_osc(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn phasor(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn phasor_sinc(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn sweep(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn freq_shift(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn rand_trig(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn fold2(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn trig(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn trig_sinc(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn tri_osc.cons(%0 freq[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $tri_osc.frame
    %3 = const.string "tri_osc"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 8
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $tri_osc.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $tri_osc.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn tri_osc.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn tri_osc.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 freq[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 1
    %6 = const.double 4
    %7 = fn_ref $ylc_audio_phasor_kernel
    %8 = const.int 0
    %9 = ptr_offset %1, %8[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %8:index/borrow#1][0m
    %10 = primitive_cast %9 from Ptr of Char to Ptr of Double[2m ; ops [%9:value/borrow#0][0m
    %11 = call %7(%10, %3, %4)[2m : Double[0m[2m ; ops [%7:callee/borrow#0, %10:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2][0m
    %12 = fmul %6, %11[2m ; ops [%6:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_fold2_kernel
    %14 = primitive_cast %1 from Ptr of Char to Ptr of Double[2m ; ops [%1:value/borrow#0][0m
    %15 = call %13(%14, %3, %5, %12)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %14:value/borrow#0, %3:value/borrow#1, %5:value/borrow#2, %12:value/borrow#3][0m
    return %15[2m ; ops [%15:return/consume#0][0m
}
fn tri_osc.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $tri_osc.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
extern fn ylc_audio_phasor_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_fold2_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_read_inlet_node_lane_i32(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: Int @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_get_output_buf(%0 arg0[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m;
extern fn ylc_create_audio_frame_node(%0 arg0[2m: (Ptr -> (Ptr -> (Ptr -> (Int -> (Double -> ()))))) @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Int @borrow[0m, %4 arg4[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m;
extern fn ylc_audio_node_set_state_init(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Ptr @borrow[0m)[2m -> () @none[0m;
extern fn ylc_audio_node_inline_state(%0 arg0[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m;
extern fn node_connect_input(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Ptr @borrow[0m, %2 arg2[2m: Ptr @borrow[0m)[2m -> () @none[0m;
extern fn const_sig(%0 arg0[2m: Double @borrow[0m)[2m -> Ptr @owned[0m;
extern fn changed(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn change_up(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn tabread(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn tabread_samp(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn decay(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn scale(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn scale_bp(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn array_of_buf(%0 arg0[2m: Ptr @borrow[0m)[2m -> Array of Double @owned[0m;
extern fn bufsize(%0 arg0[2m: Ptr @borrow[0m)[2m -> Int @owned[0m;
extern fn lfnoise(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn lfnoise0(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn kill_on_end(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn adsr(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn adsr_array(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn array_env(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn rect(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn delay_line(%0 arg0[2m: Double @borrow[0m)[2m -> Array of Double @owned[0m;
extern fn delay(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn comb(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn dl_allpass(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn pitchshift(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn hasher(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn mixadd(%0 arg0[2m: T @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn feedback_net(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m, %2 arg2[2m: Array of Double @borrow[0m, %3 arg3[2m: (Double -> Double) @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn delay_proc(%0 arg0[2m: (Int -> (Array of Double -> (Double -> Double))) @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn disperser(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn glue(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn bufplay(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn mbufplay(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn grains(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn grains_env(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m, %2 arg2[2m: Array of Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn lag(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn arr_choose(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn arr_seq(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn sah(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn mix(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn splay(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn sum.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $sum.frame
    %3 = const.string "sum"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 0
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $sum.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $sum.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/consume#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn sum.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn sum.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 1
    return %4[2m ; ops [%4:return/consume#0][0m
}
fn sum.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $sum.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/consume#1, %3:value/consume#2, %4:value/consume#3, %10:value/consume#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
extern fn pan(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn to_samps(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn $DSP.DSP_common.dsp_iter_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $int_fold
    %1 = fn_ref $int_foldi
    %2 = fn_ref $list_map
    %3 = fn_ref $list_fold
    %4 = fn_ref $list_foldi
    %5 = fn_ref $array_map
    %6 = fn_ref $array_mapi
    %7 = fn_ref $array_fold
    %8 = fn_ref $array_foldi
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
extern fn int_fold(%0 arg0[2m: (Int -> (R -> R)) @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
extern fn int_foldi(%0 arg0[2m: (Int -> (R -> R)) @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
extern fn list_map(%0 arg0[2m: (T -> R) @borrow[0m, %1 arg1[2m: List
  | []
  | :: of T, List
 @borrow[0m)[2m -> List
  | []
  | :: of R, List
 @owned[0m;
extern fn list_fold(%0 arg0[2m: (R -> (T -> R)) @borrow[0m, %1 arg1[2m: List
  | []
  | :: of T, List
 @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
extern fn list_foldi(%0 arg0[2m: (Int -> (R -> (T -> R))) @borrow[0m, %1 arg1[2m: List
  | []
  | :: of T, List
 @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
extern fn array_map(%0 arg0[2m: (T -> R) @borrow[0m, %1 arg1[2m: Array of T @borrow[0m)[2m -> Array of R @owned[0m;
extern fn array_mapi(%0 arg0[2m: (Int -> (T -> R)) @borrow[0m, %1 arg1[2m: Array of T @borrow[0m)[2m -> Array of R @owned[0m;
extern fn array_fold(%0 arg0[2m: (R -> (T -> R)) @borrow[0m, %1 arg1[2m: Array of T @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
extern fn array_foldi(%0 arg0[2m: (Int -> (R -> (T -> R))) @borrow[0m, %1 arg1[2m: Array of T @borrow[0m, %2 arg2[2m: R @borrow[0m)[2m -> R @owned[0m;
fn $DSP.DSP_common.Math_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = const.int 2147483647
    %1 = global_store @$global.DSP.DSP_common.Math.MAX_INT, %0[2m : Int[0m[2m ; ops [%0:value/consume#0][0m
    %2 = const.double 3.14159
    %3 = global_store @$global.DSP.DSP_common.Math.PI, %2[2m : Double[0m[2m ; ops [%2:value/consume#0][0m
    %4 = const.double 1e-12
    %5 = global_store @$global.DSP.DSP_common.Math.epsilon, %4[2m : Double[0m[2m ; ops [%4:value/consume#0][0m
    %6 = fn_ref $rand_int
    %7 = fn_ref $rand_double
    %8 = fn_ref $rand_double_range
    %9 = fn_ref $rand_double_range
    %10 = global_store @$global.DSP.DSP_common.Math.rrand, %9[2m : (Double -> (Double -> Double))[0m[2m ; ops [%9:value/consume#0][0m
    %11 = fn_ref $amp_db
    %12 = fn_ref $db_amp
    %13 = fn_ref $sin
    %14 = fn_ref $cos
    %15 = fn_ref $bipolar_scale
    %16 = fn_ref $unipolar_scale
    %17 = fn_ref $double_array_init
    %18 = fn_ref $double_array
    %19 = fn_ref $pow
    %20 = fn_ref $tanh
    %21 = fn_ref $tan
    %22 = fn_ref $atan
    %23 = fn_ref $atan2
    %24 = fn_ref $ilog2
    %25 = fn_ref $log
    %26 = fn_ref $u64pow
    %27 = fn_ref $sqrt
    %28 = fn_ref $floor
    %29 = fn_ref $round
    %30 = fn_ref $ceil
    %31 = fn_ref $DSP.DSP_common.Math.max
    %32 = fn_ref $DSP.DSP_common.Math.min
    %33 = fn_ref $DSP.DSP_common.Math.clamp
    %34 = fn_ref $DSP.DSP_common.Math.abs
    %35 = fn_ref $DSP.DSP_common.Math.neg
    %36 = fn_ref $DSP.DSP_common.Math.normalize
    %37 = fn_ref $DSP.DSP_common.Math.chance
    %38 = fn_ref $DSP.DSP_common.Math.chance2
    %39 = fn_ref $DSP.DSP_common.Math.shuffle
    %40 = fn_ref $DSP.DSP_common.Math.shuffle_ip
    %41 = fn_ref $DSP.DSP_common.Math.array_choose
    %42 = fn_ref $nonzero_randu_double
    %43 = fn_ref $log
    %44 = fn_ref $exp
    %45 = fn_ref $_randn_pair
    %46 = fn_ref $DSP.DSP_common.Math.randn
    %47 = fn_ref $DSP.DSP_common.Math.ilog10
    %48 = fn_ref $ipow
    %49 = const.void
    return %49[2m ; ops [%49:return/consume#0][0m
}
extern fn rand_int(%0 arg0[2m: Int @borrow[0m)[2m -> Int @owned[0m;
extern fn rand_double()[2m -> Double @owned[0m;
extern fn rand_double_range(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn amp_db(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn db_amp(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn sin(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn cos(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn bipolar_scale(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn unipolar_scale(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn double_array_init(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Ptr @owned[0m;
extern fn double_array(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Array of Double @owned[0m;
extern fn pow(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn tanh(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn tan(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn atan(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn atan2(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ilog2(%0 arg0[2m: Int @borrow[0m)[2m -> Int @owned[0m;
extern fn log(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn u64pow(%0 arg0[2m: Uint64 @borrow[0m, %1 arg1[2m: Uint64 @borrow[0m)[2m -> Uint64 @owned[0m;
extern fn sqrt(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn floor(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn round(%0 arg0[2m: Double @borrow[0m)[2m -> Int @owned[0m;
extern fn ceil(%0 arg0[2m: Double @borrow[0m)[2m -> Int @owned[0m;
fn DSP.DSP_common.Math.max(%0 a[2m: `121 @consume[0m, %1 b[2m: `121 @consume[0m)[2m -> `121 @owned[0m {
  bb0 entry:
    %2 = call @>(%0, %1)[2m : Bool[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %5 = phi [bb4: %0, bb6: %1][2m : `121[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn DSP.DSP_common.Math.min(%0 a[2m: `128 @consume[0m, %1 b[2m: `128 @consume[0m)[2m -> `128 @owned[0m {
  bb0 entry:
    %2 = call @<(%0, %1)[2m : Bool[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %5 = phi [bb4: %0, bb6: %1][2m : `128[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn DSP.DSP_common.Math.clamp(%0 mn[2m: `136 @consume[0m, %1 mx[2m: `136 @consume[0m, %2 v[2m: `136 @consume[0m)[2m -> `136 @owned[0m {
  bb0 entry:
    %3 = fn_ref $DSP.DSP_common.Math.max
    %4 = fn_ref $DSP.DSP_common.Math.min
    %5 = call %4(%1, %2)[2m : `136[0m[2m ; ops [%4:callee/borrow#0, %1:value/consume#0, %2:value/consume#1][0m
    %6 = call %3(%0, %5)[2m : `136[0m[2m ; ops [%3:callee/borrow#0, %0:value/consume#0, %5:value/consume#1][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn DSP.DSP_common.Math.abs(%0 a[2m: Int @consume[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = igt %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %7 = phi [bb4: %0, bb6: %6][2m : Int[0m[2m ; ops [%0:value/borrow#0, %6:value/borrow#1][0m
    return %7[2m ; ops [%7:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %5 = const.int -1
    %6 = imul %0, %5[2m ; ops [%0:value/borrow#0, %5:value/borrow#1][0m
    br bb2
}
fn DSP.DSP_common.Math.neg(%0 a[2m: `150 @borrow[0m)[2m -> `151 @owned[0m {
  bb0 entry:
    %1 = const.int -1
    %2 = call @*(%1, %0)[2m : `151[0m[2m ; ops [_:callee/borrow#0, %1:value/borrow#0, %0:value/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn DSP.DSP_common.Math.normalize(%0 mn[2m: `160 @borrow[0m, %1 mx[2m: `162 @borrow[0m, %2 v[2m: `159 @borrow[0m)[2m -> `158 @owned[0m {
  bb0 entry:
    %3 = call @-(%2, %0)[2m : `156[0m[2m ; ops [_:callee/borrow#0, %2:value/borrow#0, %0:value/borrow#1][0m
    %4 = call @-(%1, %0)[2m : `157[0m[2m ; ops [_:callee/borrow#0, %1:value/borrow#0, %0:value/borrow#1][0m
    %5 = call @/(%3, %4)[2m : `158[0m[2m ; ops [_:callee/borrow#0, %3:value/borrow#0, %4:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn DSP.DSP_common.Math.chance(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> `172) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %2 = fn_ref $rand_double
    %3 = call %2()[2m : Double[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = flte %3, %0[2m ; ops [%3:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %10 = const.void
    return %10[2m ; ops [%10:return/consume#0][0m
  bb3 match.arm.0.test:
    %5 = const.bool true
    %6 = beq %4, %5[2m ; ops [%4:value/borrow#0, %5:value/borrow#1][0m
    cond %6, bb4, bb5[2m ; ops [%6:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %7 = call %1()[2m : `172[0m[2m ; ops [%1:callee/borrow#0][0m
    %8 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %9 = const.void
    br bb2
}
fn DSP.DSP_common.Math.chance2(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> `181) @borrow[0m, %2 g[2m: (() -> `182) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %3 = fn_ref $rand_double
    %4 = call %3()[2m : Double[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = flte %4, %0[2m ; ops [%4:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
  bb3 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb4, bb5[2m ; ops [%7:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %8 = call %1()[2m : `181[0m[2m ; ops [%1:callee/borrow#0][0m
    %9 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %10 = call %2()[2m : `182[0m[2m ; ops [%2:callee/borrow#0][0m
    %11 = const.void
    br bb2
}
fn DSP.DSP_common.Math.shuffle(%0 n[2m: Int @borrow[0m)[2m -> Array of Int @owned[0m {
  bb0 entry:
    %1 = fn_ref $DSP.DSP_common.Math.shuffle.<anonymous>
    %2 = construct.array_fill %0, %1[2m : Array of Int[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%0:value/borrow#0, %1:function/borrow#1][0m
    %3 = const.int 0
    br bb1
  bb1 loop.cond:
    %4 = phi [bb0: %3, bb3: %26][2m : Int[0m[2m ; ops [%3:value/borrow#0, %26:value/borrow#1][0m
    %5 = ult %4, %0[2m ; ops [%4:value/borrow#0, %0:value/borrow#1][0m
    cond %5, bb2, bb4[2m ; ops [%5:condition/borrow#0][0m
  bb2 loop.body:
    %6 = const.int 1
    %7 = isub %0, %6[2m ; ops [%0:value/borrow#0, %6:value/borrow#1][0m
    %8 = isub %7, %4[2m ; ops [%7:value/borrow#0, %4:value/borrow#1][0m
    %9 = fn_ref $rand_int
    %10 = const.int 1
    %11 = iadd %8, %10[2m ; ops [%8:value/borrow#0, %10:value/borrow#1][0m
    %12 = call %9(%11)[2m : Int[0m[2m ; ops [%9:callee/borrow#0, %11:value/borrow#0][0m
    %13 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %14 = ptr_offset %13, %8[2m : Ptr[0m[2m ; ops [%13:value/borrow#0, %8:index/borrow#1][0m
    %15 = load %14[2m : Int[0m[2m ; ops [%14:value/borrow#0][0m
    %16 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %17 = ptr_offset %16, %12[2m : Ptr[0m[2m ; ops [%16:value/borrow#0, %12:index/borrow#1][0m
    %18 = load %17[2m : Int[0m[2m ; ops [%17:value/borrow#0][0m
    %19 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %20 = ptr_offset %19, %8[2m : Ptr[0m[2m ; ops [%19:value/borrow#0, %8:index/borrow#1][0m
[35m    %28 = dup %18[2m : ()[0m[2m ; ops [%18:value/borrow#0][0m[0m
    %21 = store %20, %18[2m : ()[0m[2m ; ops [%20:value/borrow#0, %18:element/consume#1][0m
    %22 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %23 = ptr_offset %22, %12[2m : Ptr[0m[2m ; ops [%22:value/borrow#0, %12:index/borrow#1][0m
[35m    %29 = dup %15[2m : ()[0m[2m ; ops [%15:value/borrow#0][0m[0m
    %24 = store %23, %15[2m : ()[0m[2m ; ops [%23:value/borrow#0, %15:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %25 = const.int 1
    %26 = iadd %4, %25[2m ; ops [%4:value/borrow#0, %25:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %27 = const.void
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn DSP.DSP_common.Math.shuffle.<anonymous>(%0 i[2m: Int @consume[0m)[2m -> Int @owned[0m {
  bb0 entry:
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn DSP.DSP_common.Math.shuffle_ip(%0 arr[2m: Array of `214 @consume[0m)[2m -> Array of `214 @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = const.int 0
    br bb1
  bb1 loop.cond:
    %3 = phi [bb0: %2, bb3: %25][2m : Int[0m[2m ; ops [%2:value/borrow#0, %25:value/borrow#1][0m
    %4 = ult %3, %1[2m ; ops [%3:value/borrow#0, %1:value/borrow#1][0m
    cond %4, bb2, bb4[2m ; ops [%4:condition/borrow#0][0m
  bb2 loop.body:
    %5 = const.int 1
    %6 = isub %1, %5[2m ; ops [%1:value/borrow#0, %5:value/borrow#1][0m
    %7 = isub %6, %3[2m ; ops [%6:value/borrow#0, %3:value/borrow#1][0m
    %8 = fn_ref $rand_int
    %9 = const.int 1
    %10 = iadd %7, %9[2m ; ops [%7:value/borrow#0, %9:value/borrow#1][0m
    %11 = call %8(%10)[2m : Int[0m[2m ; ops [%8:callee/borrow#0, %10:value/borrow#0][0m
    %12 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %13 = ptr_offset %12, %7[2m : Ptr[0m[2m ; ops [%12:value/borrow#0, %7:index/borrow#1][0m
    %14 = load %13[2m : `214[0m[2m ; ops [%13:value/borrow#0][0m
    %15 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %16 = ptr_offset %15, %11[2m : Ptr[0m[2m ; ops [%15:value/borrow#0, %11:index/borrow#1][0m
    %17 = load %16[2m : `214[0m[2m ; ops [%16:value/borrow#0][0m
    %18 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %19 = ptr_offset %18, %7[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %7:index/borrow#1][0m
[35m    %27 = dup %17[2m : ()[0m[2m ; ops [%17:value/borrow#0][0m[0m
    %20 = store %19, %17[2m : ()[0m[2m ; ops [%19:value/borrow#0, %17:element/consume#1][0m
    %21 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %22 = ptr_offset %21, %11[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %11:index/borrow#1][0m
[35m    %28 = dup %14[2m : ()[0m[2m ; ops [%14:value/borrow#0][0m[0m
    %23 = store %22, %14[2m : ()[0m[2m ; ops [%22:value/borrow#0, %14:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %24 = const.int 1
    %25 = iadd %3, %24[2m ; ops [%3:value/borrow#0, %24:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %26 = const.void
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn DSP.DSP_common.Math.array_choose(%0 arr[2m: Array of `221 @borrow[0m)[2m -> `221 @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = fn_ref $rand_int
    %3 = call %2(%1)[2m : Int[0m[2m ; ops [%2:callee/borrow#0, %1:value/borrow#0][0m
    %4 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : `221[0m[2m ; ops [%5:value/borrow#0][0m
[35m    %7 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
extern fn nonzero_randu_double()[2m -> Double @owned[0m;
extern fn exp(%0 arg0[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn _randn_pair(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> (Double, Double, ) @owned[0m;
fn DSP.DSP_common.Math.randn(%0 mu[2m: Double @borrow[0m, %1 sig[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fn_ref $_randn_pair
    %3 = call %2(%0, %1)[2m : (Double, Double, )[0m[2m ; ops [%2:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    %4 = extract.field %3, 0[2m : Double[0m[2m ; ops [%3:container/borrow#0][0m
    %5 = extract.field %3, 1[2m : Double[0m[2m ; ops [%3:container/borrow#0][0m
    return %4[2m ; ops [%4:return/consume#0][0m
}
fn DSP.DSP_common.Math.ilog10(%0 a[2m: Int @borrow[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %1 = fn_ref $DSP.DSP_common.Math.ilog10.aux
    %2 = const.int 0
    %3 = call %1 as $DSP_DSP_common_Math_ilog10_aux.Int.Int.Int(%2, %0)[2m : Int[0m[2m ; ops [%1:callee/borrow#0, %2:value/consume#0, %0:value/borrow#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
}
fn DSP.DSP_common.Math.ilog10.aux(%0 d[2m: `239 @consume[0m, %1 x[2m: Int @borrow[0m)[2m -> `239 @owned[0m {
  bb0 entry:
    %2 = const.int 10
    %3 = idiv %1, %2[2m ; ops [%1:value/borrow#0, %2:value/borrow#1][0m
    %4 = const.int 0
    %5 = igt %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb3, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb3 match.arm.0.body:
    %8 = fn_ref $DSP.DSP_common.Math.ilog10.aux
    %9 = const.int 1
    %10 = call @+(%0, %9)[2m : `239[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %9:value/borrow#1][0m
    tail_call %8(%10, %3)[2m ; ops [%8:callee/borrow#0, %10:value/consume#0, %3:value/consume#1][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn DSP_DSP_common_Math_ilog10_aux.Int.Int.Int(%0 d[2m: Int @consume[0m, %1 x[2m: Int @borrow[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %2 = const.int 10
    %3 = idiv %1, %2[2m ; ops [%1:value/borrow#0, %2:value/borrow#1][0m
    %4 = const.int 0
    %5 = igt %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb3, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb3 match.arm.0.body:
    %8 = fn_ref $DSP_DSP_common_Math_ilog10_aux.Int.Int.Int
    %9 = const.int 1
    %10 = iadd %0, %9[2m ; ops [%0:value/borrow#0, %9:value/borrow#1][0m
    tail_call %8(%10, %3)[2m ; ops [%8:callee/borrow#0, %10:value/consume#0, %3:value/consume#1][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    return %0[2m ; ops [%0:return/consume#0][0m
}
extern fn ipow(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Int @borrow[0m)[2m -> Int @owned[0m;
fn $DSP.DSP_common.Filter_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $DSP.DSP_common.Filter._min
    %1 = fn_ref $biquad.cons
    %2 = fn_ref $iir1.cons
    %3 = fn_ref $allpass.cons
    %4 = fn_ref $lpf.cons
    %5 = fn_ref $hpf.cons
    %6 = fn_ref $bpf.cons
    %7 = fn_ref $notch.cons
    %8 = fn_ref $lowshelf.cons
    %9 = fn_ref $highshelf.cons
    %10 = fn_ref $peak.cons
    %11 = fn_ref $pole.cons
    %12 = fn_ref $lpf1p.cons
    %13 = fn_ref $hpf1p.cons
    %14 = fn_ref $ladder.cons
    %15 = fn_ref $allpass2.cons
    %16 = fn_ref $dispersion32.cons
    %17 = fn_ref $moogff
    %18 = fn_ref $limiter
    %19 = const.void
    return %19[2m ; ops [%19:return/consume#0][0m
}
fn DSP.DSP_common.Filter._min(%0 a[2m: Double @consume[0m, %1 b[2m: Double @consume[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fn_ref $DSP.DSP_common.Math.min
    %3 = call %2 as $DSP_DSP_common_Math_min.Double.Double.Double(%0, %1)[2m : Double[0m[2m ; ops [%2:callee/borrow#0, %0:value/consume#0, %1:value/consume#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
}
fn DSP_DSP_common_Math_min.Double.Double.Double(%0 a[2m: Double @consume[0m, %1 b[2m: Double @consume[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = flt %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %3 = phi [bb4: %0, bb6: %1][2m : Double[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
  bb3 match.arm.0.test:
    %4 = const.bool true
    %5 = beq %2, %4[2m ; ops [%2:value/borrow#0, %4:value/borrow#1][0m
    cond %5, bb4, bb5[2m ; ops [%5:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn biquad.cons(%0 b0[2m: Double @borrow[0m, %1 b1[2m: Double @borrow[0m, %2 b2[2m: Double @borrow[0m, %3 a1[2m: Double @borrow[0m, %4 a2[2m: Double @borrow[0m, %5 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = fn_ref $ylc_create_audio_frame_node
    %7 = fn_ref $biquad.frame
    %8 = const.string "biquad"
    %9 = const.int 6
    %10 = const.int 1
    %11 = const.int 32
    %12 = call %6(%7, %9, %10, %11, %8)[2m : Ptr[0m[2m ; ops [%6:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1, %10:value/borrow#2, %11:value/borrow#3, %8:value/borrow#4][0m
    %13 = fn_ref $ylc_audio_node_set_state_init
    %14 = fn_ref $biquad.init
    %15 = call %13(%12, %14)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0, %14:value/borrow#1][0m
    %16 = fn_ref $ylc_audio_node_inline_state
    %17 = call %16(%12)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %12:value/borrow#0][0m
    %18 = fn_ref $biquad.init
    %19 = call %18(%17)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %17:value/borrow#0][0m
    %20 = fn_ref $node_connect_input
    %21 = fn_ref $const_sig
    %22 = call %21(%0)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %0:value/borrow#0][0m
    %23 = const.int 0
    %24 = call %20(%23, %12, %22)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %12:value/borrow#1, %22:value/borrow#2][0m
    %25 = fn_ref $const_sig
    %26 = call %25(%1)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %1:value/borrow#0][0m
    %27 = const.int 1
    %28 = call %20(%27, %12, %26)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %27:value/borrow#0, %12:value/borrow#1, %26:value/borrow#2][0m
    %29 = fn_ref $const_sig
    %30 = call %29(%2)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %2:value/borrow#0][0m
    %31 = const.int 2
    %32 = call %20(%31, %12, %30)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %31:value/borrow#0, %12:value/borrow#1, %30:value/borrow#2][0m
    %33 = fn_ref $const_sig
    %34 = call %33(%3)[2m : Ptr[0m[2m ; ops [%33:callee/borrow#0, %3:value/borrow#0][0m
    %35 = const.int 3
    %36 = call %20(%35, %12, %34)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %35:value/borrow#0, %12:value/borrow#1, %34:value/borrow#2][0m
    %37 = fn_ref $const_sig
    %38 = call %37(%4)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %4:value/borrow#0][0m
    %39 = const.int 4
    %40 = call %20(%39, %12, %38)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %39:value/borrow#0, %12:value/borrow#1, %38:value/borrow#2][0m
    %41 = fn_ref $const_sig
    %42 = call %41(%5)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %5:value/borrow#0][0m
    %43 = const.int 5
    %44 = call %20(%43, %12, %42)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %43:value/borrow#0, %12:value/borrow#1, %42:value/borrow#2][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn biquad.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 16
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 0
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 24
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 16
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 8
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.int 0
    %27 = ptr_offset %0, %26[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = const.double 0
    %30 = store %28, %29[2m : ()[0m[2m ; ops [%28:value/borrow#0, %29:element/consume#1][0m
    %31 = const.void
    return %31[2m ; ops [%31:return/consume#0][0m
}
fn biquad.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 b0[2m: Double @borrow[0m, %5 b1[2m: Double @borrow[0m, %6 b2[2m: Double @borrow[0m, %7 a1[2m: Double @borrow[0m, %8 a2[2m: Double @borrow[0m, %9 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = const.int 0
    %11 = ptr_offset %1, %10[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Double[2m ; ops [%11:value/borrow#0][0m
    %13 = const.int 2
    %14 = const.int 0
    %15 = construct.tuple { %13, %14, %12 }[2m : Array of Double[0m[2m ; ops [%13:field/borrow#0, %14:field/borrow#1, %12:field/borrow#2][0m
    %16 = const.int 16
    %17 = ptr_offset %1, %16[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.int 2
    %20 = const.int 0
    %21 = construct.tuple { %19, %20, %18 }[2m : Array of Double[0m[2m ; ops [%19:field/borrow#0, %20:field/borrow#1, %18:field/borrow#2][0m
    %22 = fmul %4, %9[2m ; ops [%4:value/borrow#0, %9:value/borrow#1][0m
    %23 = const.int 0
    %24 = extract.field %15, 1[2m : Int[0m[2m ; ops [%15:container/borrow#0][0m
    %25 = iadd %24, %23[2m ; ops [%24:value/borrow#0, %23:value/borrow#1][0m
    %26 = extract.field %15, 2[2m : Ptr[0m[2m ; ops [%15:container/borrow#0][0m
    %27 = ptr_offset %26, %25[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %25:index/borrow#1][0m
    %28 = load %27[2m : Double[0m[2m ; ops [%27:value/borrow#0][0m
    %29 = fmul %5, %28[2m ; ops [%5:value/borrow#0, %28:value/borrow#1][0m
    %30 = fadd %22, %29[2m ; ops [%22:value/borrow#0, %29:value/borrow#1][0m
    %31 = const.int 1
    %32 = extract.field %15, 1[2m : Int[0m[2m ; ops [%15:container/borrow#0][0m
    %33 = iadd %32, %31[2m ; ops [%32:value/borrow#0, %31:value/borrow#1][0m
    %34 = extract.field %15, 2[2m : Ptr[0m[2m ; ops [%15:container/borrow#0][0m
    %35 = ptr_offset %34, %33[2m : Ptr[0m[2m ; ops [%34:value/borrow#0, %33:index/borrow#1][0m
    %36 = load %35[2m : Double[0m[2m ; ops [%35:value/borrow#0][0m
    %37 = fmul %6, %36[2m ; ops [%6:value/borrow#0, %36:value/borrow#1][0m
    %38 = fadd %30, %37[2m ; ops [%30:value/borrow#0, %37:value/borrow#1][0m
    %39 = const.int 0
    %40 = extract.field %21, 1[2m : Int[0m[2m ; ops [%21:container/borrow#0][0m
    %41 = iadd %40, %39[2m ; ops [%40:value/borrow#0, %39:value/borrow#1][0m
    %42 = extract.field %21, 2[2m : Ptr[0m[2m ; ops [%21:container/borrow#0][0m
    %43 = ptr_offset %42, %41[2m : Ptr[0m[2m ; ops [%42:value/borrow#0, %41:index/borrow#1][0m
    %44 = load %43[2m : Double[0m[2m ; ops [%43:value/borrow#0][0m
    %45 = fmul %7, %44[2m ; ops [%7:value/borrow#0, %44:value/borrow#1][0m
    %46 = fsub %38, %45[2m ; ops [%38:value/borrow#0, %45:value/borrow#1][0m
    %47 = const.int 1
    %48 = extract.field %21, 1[2m : Int[0m[2m ; ops [%21:container/borrow#0][0m
    %49 = iadd %48, %47[2m ; ops [%48:value/borrow#0, %47:value/borrow#1][0m
    %50 = extract.field %21, 2[2m : Ptr[0m[2m ; ops [%21:container/borrow#0][0m
    %51 = ptr_offset %50, %49[2m : Ptr[0m[2m ; ops [%50:value/borrow#0, %49:index/borrow#1][0m
    %52 = load %51[2m : Double[0m[2m ; ops [%51:value/borrow#0][0m
    %53 = fmul %8, %52[2m ; ops [%8:value/borrow#0, %52:value/borrow#1][0m
    %54 = fsub %46, %53[2m ; ops [%46:value/borrow#0, %53:value/borrow#1][0m
    %55 = const.int 1
    %56 = const.int 0
    %57 = extract.field %15, 1[2m : Int[0m[2m ; ops [%15:container/borrow#0][0m
    %58 = iadd %57, %56[2m ; ops [%57:value/borrow#0, %56:value/borrow#1][0m
    %59 = extract.field %15, 2[2m : Ptr[0m[2m ; ops [%15:container/borrow#0][0m
    %60 = ptr_offset %59, %58[2m : Ptr[0m[2m ; ops [%59:value/borrow#0, %58:index/borrow#1][0m
    %61 = load %60[2m : Double[0m[2m ; ops [%60:value/borrow#0][0m
    %62 = extract.field %15, 1[2m : Int[0m[2m ; ops [%15:container/borrow#0][0m
    %63 = iadd %62, %55[2m ; ops [%62:value/borrow#0, %55:value/borrow#1][0m
    %64 = extract.field %15, 2[2m : Ptr[0m[2m ; ops [%15:container/borrow#0][0m
    %65 = ptr_offset %64, %63[2m : Ptr[0m[2m ; ops [%64:value/borrow#0, %63:index/borrow#1][0m
[35m    %91 = dup %61[2m : ()[0m[2m ; ops [%61:value/borrow#0][0m[0m
    %66 = store %65, %61[2m : ()[0m[2m ; ops [%65:value/borrow#0, %61:element/consume#1][0m
    %67 = const.int 0
    %68 = extract.field %15, 1[2m : Int[0m[2m ; ops [%15:container/borrow#0][0m
    %69 = iadd %68, %67[2m ; ops [%68:value/borrow#0, %67:value/borrow#1][0m
    %70 = extract.field %15, 2[2m : Ptr[0m[2m ; ops [%15:container/borrow#0][0m
    %71 = ptr_offset %70, %69[2m : Ptr[0m[2m ; ops [%70:value/borrow#0, %69:index/borrow#1][0m
    %72 = store %71, %9[2m : ()[0m[2m ; ops [%71:value/borrow#0, %9:element/consume#1][0m
    %73 = const.int 1
    %74 = const.int 0
    %75 = extract.field %21, 1[2m : Int[0m[2m ; ops [%21:container/borrow#0][0m
    %76 = iadd %75, %74[2m ; ops [%75:value/borrow#0, %74:value/borrow#1][0m
    %77 = extract.field %21, 2[2m : Ptr[0m[2m ; ops [%21:container/borrow#0][0m
    %78 = ptr_offset %77, %76[2m : Ptr[0m[2m ; ops [%77:value/borrow#0, %76:index/borrow#1][0m
    %79 = load %78[2m : Double[0m[2m ; ops [%78:value/borrow#0][0m
    %80 = extract.field %21, 1[2m : Int[0m[2m ; ops [%21:container/borrow#0][0m
    %81 = iadd %80, %73[2m ; ops [%80:value/borrow#0, %73:value/borrow#1][0m
    %82 = extract.field %21, 2[2m : Ptr[0m[2m ; ops [%21:container/borrow#0][0m
    %83 = ptr_offset %82, %81[2m : Ptr[0m[2m ; ops [%82:value/borrow#0, %81:index/borrow#1][0m
[35m    %92 = dup %79[2m : ()[0m[2m ; ops [%79:value/borrow#0][0m[0m
    %84 = store %83, %79[2m : ()[0m[2m ; ops [%83:value/borrow#0, %79:element/consume#1][0m
    %85 = const.int 0
    %86 = extract.field %21, 1[2m : Int[0m[2m ; ops [%21:container/borrow#0][0m
    %87 = iadd %86, %85[2m ; ops [%86:value/borrow#0, %85:value/borrow#1][0m
    %88 = extract.field %21, 2[2m : Ptr[0m[2m ; ops [%21:container/borrow#0][0m
    %89 = ptr_offset %88, %87[2m : Ptr[0m[2m ; ops [%88:value/borrow#0, %87:index/borrow#1][0m
    %90 = store %89, %54[2m : ()[0m[2m ; ops [%89:value/borrow#0, %54:element/consume#1][0m
    return %54[2m ; ops [%54:return/consume#0][0m
}
fn biquad.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = const.int 4
    %30 = ptr_offset %2, %29[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %29:index/borrow#1][0m
    %31 = load %30[2m : Ptr[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fn_ref $ylc_read_inlet_node_lane_i32
    %33 = const.int 0
    %34 = call %32(%31, %3, %33)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %33:value/borrow#2][0m
    %35 = const.int 5
    %36 = ptr_offset %2, %35[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %35:index/borrow#1][0m
    %37 = load %36[2m : Ptr[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = fn_ref $ylc_read_inlet_node_lane_i32
    %39 = const.int 0
    %40 = call %38(%37, %3, %39)[2m : Double[0m[2m ; ops [%38:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %39:value/borrow#2][0m
    %41 = fn_ref $biquad.kernel
    %42 = call %41(%0, %1, %3, %4, %10, %16, %22, %28, %34, %40)[2m : Double[0m[2m ; ops [%41:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/borrow#7, %34:value/borrow#8, %40:value/consume#9][0m
    %43 = fn_ref $ylc_get_output_buf
    %44 = call %43(%0)[2m : Ptr[0m[2m ; ops [%43:callee/borrow#0, %0:value/borrow#0][0m
    %45 = ptr_offset %44, %3[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %3:index/borrow#1][0m
    %46 = store %45, %42[2m : ()[0m[2m ; ops [%45:value/borrow#0, %42:element/consume#1][0m
    %47 = const.void
    return %47[2m ; ops [%47:return/consume#0][0m
}
fn iir1.cons(%0 b0[2m: Double @borrow[0m, %1 b1[2m: Double @borrow[0m, %2 a1[2m: Double @borrow[0m, %3 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = fn_ref $ylc_create_audio_frame_node
    %5 = fn_ref $iir1.frame
    %6 = const.string "iir1"
    %7 = const.int 4
    %8 = const.int 1
    %9 = const.int 16
    %10 = call %4(%5, %7, %8, %9, %6)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %5:value/borrow#0, %7:value/borrow#1, %8:value/borrow#2, %9:value/borrow#3, %6:value/borrow#4][0m
    %11 = fn_ref $ylc_audio_node_set_state_init
    %12 = fn_ref $iir1.init
    %13 = call %11(%10, %12)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %12:value/borrow#1][0m
    %14 = fn_ref $ylc_audio_node_inline_state
    %15 = call %14(%10)[2m : Ptr[0m[2m ; ops [%14:callee/borrow#0, %10:value/borrow#0][0m
    %16 = fn_ref $iir1.init
    %17 = call %16(%15)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %15:value/borrow#0][0m
    %18 = fn_ref $node_connect_input
    %19 = fn_ref $const_sig
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 0
    %22 = call %18(%21, %10, %20)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %10:value/borrow#1, %20:value/borrow#2][0m
    %23 = fn_ref $const_sig
    %24 = call %23(%1)[2m : Ptr[0m[2m ; ops [%23:callee/borrow#0, %1:value/borrow#0][0m
    %25 = const.int 1
    %26 = call %18(%25, %10, %24)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %25:value/borrow#0, %10:value/borrow#1, %24:value/borrow#2][0m
    %27 = fn_ref $const_sig
    %28 = call %27(%2)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %2:value/borrow#0][0m
    %29 = const.int 2
    %30 = call %18(%29, %10, %28)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %29:value/borrow#0, %10:value/borrow#1, %28:value/borrow#2][0m
    %31 = fn_ref $const_sig
    %32 = call %31(%3)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %3:value/borrow#0][0m
    %33 = const.int 3
    %34 = call %18(%33, %10, %32)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %33:value/borrow#0, %10:value/borrow#1, %32:value/borrow#2][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn iir1.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 8
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 0
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 8
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 0
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.void
    return %21[2m ; ops [%21:return/consume#0][0m
}
fn iir1.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 b0[2m: Double @borrow[0m, %5 b1[2m: Double @borrow[0m, %6 a1[2m: Double @borrow[0m, %7 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.int 0
    %9 = ptr_offset %1, %8[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %8:index/borrow#1][0m
    %10 = primitive_cast %9 from Ptr of Char to Ptr of Double[2m ; ops [%9:value/borrow#0][0m
    %11 = const.int 1
    %12 = const.int 0
    %13 = construct.tuple { %11, %12, %10 }[2m : Array of Double[0m[2m ; ops [%11:field/borrow#0, %12:field/borrow#1, %10:field/borrow#2][0m
    %14 = const.int 8
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = const.int 1
    %18 = const.int 0
    %19 = construct.tuple { %17, %18, %16 }[2m : Array of Double[0m[2m ; ops [%17:field/borrow#0, %18:field/borrow#1, %16:field/borrow#2][0m
    %20 = fmul %4, %7[2m ; ops [%4:value/borrow#0, %7:value/borrow#1][0m
    %21 = const.int 0
    %22 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %23 = iadd %22, %21[2m ; ops [%22:value/borrow#0, %21:value/borrow#1][0m
    %24 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %25 = ptr_offset %24, %23[2m : Ptr[0m[2m ; ops [%24:value/borrow#0, %23:index/borrow#1][0m
    %26 = load %25[2m : Double[0m[2m ; ops [%25:value/borrow#0][0m
    %27 = fmul %5, %26[2m ; ops [%5:value/borrow#0, %26:value/borrow#1][0m
    %28 = fadd %20, %27[2m ; ops [%20:value/borrow#0, %27:value/borrow#1][0m
    %29 = const.int 0
    %30 = extract.field %19, 1[2m : Int[0m[2m ; ops [%19:container/borrow#0][0m
    %31 = iadd %30, %29[2m ; ops [%30:value/borrow#0, %29:value/borrow#1][0m
    %32 = extract.field %19, 2[2m : Ptr[0m[2m ; ops [%19:container/borrow#0][0m
    %33 = ptr_offset %32, %31[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %31:index/borrow#1][0m
    %34 = load %33[2m : Double[0m[2m ; ops [%33:value/borrow#0][0m
    %35 = fmul %6, %34[2m ; ops [%6:value/borrow#0, %34:value/borrow#1][0m
    %36 = fsub %28, %35[2m ; ops [%28:value/borrow#0, %35:value/borrow#1][0m
    %37 = const.int 0
    %38 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %39 = iadd %38, %37[2m ; ops [%38:value/borrow#0, %37:value/borrow#1][0m
    %40 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %41 = ptr_offset %40, %39[2m : Ptr[0m[2m ; ops [%40:value/borrow#0, %39:index/borrow#1][0m
    %42 = store %41, %7[2m : ()[0m[2m ; ops [%41:value/borrow#0, %7:element/consume#1][0m
    %43 = const.int 0
    %44 = extract.field %19, 1[2m : Int[0m[2m ; ops [%19:container/borrow#0][0m
    %45 = iadd %44, %43[2m ; ops [%44:value/borrow#0, %43:value/borrow#1][0m
    %46 = extract.field %19, 2[2m : Ptr[0m[2m ; ops [%19:container/borrow#0][0m
    %47 = ptr_offset %46, %45[2m : Ptr[0m[2m ; ops [%46:value/borrow#0, %45:index/borrow#1][0m
    %48 = store %47, %36[2m : ()[0m[2m ; ops [%47:value/borrow#0, %36:element/consume#1][0m
    return %36[2m ; ops [%36:return/consume#0][0m
}
fn iir1.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = fn_ref $iir1.kernel
    %30 = call %29(%0, %1, %3, %4, %10, %16, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/consume#7][0m
    %31 = fn_ref $ylc_get_output_buf
    %32 = call %31(%0)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %0:value/borrow#0][0m
    %33 = ptr_offset %32, %3[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %3:index/borrow#1][0m
    %34 = store %33, %30[2m : ()[0m[2m ; ops [%33:value/borrow#0, %30:element/consume#1][0m
    %35 = const.void
    return %35[2m ; ops [%35:return/consume#0][0m
}
fn allpass.cons(%0 freq[2m: Double @borrow[0m, %1 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = fn_ref $ylc_create_audio_frame_node
    %3 = fn_ref $allpass.frame
    %4 = const.string "allpass"
    %5 = const.int 2
    %6 = const.int 1
    %7 = const.int 16
    %8 = call %2(%3, %5, %6, %7, %4)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %5:value/borrow#1, %6:value/borrow#2, %7:value/borrow#3, %4:value/borrow#4][0m
    %9 = fn_ref $ylc_audio_node_set_state_init
    %10 = fn_ref $allpass.init
    %11 = call %9(%8, %10)[2m : ()[0m[2m ; ops [%9:callee/borrow#0, %8:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $ylc_audio_node_inline_state
    %13 = call %12(%8)[2m : Ptr[0m[2m ; ops [%12:callee/borrow#0, %8:value/borrow#0][0m
    %14 = fn_ref $allpass.init
    %15 = call %14(%13)[2m : ()[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0][0m
    %16 = fn_ref $node_connect_input
    %17 = fn_ref $const_sig
    %18 = call %17(%0)[2m : Ptr[0m[2m ; ops [%17:callee/borrow#0, %0:value/borrow#0][0m
    %19 = const.int 0
    %20 = call %16(%19, %8, %18)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %19:value/borrow#0, %8:value/borrow#1, %18:value/borrow#2][0m
    %21 = fn_ref $const_sig
    %22 = call %21(%1)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %1:value/borrow#0][0m
    %23 = const.int 1
    %24 = call %16(%23, %8, %22)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %23:value/borrow#0, %8:value/borrow#1, %22:value/borrow#2][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn allpass.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $iir1.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn allpass.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = fn_ref $tan
    %7 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %8 = fmul %7, %4[2m ; ops [%7:value/borrow#0, %4:value/borrow#1][0m
    %9 = fmul %8, %3[2m ; ops [%8:value/borrow#0, %3:value/borrow#1][0m
    %10 = call %6(%9)[2m : Double[0m[2m ; ops [%6:callee/borrow#0, %9:value/borrow#0][0m
    %11 = const.double 1
    %12 = fsub %10, %11[2m ; ops [%10:value/borrow#0, %11:value/borrow#1][0m
    %13 = const.double 1
    %14 = fadd %10, %13[2m ; ops [%10:value/borrow#0, %13:value/borrow#1][0m
    %15 = fdiv %12, %14[2m ; ops [%12:value/borrow#0, %14:value/borrow#1][0m
    %16 = const.double 1
    %17 = fn_ref $iir1.kernel
    %18 = const.int 0
    %19 = ptr_offset %1, %18[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %18:index/borrow#1][0m
    %20 = primitive_cast %19 from Ptr of Char to Ptr of Char[2m ; ops [%19:value/borrow#0][0m
    %21 = call %17(%0, %20, %2, %3, %15, %16, %15, %5)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %20:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %15:value/borrow#4, %16:value/borrow#5, %15:value/borrow#6, %5:value/consume#7][0m
    return %21[2m ; ops [%21:return/consume#0][0m
}
fn allpass.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = fn_ref $allpass.kernel
    %18 = call %17(%0, %1, %3, %4, %10, %16)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/consume#5][0m
    %19 = fn_ref $ylc_get_output_buf
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = ptr_offset %20, %3[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %3:index/borrow#1][0m
    %22 = store %21, %18[2m : ()[0m[2m ; ops [%21:value/borrow#0, %18:element/consume#1][0m
    %23 = const.void
    return %23[2m ; ops [%23:return/consume#0][0m
}
fn lpf.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $lpf.frame
    %5 = const.string "lpf"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 32
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $lpf.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $lpf.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn lpf.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn lpf.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 2
    %8 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    %10 = fmul %9, %4[2m ; ops [%9:value/borrow#0, %4:value/borrow#1][0m
    %11 = fmul %10, %3[2m ; ops [%10:value/borrow#0, %3:value/borrow#1][0m
    %12 = fn_ref $cos
    %13 = call %12(%11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    %14 = fn_ref $sin
    %15 = call %14(%11)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %11:value/borrow#0][0m
    %16 = const.double 2
    %17 = fmul %16, %5[2m ; ops [%16:value/borrow#0, %5:value/borrow#1][0m
    %18 = fdiv %15, %17[2m ; ops [%15:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.double 1
    %20 = fsub %19, %13[2m ; ops [%19:value/borrow#0, %13:value/borrow#1][0m
    %21 = const.double 2
    %22 = fdiv %20, %21[2m ; ops [%20:value/borrow#0, %21:value/borrow#1][0m
    %23 = const.double 1
    %24 = fsub %23, %13[2m ; ops [%23:value/borrow#0, %13:value/borrow#1][0m
    %25 = const.double 1
    %26 = fsub %25, %13[2m ; ops [%25:value/borrow#0, %13:value/borrow#1][0m
    %27 = const.double 2
    %28 = fdiv %26, %27[2m ; ops [%26:value/borrow#0, %27:value/borrow#1][0m
    %29 = const.double 1
    %30 = fadd %29, %18[2m ; ops [%29:value/borrow#0, %18:value/borrow#1][0m
    %31 = const.double -2
    %32 = fmul %31, %13[2m ; ops [%31:value/borrow#0, %13:value/borrow#1][0m
    %33 = const.double 1
    %34 = fsub %33, %18[2m ; ops [%33:value/borrow#0, %18:value/borrow#1][0m
    %35 = fdiv %22, %30[2m ; ops [%22:value/borrow#0, %30:value/borrow#1][0m
    %36 = fdiv %24, %30[2m ; ops [%24:value/borrow#0, %30:value/borrow#1][0m
    %37 = fdiv %28, %30[2m ; ops [%28:value/borrow#0, %30:value/borrow#1][0m
    %38 = fdiv %32, %30[2m ; ops [%32:value/borrow#0, %30:value/borrow#1][0m
    %39 = fdiv %34, %30[2m ; ops [%34:value/borrow#0, %30:value/borrow#1][0m
    %40 = fn_ref $biquad.kernel
    %41 = const.int 0
    %42 = ptr_offset %1, %41[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %41:index/borrow#1][0m
    %43 = primitive_cast %42 from Ptr of Char to Ptr of Char[2m ; ops [%42:value/borrow#0][0m
    %44 = call %40(%0, %43, %2, %3, %35, %36, %37, %38, %39, %6)[2m : Double[0m[2m ; ops [%40:callee/borrow#0, %0:value/consume#0, %43:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %35:value/borrow#4, %36:value/borrow#5, %37:value/borrow#6, %38:value/borrow#7, %39:value/borrow#8, %6:value/consume#9][0m
    return %44[2m ; ops [%44:return/consume#0][0m
}
fn lpf.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $lpf.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn hpf.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $hpf.frame
    %5 = const.string "hpf"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 32
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $hpf.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $hpf.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn hpf.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn hpf.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 2
    %8 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    %10 = fmul %9, %4[2m ; ops [%9:value/borrow#0, %4:value/borrow#1][0m
    %11 = fmul %10, %3[2m ; ops [%10:value/borrow#0, %3:value/borrow#1][0m
    %12 = fn_ref $cos
    %13 = call %12(%11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    %14 = fn_ref $sin
    %15 = call %14(%11)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %11:value/borrow#0][0m
    %16 = const.double 2
    %17 = fmul %16, %5[2m ; ops [%16:value/borrow#0, %5:value/borrow#1][0m
    %18 = fdiv %15, %17[2m ; ops [%15:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.double 1
    %20 = fadd %19, %13[2m ; ops [%19:value/borrow#0, %13:value/borrow#1][0m
    %21 = const.double 2
    %22 = fdiv %20, %21[2m ; ops [%20:value/borrow#0, %21:value/borrow#1][0m
    %23 = const.int -1
    %24 = const.double 1
    %25 = fadd %24, %13[2m ; ops [%24:value/borrow#0, %13:value/borrow#1][0m
    %26 = primitive_cast %23 from Int to Double[2m ; ops [%23:value/borrow#0][0m
    %27 = fmul %26, %25[2m ; ops [%26:value/borrow#0, %25:value/borrow#1][0m
    %28 = const.double 1
    %29 = fadd %28, %13[2m ; ops [%28:value/borrow#0, %13:value/borrow#1][0m
    %30 = const.double 2
    %31 = fdiv %29, %30[2m ; ops [%29:value/borrow#0, %30:value/borrow#1][0m
    %32 = const.double 1
    %33 = fadd %32, %18[2m ; ops [%32:value/borrow#0, %18:value/borrow#1][0m
    %34 = const.double -2
    %35 = fmul %34, %13[2m ; ops [%34:value/borrow#0, %13:value/borrow#1][0m
    %36 = const.double 1
    %37 = fsub %36, %18[2m ; ops [%36:value/borrow#0, %18:value/borrow#1][0m
    %38 = fdiv %22, %33[2m ; ops [%22:value/borrow#0, %33:value/borrow#1][0m
    %39 = fdiv %27, %33[2m ; ops [%27:value/borrow#0, %33:value/borrow#1][0m
    %40 = fdiv %31, %33[2m ; ops [%31:value/borrow#0, %33:value/borrow#1][0m
    %41 = fdiv %35, %33[2m ; ops [%35:value/borrow#0, %33:value/borrow#1][0m
    %42 = fdiv %37, %33[2m ; ops [%37:value/borrow#0, %33:value/borrow#1][0m
    %43 = fn_ref $biquad.kernel
    %44 = const.int 0
    %45 = ptr_offset %1, %44[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %44:index/borrow#1][0m
    %46 = primitive_cast %45 from Ptr of Char to Ptr of Char[2m ; ops [%45:value/borrow#0][0m
    %47 = call %43(%0, %46, %2, %3, %38, %39, %40, %41, %42, %6)[2m : Double[0m[2m ; ops [%43:callee/borrow#0, %0:value/consume#0, %46:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %38:value/borrow#4, %39:value/borrow#5, %40:value/borrow#6, %41:value/borrow#7, %42:value/borrow#8, %6:value/consume#9][0m
    return %47[2m ; ops [%47:return/consume#0][0m
}
fn hpf.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $hpf.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn bpf.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $bpf.frame
    %5 = const.string "bpf"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 32
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $bpf.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $bpf.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn bpf.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn bpf.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 2
    %8 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    %10 = fmul %9, %4[2m ; ops [%9:value/borrow#0, %4:value/borrow#1][0m
    %11 = fmul %10, %3[2m ; ops [%10:value/borrow#0, %3:value/borrow#1][0m
    %12 = fn_ref $cos
    %13 = call %12(%11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    %14 = fn_ref $sin
    %15 = call %14(%11)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %11:value/borrow#0][0m
    %16 = const.double 2
    %17 = fmul %16, %5[2m ; ops [%16:value/borrow#0, %5:value/borrow#1][0m
    %18 = fdiv %15, %17[2m ; ops [%15:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.double 0
    %20 = const.int -1
    %21 = primitive_cast %20 from Int to Double[2m ; ops [%20:value/borrow#0][0m
    %22 = fmul %21, %18[2m ; ops [%21:value/borrow#0, %18:value/borrow#1][0m
    %23 = const.double 1
    %24 = fadd %23, %18[2m ; ops [%23:value/borrow#0, %18:value/borrow#1][0m
    %25 = const.double -2
    %26 = fmul %25, %13[2m ; ops [%25:value/borrow#0, %13:value/borrow#1][0m
    %27 = const.double 1
    %28 = fsub %27, %18[2m ; ops [%27:value/borrow#0, %18:value/borrow#1][0m
    %29 = fdiv %18, %24[2m ; ops [%18:value/borrow#0, %24:value/borrow#1][0m
    %30 = fdiv %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %31 = fdiv %22, %24[2m ; ops [%22:value/borrow#0, %24:value/borrow#1][0m
    %32 = fdiv %26, %24[2m ; ops [%26:value/borrow#0, %24:value/borrow#1][0m
    %33 = fdiv %28, %24[2m ; ops [%28:value/borrow#0, %24:value/borrow#1][0m
    %34 = fn_ref $biquad.kernel
    %35 = const.int 0
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Char[2m ; ops [%36:value/borrow#0][0m
    %38 = call %34(%0, %37, %2, %3, %29, %30, %31, %32, %33, %6)[2m : Double[0m[2m ; ops [%34:callee/borrow#0, %0:value/consume#0, %37:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %29:value/borrow#4, %30:value/borrow#5, %31:value/borrow#6, %32:value/borrow#7, %33:value/borrow#8, %6:value/consume#9][0m
    return %38[2m ; ops [%38:return/consume#0][0m
}
fn bpf.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $bpf.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn notch.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $notch.frame
    %5 = const.string "notch"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 32
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $notch.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $notch.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn notch.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn notch.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 2
    %8 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    %10 = fmul %9, %4[2m ; ops [%9:value/borrow#0, %4:value/borrow#1][0m
    %11 = fmul %10, %3[2m ; ops [%10:value/borrow#0, %3:value/borrow#1][0m
    %12 = fn_ref $cos
    %13 = call %12(%11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    %14 = fn_ref $sin
    %15 = call %14(%11)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %11:value/borrow#0][0m
    %16 = const.double 2
    %17 = fmul %16, %5[2m ; ops [%16:value/borrow#0, %5:value/borrow#1][0m
    %18 = fdiv %15, %17[2m ; ops [%15:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.double 1
    %20 = const.double -2
    %21 = fmul %20, %13[2m ; ops [%20:value/borrow#0, %13:value/borrow#1][0m
    %22 = const.double 1
    %23 = const.double 1
    %24 = fadd %23, %18[2m ; ops [%23:value/borrow#0, %18:value/borrow#1][0m
    %25 = const.double -2
    %26 = fmul %25, %13[2m ; ops [%25:value/borrow#0, %13:value/borrow#1][0m
    %27 = const.double 1
    %28 = fsub %27, %18[2m ; ops [%27:value/borrow#0, %18:value/borrow#1][0m
    %29 = fdiv %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %30 = fdiv %21, %24[2m ; ops [%21:value/borrow#0, %24:value/borrow#1][0m
    %31 = fdiv %22, %24[2m ; ops [%22:value/borrow#0, %24:value/borrow#1][0m
    %32 = fdiv %26, %24[2m ; ops [%26:value/borrow#0, %24:value/borrow#1][0m
    %33 = fdiv %28, %24[2m ; ops [%28:value/borrow#0, %24:value/borrow#1][0m
    %34 = fn_ref $biquad.kernel
    %35 = const.int 0
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Char[2m ; ops [%36:value/borrow#0][0m
    %38 = call %34(%0, %37, %2, %3, %29, %30, %31, %32, %33, %6)[2m : Double[0m[2m ; ops [%34:callee/borrow#0, %0:value/consume#0, %37:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %29:value/borrow#4, %30:value/borrow#5, %31:value/borrow#6, %32:value/borrow#7, %33:value/borrow#8, %6:value/consume#9][0m
    return %38[2m ; ops [%38:return/consume#0][0m
}
fn notch.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $notch.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn lowshelf.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 gain_db[2m: Double @borrow[0m, %3 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = fn_ref $ylc_create_audio_frame_node
    %5 = fn_ref $lowshelf.frame
    %6 = const.string "lowshelf"
    %7 = const.int 4
    %8 = const.int 1
    %9 = const.int 32
    %10 = call %4(%5, %7, %8, %9, %6)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %5:value/borrow#0, %7:value/borrow#1, %8:value/borrow#2, %9:value/borrow#3, %6:value/borrow#4][0m
    %11 = fn_ref $ylc_audio_node_set_state_init
    %12 = fn_ref $lowshelf.init
    %13 = call %11(%10, %12)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %12:value/borrow#1][0m
    %14 = fn_ref $ylc_audio_node_inline_state
    %15 = call %14(%10)[2m : Ptr[0m[2m ; ops [%14:callee/borrow#0, %10:value/borrow#0][0m
    %16 = fn_ref $lowshelf.init
    %17 = call %16(%15)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %15:value/borrow#0][0m
    %18 = fn_ref $node_connect_input
    %19 = fn_ref $const_sig
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 0
    %22 = call %18(%21, %10, %20)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %10:value/borrow#1, %20:value/borrow#2][0m
    %23 = fn_ref $const_sig
    %24 = call %23(%1)[2m : Ptr[0m[2m ; ops [%23:callee/borrow#0, %1:value/borrow#0][0m
    %25 = const.int 1
    %26 = call %18(%25, %10, %24)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %25:value/borrow#0, %10:value/borrow#1, %24:value/borrow#2][0m
    %27 = fn_ref $const_sig
    %28 = call %27(%2)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %2:value/borrow#0][0m
    %29 = const.int 2
    %30 = call %18(%29, %10, %28)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %29:value/borrow#0, %10:value/borrow#1, %28:value/borrow#2][0m
    %31 = fn_ref $const_sig
    %32 = call %31(%3)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %3:value/borrow#0][0m
    %33 = const.int 3
    %34 = call %18(%33, %10, %32)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %33:value/borrow#0, %10:value/borrow#1, %32:value/borrow#2][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn lowshelf.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn lowshelf.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 gain_db[2m: Double @borrow[0m, %7 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.double 2
    %9 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %10 = fmul %8, %9[2m ; ops [%8:value/borrow#0, %9:value/borrow#1][0m
    %11 = fmul %10, %4[2m ; ops [%10:value/borrow#0, %4:value/borrow#1][0m
    %12 = fmul %11, %3[2m ; ops [%11:value/borrow#0, %3:value/borrow#1][0m
    %13 = fn_ref $cos
    %14 = call %13(%12)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $sin
    %16 = call %15(%12)[2m : Double[0m[2m ; ops [%15:callee/borrow#0, %12:value/borrow#0][0m
    %17 = fn_ref $pow
    %18 = const.double 10
    %19 = const.double 40
    %20 = fdiv %6, %19[2m ; ops [%6:value/borrow#0, %19:value/borrow#1][0m
    %21 = call %17(%18, %20)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %18:value/borrow#0, %20:value/borrow#1][0m
    %22 = const.double 2
    %23 = fmul %22, %5[2m ; ops [%22:value/borrow#0, %5:value/borrow#1][0m
    %24 = fdiv %16, %23[2m ; ops [%16:value/borrow#0, %23:value/borrow#1][0m
    %25 = fn_ref $sqrt
    %26 = call %25(%21)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %21:value/borrow#0][0m
    %27 = const.double 1
    %28 = fadd %21, %27[2m ; ops [%21:value/borrow#0, %27:value/borrow#1][0m
    %29 = const.double 1
    %30 = fsub %21, %29[2m ; ops [%21:value/borrow#0, %29:value/borrow#1][0m
    %31 = fmul %30, %14[2m ; ops [%30:value/borrow#0, %14:value/borrow#1][0m
    %32 = fsub %28, %31[2m ; ops [%28:value/borrow#0, %31:value/borrow#1][0m
    %33 = const.double 2
    %34 = fmul %33, %26[2m ; ops [%33:value/borrow#0, %26:value/borrow#1][0m
    %35 = fmul %34, %24[2m ; ops [%34:value/borrow#0, %24:value/borrow#1][0m
    %36 = fadd %32, %35[2m ; ops [%32:value/borrow#0, %35:value/borrow#1][0m
    %37 = fmul %21, %36[2m ; ops [%21:value/borrow#0, %36:value/borrow#1][0m
    %38 = const.double 2
    %39 = fmul %38, %21[2m ; ops [%38:value/borrow#0, %21:value/borrow#1][0m
    %40 = const.double 1
    %41 = fsub %21, %40[2m ; ops [%21:value/borrow#0, %40:value/borrow#1][0m
    %42 = const.double 1
    %43 = fadd %21, %42[2m ; ops [%21:value/borrow#0, %42:value/borrow#1][0m
    %44 = fmul %43, %14[2m ; ops [%43:value/borrow#0, %14:value/borrow#1][0m
    %45 = fsub %41, %44[2m ; ops [%41:value/borrow#0, %44:value/borrow#1][0m
    %46 = fmul %39, %45[2m ; ops [%39:value/borrow#0, %45:value/borrow#1][0m
    %47 = const.double 1
    %48 = fadd %21, %47[2m ; ops [%21:value/borrow#0, %47:value/borrow#1][0m
    %49 = const.double 1
    %50 = fsub %21, %49[2m ; ops [%21:value/borrow#0, %49:value/borrow#1][0m
    %51 = fmul %50, %14[2m ; ops [%50:value/borrow#0, %14:value/borrow#1][0m
    %52 = fsub %48, %51[2m ; ops [%48:value/borrow#0, %51:value/borrow#1][0m
    %53 = const.double 2
    %54 = fmul %53, %26[2m ; ops [%53:value/borrow#0, %26:value/borrow#1][0m
    %55 = fmul %54, %24[2m ; ops [%54:value/borrow#0, %24:value/borrow#1][0m
    %56 = fsub %52, %55[2m ; ops [%52:value/borrow#0, %55:value/borrow#1][0m
    %57 = fmul %21, %56[2m ; ops [%21:value/borrow#0, %56:value/borrow#1][0m
    %58 = const.double 1
    %59 = fadd %21, %58[2m ; ops [%21:value/borrow#0, %58:value/borrow#1][0m
    %60 = const.double 1
    %61 = fsub %21, %60[2m ; ops [%21:value/borrow#0, %60:value/borrow#1][0m
    %62 = fmul %61, %14[2m ; ops [%61:value/borrow#0, %14:value/borrow#1][0m
    %63 = fadd %59, %62[2m ; ops [%59:value/borrow#0, %62:value/borrow#1][0m
    %64 = const.double 2
    %65 = fmul %64, %26[2m ; ops [%64:value/borrow#0, %26:value/borrow#1][0m
    %66 = fmul %65, %24[2m ; ops [%65:value/borrow#0, %24:value/borrow#1][0m
    %67 = fadd %63, %66[2m ; ops [%63:value/borrow#0, %66:value/borrow#1][0m
    %68 = const.double -2
    %69 = const.double 1
    %70 = fsub %21, %69[2m ; ops [%21:value/borrow#0, %69:value/borrow#1][0m
    %71 = const.double 1
    %72 = fadd %21, %71[2m ; ops [%21:value/borrow#0, %71:value/borrow#1][0m
    %73 = fmul %72, %14[2m ; ops [%72:value/borrow#0, %14:value/borrow#1][0m
    %74 = fadd %70, %73[2m ; ops [%70:value/borrow#0, %73:value/borrow#1][0m
    %75 = fmul %68, %74[2m ; ops [%68:value/borrow#0, %74:value/borrow#1][0m
    %76 = const.double 1
    %77 = fadd %21, %76[2m ; ops [%21:value/borrow#0, %76:value/borrow#1][0m
    %78 = const.double 1
    %79 = fsub %21, %78[2m ; ops [%21:value/borrow#0, %78:value/borrow#1][0m
    %80 = fmul %79, %14[2m ; ops [%79:value/borrow#0, %14:value/borrow#1][0m
    %81 = fadd %77, %80[2m ; ops [%77:value/borrow#0, %80:value/borrow#1][0m
    %82 = const.double 2
    %83 = fmul %82, %26[2m ; ops [%82:value/borrow#0, %26:value/borrow#1][0m
    %84 = fmul %83, %24[2m ; ops [%83:value/borrow#0, %24:value/borrow#1][0m
    %85 = fsub %81, %84[2m ; ops [%81:value/borrow#0, %84:value/borrow#1][0m
    %86 = fdiv %37, %67[2m ; ops [%37:value/borrow#0, %67:value/borrow#1][0m
    %87 = fdiv %46, %67[2m ; ops [%46:value/borrow#0, %67:value/borrow#1][0m
    %88 = fdiv %57, %67[2m ; ops [%57:value/borrow#0, %67:value/borrow#1][0m
    %89 = fdiv %75, %67[2m ; ops [%75:value/borrow#0, %67:value/borrow#1][0m
    %90 = fdiv %85, %67[2m ; ops [%85:value/borrow#0, %67:value/borrow#1][0m
    %91 = fn_ref $biquad.kernel
    %92 = const.int 0
    %93 = ptr_offset %1, %92[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %92:index/borrow#1][0m
    %94 = primitive_cast %93 from Ptr of Char to Ptr of Char[2m ; ops [%93:value/borrow#0][0m
    %95 = call %91(%0, %94, %2, %3, %86, %87, %88, %89, %90, %7)[2m : Double[0m[2m ; ops [%91:callee/borrow#0, %0:value/consume#0, %94:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %86:value/borrow#4, %87:value/borrow#5, %88:value/borrow#6, %89:value/borrow#7, %90:value/borrow#8, %7:value/consume#9][0m
    return %95[2m ; ops [%95:return/consume#0][0m
}
fn lowshelf.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = fn_ref $lowshelf.kernel
    %30 = call %29(%0, %1, %3, %4, %10, %16, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/consume#7][0m
    %31 = fn_ref $ylc_get_output_buf
    %32 = call %31(%0)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %0:value/borrow#0][0m
    %33 = ptr_offset %32, %3[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %3:index/borrow#1][0m
    %34 = store %33, %30[2m : ()[0m[2m ; ops [%33:value/borrow#0, %30:element/consume#1][0m
    %35 = const.void
    return %35[2m ; ops [%35:return/consume#0][0m
}
fn highshelf.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 gain_db[2m: Double @borrow[0m, %3 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = fn_ref $ylc_create_audio_frame_node
    %5 = fn_ref $highshelf.frame
    %6 = const.string "highshelf"
    %7 = const.int 4
    %8 = const.int 1
    %9 = const.int 32
    %10 = call %4(%5, %7, %8, %9, %6)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %5:value/borrow#0, %7:value/borrow#1, %8:value/borrow#2, %9:value/borrow#3, %6:value/borrow#4][0m
    %11 = fn_ref $ylc_audio_node_set_state_init
    %12 = fn_ref $highshelf.init
    %13 = call %11(%10, %12)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %12:value/borrow#1][0m
    %14 = fn_ref $ylc_audio_node_inline_state
    %15 = call %14(%10)[2m : Ptr[0m[2m ; ops [%14:callee/borrow#0, %10:value/borrow#0][0m
    %16 = fn_ref $highshelf.init
    %17 = call %16(%15)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %15:value/borrow#0][0m
    %18 = fn_ref $node_connect_input
    %19 = fn_ref $const_sig
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 0
    %22 = call %18(%21, %10, %20)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %10:value/borrow#1, %20:value/borrow#2][0m
    %23 = fn_ref $const_sig
    %24 = call %23(%1)[2m : Ptr[0m[2m ; ops [%23:callee/borrow#0, %1:value/borrow#0][0m
    %25 = const.int 1
    %26 = call %18(%25, %10, %24)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %25:value/borrow#0, %10:value/borrow#1, %24:value/borrow#2][0m
    %27 = fn_ref $const_sig
    %28 = call %27(%2)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %2:value/borrow#0][0m
    %29 = const.int 2
    %30 = call %18(%29, %10, %28)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %29:value/borrow#0, %10:value/borrow#1, %28:value/borrow#2][0m
    %31 = fn_ref $const_sig
    %32 = call %31(%3)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %3:value/borrow#0][0m
    %33 = const.int 3
    %34 = call %18(%33, %10, %32)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %33:value/borrow#0, %10:value/borrow#1, %32:value/borrow#2][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn highshelf.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn highshelf.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 gain_db[2m: Double @borrow[0m, %7 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.double 2
    %9 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %10 = fmul %8, %9[2m ; ops [%8:value/borrow#0, %9:value/borrow#1][0m
    %11 = fmul %10, %4[2m ; ops [%10:value/borrow#0, %4:value/borrow#1][0m
    %12 = fmul %11, %3[2m ; ops [%11:value/borrow#0, %3:value/borrow#1][0m
    %13 = fn_ref $cos
    %14 = call %13(%12)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $sin
    %16 = call %15(%12)[2m : Double[0m[2m ; ops [%15:callee/borrow#0, %12:value/borrow#0][0m
    %17 = fn_ref $pow
    %18 = const.double 10
    %19 = const.double 40
    %20 = fdiv %6, %19[2m ; ops [%6:value/borrow#0, %19:value/borrow#1][0m
    %21 = call %17(%18, %20)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %18:value/borrow#0, %20:value/borrow#1][0m
    %22 = const.double 2
    %23 = fmul %22, %5[2m ; ops [%22:value/borrow#0, %5:value/borrow#1][0m
    %24 = fdiv %16, %23[2m ; ops [%16:value/borrow#0, %23:value/borrow#1][0m
    %25 = fn_ref $sqrt
    %26 = call %25(%21)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %21:value/borrow#0][0m
    %27 = const.double 1
    %28 = fadd %21, %27[2m ; ops [%21:value/borrow#0, %27:value/borrow#1][0m
    %29 = const.double 1
    %30 = fsub %21, %29[2m ; ops [%21:value/borrow#0, %29:value/borrow#1][0m
    %31 = fmul %30, %14[2m ; ops [%30:value/borrow#0, %14:value/borrow#1][0m
    %32 = fadd %28, %31[2m ; ops [%28:value/borrow#0, %31:value/borrow#1][0m
    %33 = const.double 2
    %34 = fmul %33, %26[2m ; ops [%33:value/borrow#0, %26:value/borrow#1][0m
    %35 = fmul %34, %24[2m ; ops [%34:value/borrow#0, %24:value/borrow#1][0m
    %36 = fadd %32, %35[2m ; ops [%32:value/borrow#0, %35:value/borrow#1][0m
    %37 = fmul %21, %36[2m ; ops [%21:value/borrow#0, %36:value/borrow#1][0m
    %38 = const.double -2
    %39 = fmul %38, %21[2m ; ops [%38:value/borrow#0, %21:value/borrow#1][0m
    %40 = const.double 1
    %41 = fsub %21, %40[2m ; ops [%21:value/borrow#0, %40:value/borrow#1][0m
    %42 = const.double 1
    %43 = fadd %21, %42[2m ; ops [%21:value/borrow#0, %42:value/borrow#1][0m
    %44 = fmul %43, %14[2m ; ops [%43:value/borrow#0, %14:value/borrow#1][0m
    %45 = fadd %41, %44[2m ; ops [%41:value/borrow#0, %44:value/borrow#1][0m
    %46 = fmul %39, %45[2m ; ops [%39:value/borrow#0, %45:value/borrow#1][0m
    %47 = const.double 1
    %48 = fadd %21, %47[2m ; ops [%21:value/borrow#0, %47:value/borrow#1][0m
    %49 = const.double 1
    %50 = fsub %21, %49[2m ; ops [%21:value/borrow#0, %49:value/borrow#1][0m
    %51 = fmul %50, %14[2m ; ops [%50:value/borrow#0, %14:value/borrow#1][0m
    %52 = fadd %48, %51[2m ; ops [%48:value/borrow#0, %51:value/borrow#1][0m
    %53 = const.double 2
    %54 = fmul %53, %26[2m ; ops [%53:value/borrow#0, %26:value/borrow#1][0m
    %55 = fmul %54, %24[2m ; ops [%54:value/borrow#0, %24:value/borrow#1][0m
    %56 = fsub %52, %55[2m ; ops [%52:value/borrow#0, %55:value/borrow#1][0m
    %57 = fmul %21, %56[2m ; ops [%21:value/borrow#0, %56:value/borrow#1][0m
    %58 = const.double 1
    %59 = fadd %21, %58[2m ; ops [%21:value/borrow#0, %58:value/borrow#1][0m
    %60 = const.double 1
    %61 = fsub %21, %60[2m ; ops [%21:value/borrow#0, %60:value/borrow#1][0m
    %62 = fmul %61, %14[2m ; ops [%61:value/borrow#0, %14:value/borrow#1][0m
    %63 = fsub %59, %62[2m ; ops [%59:value/borrow#0, %62:value/borrow#1][0m
    %64 = const.double 2
    %65 = fmul %64, %26[2m ; ops [%64:value/borrow#0, %26:value/borrow#1][0m
    %66 = fmul %65, %24[2m ; ops [%65:value/borrow#0, %24:value/borrow#1][0m
    %67 = fadd %63, %66[2m ; ops [%63:value/borrow#0, %66:value/borrow#1][0m
    %68 = const.double 2
    %69 = const.double 1
    %70 = fsub %21, %69[2m ; ops [%21:value/borrow#0, %69:value/borrow#1][0m
    %71 = const.double 1
    %72 = fadd %21, %71[2m ; ops [%21:value/borrow#0, %71:value/borrow#1][0m
    %73 = fmul %72, %14[2m ; ops [%72:value/borrow#0, %14:value/borrow#1][0m
    %74 = fsub %70, %73[2m ; ops [%70:value/borrow#0, %73:value/borrow#1][0m
    %75 = fmul %68, %74[2m ; ops [%68:value/borrow#0, %74:value/borrow#1][0m
    %76 = const.double 1
    %77 = fadd %21, %76[2m ; ops [%21:value/borrow#0, %76:value/borrow#1][0m
    %78 = const.double 1
    %79 = fsub %21, %78[2m ; ops [%21:value/borrow#0, %78:value/borrow#1][0m
    %80 = fmul %79, %14[2m ; ops [%79:value/borrow#0, %14:value/borrow#1][0m
    %81 = fsub %77, %80[2m ; ops [%77:value/borrow#0, %80:value/borrow#1][0m
    %82 = const.double 2
    %83 = fmul %82, %26[2m ; ops [%82:value/borrow#0, %26:value/borrow#1][0m
    %84 = fmul %83, %24[2m ; ops [%83:value/borrow#0, %24:value/borrow#1][0m
    %85 = fsub %81, %84[2m ; ops [%81:value/borrow#0, %84:value/borrow#1][0m
    %86 = fdiv %37, %67[2m ; ops [%37:value/borrow#0, %67:value/borrow#1][0m
    %87 = fdiv %46, %67[2m ; ops [%46:value/borrow#0, %67:value/borrow#1][0m
    %88 = fdiv %57, %67[2m ; ops [%57:value/borrow#0, %67:value/borrow#1][0m
    %89 = fdiv %75, %67[2m ; ops [%75:value/borrow#0, %67:value/borrow#1][0m
    %90 = fdiv %85, %67[2m ; ops [%85:value/borrow#0, %67:value/borrow#1][0m
    %91 = fn_ref $biquad.kernel
    %92 = const.int 0
    %93 = ptr_offset %1, %92[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %92:index/borrow#1][0m
    %94 = primitive_cast %93 from Ptr of Char to Ptr of Char[2m ; ops [%93:value/borrow#0][0m
    %95 = call %91(%0, %94, %2, %3, %86, %87, %88, %89, %90, %7)[2m : Double[0m[2m ; ops [%91:callee/borrow#0, %0:value/consume#0, %94:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %86:value/borrow#4, %87:value/borrow#5, %88:value/borrow#6, %89:value/borrow#7, %90:value/borrow#8, %7:value/consume#9][0m
    return %95[2m ; ops [%95:return/consume#0][0m
}
fn highshelf.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = fn_ref $highshelf.kernel
    %30 = call %29(%0, %1, %3, %4, %10, %16, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/consume#7][0m
    %31 = fn_ref $ylc_get_output_buf
    %32 = call %31(%0)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %0:value/borrow#0][0m
    %33 = ptr_offset %32, %3[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %3:index/borrow#1][0m
    %34 = store %33, %30[2m : ()[0m[2m ; ops [%33:value/borrow#0, %30:element/consume#1][0m
    %35 = const.void
    return %35[2m ; ops [%35:return/consume#0][0m
}
fn peak.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 gain_db[2m: Double @borrow[0m, %3 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = fn_ref $ylc_create_audio_frame_node
    %5 = fn_ref $peak.frame
    %6 = const.string "peak"
    %7 = const.int 4
    %8 = const.int 1
    %9 = const.int 32
    %10 = call %4(%5, %7, %8, %9, %6)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %5:value/borrow#0, %7:value/borrow#1, %8:value/borrow#2, %9:value/borrow#3, %6:value/borrow#4][0m
    %11 = fn_ref $ylc_audio_node_set_state_init
    %12 = fn_ref $peak.init
    %13 = call %11(%10, %12)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %12:value/borrow#1][0m
    %14 = fn_ref $ylc_audio_node_inline_state
    %15 = call %14(%10)[2m : Ptr[0m[2m ; ops [%14:callee/borrow#0, %10:value/borrow#0][0m
    %16 = fn_ref $peak.init
    %17 = call %16(%15)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %15:value/borrow#0][0m
    %18 = fn_ref $node_connect_input
    %19 = fn_ref $const_sig
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 0
    %22 = call %18(%21, %10, %20)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %10:value/borrow#1, %20:value/borrow#2][0m
    %23 = fn_ref $const_sig
    %24 = call %23(%1)[2m : Ptr[0m[2m ; ops [%23:callee/borrow#0, %1:value/borrow#0][0m
    %25 = const.int 1
    %26 = call %18(%25, %10, %24)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %25:value/borrow#0, %10:value/borrow#1, %24:value/borrow#2][0m
    %27 = fn_ref $const_sig
    %28 = call %27(%2)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %2:value/borrow#0][0m
    %29 = const.int 2
    %30 = call %18(%29, %10, %28)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %29:value/borrow#0, %10:value/borrow#1, %28:value/borrow#2][0m
    %31 = fn_ref $const_sig
    %32 = call %31(%3)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %3:value/borrow#0][0m
    %33 = const.int 3
    %34 = call %18(%33, %10, %32)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %33:value/borrow#0, %10:value/borrow#1, %32:value/borrow#2][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn peak.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn peak.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 gain_db[2m: Double @borrow[0m, %7 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.double 2
    %9 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %10 = fmul %8, %9[2m ; ops [%8:value/borrow#0, %9:value/borrow#1][0m
    %11 = fmul %10, %4[2m ; ops [%10:value/borrow#0, %4:value/borrow#1][0m
    %12 = fmul %11, %3[2m ; ops [%11:value/borrow#0, %3:value/borrow#1][0m
    %13 = fn_ref $cos
    %14 = call %13(%12)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $sin
    %16 = call %15(%12)[2m : Double[0m[2m ; ops [%15:callee/borrow#0, %12:value/borrow#0][0m
    %17 = const.double 2
    %18 = fmul %17, %5[2m ; ops [%17:value/borrow#0, %5:value/borrow#1][0m
    %19 = fdiv %16, %18[2m ; ops [%16:value/borrow#0, %18:value/borrow#1][0m
    %20 = fn_ref $pow
    %21 = const.double 10
    %22 = const.double 40
    %23 = fdiv %6, %22[2m ; ops [%6:value/borrow#0, %22:value/borrow#1][0m
    %24 = call %20(%21, %23)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %21:value/borrow#0, %23:value/borrow#1][0m
    %25 = const.double 1
    %26 = fmul %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %27 = fadd %25, %26[2m ; ops [%25:value/borrow#0, %26:value/borrow#1][0m
    %28 = const.double -2
    %29 = fmul %28, %14[2m ; ops [%28:value/borrow#0, %14:value/borrow#1][0m
    %30 = const.double 1
    %31 = fmul %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %32 = fsub %30, %31[2m ; ops [%30:value/borrow#0, %31:value/borrow#1][0m
    %33 = const.double 1
    %34 = fdiv %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %35 = fadd %33, %34[2m ; ops [%33:value/borrow#0, %34:value/borrow#1][0m
    %36 = const.double -2
    %37 = fmul %36, %14[2m ; ops [%36:value/borrow#0, %14:value/borrow#1][0m
    %38 = const.double 1
    %39 = fdiv %19, %24[2m ; ops [%19:value/borrow#0, %24:value/borrow#1][0m
    %40 = fsub %38, %39[2m ; ops [%38:value/borrow#0, %39:value/borrow#1][0m
    %41 = fdiv %27, %35[2m ; ops [%27:value/borrow#0, %35:value/borrow#1][0m
    %42 = fdiv %29, %35[2m ; ops [%29:value/borrow#0, %35:value/borrow#1][0m
    %43 = fdiv %32, %35[2m ; ops [%32:value/borrow#0, %35:value/borrow#1][0m
    %44 = fdiv %37, %35[2m ; ops [%37:value/borrow#0, %35:value/borrow#1][0m
    %45 = fdiv %40, %35[2m ; ops [%40:value/borrow#0, %35:value/borrow#1][0m
    %46 = fn_ref $biquad.kernel
    %47 = const.int 0
    %48 = ptr_offset %1, %47[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %47:index/borrow#1][0m
    %49 = primitive_cast %48 from Ptr of Char to Ptr of Char[2m ; ops [%48:value/borrow#0][0m
    %50 = call %46(%0, %49, %2, %3, %41, %42, %43, %44, %45, %7)[2m : Double[0m[2m ; ops [%46:callee/borrow#0, %0:value/consume#0, %49:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %41:value/borrow#4, %42:value/borrow#5, %43:value/borrow#6, %44:value/borrow#7, %45:value/borrow#8, %7:value/consume#9][0m
    return %50[2m ; ops [%50:return/consume#0][0m
}
fn peak.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = fn_ref $peak.kernel
    %30 = call %29(%0, %1, %3, %4, %10, %16, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/consume#7][0m
    %31 = fn_ref $ylc_get_output_buf
    %32 = call %31(%0)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %0:value/borrow#0][0m
    %33 = ptr_offset %32, %3[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %3:index/borrow#1][0m
    %34 = store %33, %30[2m : ()[0m[2m ; ops [%33:value/borrow#0, %30:element/consume#1][0m
    %35 = const.void
    return %35[2m ; ops [%35:return/consume#0][0m
}
fn pole.cons(%0 g[2m: Double @borrow[0m, %1 x[2m: Double @borrow[0m, %2 z[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $pole.frame
    %5 = const.string "pole"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 0
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $pole.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $pole.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/consume#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn pole.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn pole.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 g[2m: Double @borrow[0m, %5 x[2m: Double @borrow[0m, %6 z[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = fsub %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    %8 = fmul %4, %7[2m ; ops [%4:value/borrow#0, %7:value/borrow#1][0m
    %9 = fadd %6, %8[2m ; ops [%6:value/borrow#0, %8:value/borrow#1][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn pole.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $pole.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/consume#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn lpf1p.cons(%0 freq[2m: Double @borrow[0m, %1 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = fn_ref $ylc_create_audio_frame_node
    %3 = fn_ref $lpf1p.frame
    %4 = const.string "lpf1p"
    %5 = const.int 2
    %6 = const.int 1
    %7 = const.int 16
    %8 = call %2(%3, %5, %6, %7, %4)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %5:value/borrow#1, %6:value/borrow#2, %7:value/borrow#3, %4:value/borrow#4][0m
    %9 = fn_ref $ylc_audio_node_set_state_init
    %10 = fn_ref $lpf1p.init
    %11 = call %9(%8, %10)[2m : ()[0m[2m ; ops [%9:callee/borrow#0, %8:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $ylc_audio_node_inline_state
    %13 = call %12(%8)[2m : Ptr[0m[2m ; ops [%12:callee/borrow#0, %8:value/borrow#0][0m
    %14 = fn_ref $lpf1p.init
    %15 = call %14(%13)[2m : ()[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0][0m
    %16 = fn_ref $node_connect_input
    %17 = fn_ref $const_sig
    %18 = call %17(%0)[2m : Ptr[0m[2m ; ops [%17:callee/borrow#0, %0:value/borrow#0][0m
    %19 = const.int 0
    %20 = call %16(%19, %8, %18)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %19:value/borrow#0, %8:value/borrow#1, %18:value/borrow#2][0m
    %21 = fn_ref $const_sig
    %22 = call %21(%1)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %1:value/borrow#0][0m
    %23 = const.int 1
    %24 = call %16(%23, %8, %22)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %23:value/borrow#0, %8:value/borrow#1, %22:value/borrow#2][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn lpf1p.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 8
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = primitive_cast %6 from Ptr of Char to Ptr of Double[2m ; ops [%6:value/borrow#0][0m
    %8 = const.double 0
    %9 = store %7, %8[2m : ()[0m[2m ; ops [%7:value/borrow#0, %8:element/consume#1][0m
    %10 = const.int 0
    %11 = ptr_offset %0, %10[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Double[2m ; ops [%11:value/borrow#0][0m
    %13 = const.double 0
    %14 = store %12, %13[2m : ()[0m[2m ; ops [%12:value/borrow#0, %13:element/consume#1][0m
    %15 = const.int 8
    %16 = ptr_offset %0, %15[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %15:index/borrow#1][0m
    %17 = fn_ref $pole.init
    %18 = call %17(%16)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %16:value/consume#0][0m
    %19 = const.void
    return %19[2m ; ops [%19:return/consume#0][0m
}
fn lpf1p.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = const.int 0
    %7 = ptr_offset %1, %6[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.int 1
    %10 = const.int 0
    %11 = construct.tuple { %9, %10, %8 }[2m : Array of Double[0m[2m ; ops [%9:field/borrow#0, %10:field/borrow#1, %8:field/borrow#2][0m
    %12 = fn_ref $DSP.DSP_common.Filter._min
    %13 = const.double 0.999
    %14 = const.double 2
    %15 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %16 = fmul %14, %15[2m ; ops [%14:value/borrow#0, %15:value/borrow#1][0m
    %17 = fmul %16, %4[2m ; ops [%16:value/borrow#0, %4:value/borrow#1][0m
    %18 = fmul %17, %3[2m ; ops [%17:value/borrow#0, %3:value/borrow#1][0m
    %19 = call %12(%13, %18)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %13:value/consume#0, %18:value/consume#1][0m
    %20 = const.int 0
    %21 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %22 = iadd %21, %20[2m ; ops [%21:value/borrow#0, %20:value/borrow#1][0m
    %23 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %24 = ptr_offset %23, %22[2m : Ptr[0m[2m ; ops [%23:value/borrow#0, %22:index/borrow#1][0m
    %25 = load %24[2m : Double[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $pole.kernel
    %27 = const.int 8
    %28 = ptr_offset %1, %27[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %27:index/borrow#1][0m
    %29 = primitive_cast %28 from Ptr of Char to Ptr of Char[2m ; ops [%28:value/borrow#0][0m
    %30 = call %26(%0, %29, %2, %3, %19, %5, %25)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %0:value/consume#0, %29:value/consume#1, %2:value/consume#2, %3:value/consume#3, %19:value/borrow#4, %5:value/borrow#5, %25:value/borrow#6][0m
    %31 = const.int 0
    %32 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %33 = iadd %32, %31[2m ; ops [%32:value/borrow#0, %31:value/borrow#1][0m
    %34 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %35 = ptr_offset %34, %33[2m : Ptr[0m[2m ; ops [%34:value/borrow#0, %33:index/borrow#1][0m
    %36 = store %35, %30[2m : ()[0m[2m ; ops [%35:value/borrow#0, %30:element/consume#1][0m
    return %30[2m ; ops [%30:return/consume#0][0m
}
fn lpf1p.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = fn_ref $lpf1p.kernel
    %18 = call %17(%0, %1, %3, %4, %10, %16)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5][0m
    %19 = fn_ref $ylc_get_output_buf
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = ptr_offset %20, %3[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %3:index/borrow#1][0m
    %22 = store %21, %18[2m : ()[0m[2m ; ops [%21:value/borrow#0, %18:element/consume#1][0m
    %23 = const.void
    return %23[2m ; ops [%23:return/consume#0][0m
}
fn hpf1p.cons(%0 freq[2m: Double @borrow[0m, %1 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = fn_ref $ylc_create_audio_frame_node
    %3 = fn_ref $hpf1p.frame
    %4 = const.string "hpf1p"
    %5 = const.int 2
    %6 = const.int 1
    %7 = const.int 16
    %8 = call %2(%3, %5, %6, %7, %4)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %5:value/borrow#1, %6:value/borrow#2, %7:value/borrow#3, %4:value/borrow#4][0m
    %9 = fn_ref $ylc_audio_node_set_state_init
    %10 = fn_ref $hpf1p.init
    %11 = call %9(%8, %10)[2m : ()[0m[2m ; ops [%9:callee/borrow#0, %8:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $ylc_audio_node_inline_state
    %13 = call %12(%8)[2m : Ptr[0m[2m ; ops [%12:callee/borrow#0, %8:value/borrow#0][0m
    %14 = fn_ref $hpf1p.init
    %15 = call %14(%13)[2m : ()[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0][0m
    %16 = fn_ref $node_connect_input
    %17 = fn_ref $const_sig
    %18 = call %17(%0)[2m : Ptr[0m[2m ; ops [%17:callee/borrow#0, %0:value/borrow#0][0m
    %19 = const.int 0
    %20 = call %16(%19, %8, %18)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %19:value/borrow#0, %8:value/borrow#1, %18:value/borrow#2][0m
    %21 = fn_ref $const_sig
    %22 = call %21(%1)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %1:value/borrow#0][0m
    %23 = const.int 1
    %24 = call %16(%23, %8, %22)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %23:value/borrow#0, %8:value/borrow#1, %22:value/borrow#2][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn hpf1p.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 8
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = primitive_cast %6 from Ptr of Char to Ptr of Double[2m ; ops [%6:value/borrow#0][0m
    %8 = const.double 0
    %9 = store %7, %8[2m : ()[0m[2m ; ops [%7:value/borrow#0, %8:element/consume#1][0m
    %10 = const.int 0
    %11 = ptr_offset %0, %10[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Double[2m ; ops [%11:value/borrow#0][0m
    %13 = const.double 0
    %14 = store %12, %13[2m : ()[0m[2m ; ops [%12:value/borrow#0, %13:element/consume#1][0m
    %15 = const.int 8
    %16 = ptr_offset %0, %15[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %15:index/borrow#1][0m
    %17 = fn_ref $pole.init
    %18 = call %17(%16)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %16:value/consume#0][0m
    %19 = const.void
    return %19[2m ; ops [%19:return/consume#0][0m
}
fn hpf1p.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = const.int 0
    %7 = ptr_offset %1, %6[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.int 1
    %10 = const.int 0
    %11 = construct.tuple { %9, %10, %8 }[2m : Array of Double[0m[2m ; ops [%9:field/borrow#0, %10:field/borrow#1, %8:field/borrow#2][0m
    %12 = fn_ref $DSP.DSP_common.Filter._min
    %13 = const.double 0.999
    %14 = const.double 2
    %15 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %16 = fmul %14, %15[2m ; ops [%14:value/borrow#0, %15:value/borrow#1][0m
    %17 = fmul %16, %4[2m ; ops [%16:value/borrow#0, %4:value/borrow#1][0m
    %18 = fmul %17, %3[2m ; ops [%17:value/borrow#0, %3:value/borrow#1][0m
    %19 = call %12(%13, %18)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %13:value/consume#0, %18:value/consume#1][0m
    %20 = const.int 0
    %21 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %22 = iadd %21, %20[2m ; ops [%21:value/borrow#0, %20:value/borrow#1][0m
    %23 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %24 = ptr_offset %23, %22[2m : Ptr[0m[2m ; ops [%23:value/borrow#0, %22:index/borrow#1][0m
    %25 = load %24[2m : Double[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $pole.kernel
    %27 = const.int 8
    %28 = ptr_offset %1, %27[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %27:index/borrow#1][0m
    %29 = primitive_cast %28 from Ptr of Char to Ptr of Char[2m ; ops [%28:value/borrow#0][0m
    %30 = call %26(%0, %29, %2, %3, %19, %5, %25)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %0:value/consume#0, %29:value/consume#1, %2:value/consume#2, %3:value/consume#3, %19:value/borrow#4, %5:value/borrow#5, %25:value/borrow#6][0m
    %31 = const.int 0
    %32 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %33 = iadd %32, %31[2m ; ops [%32:value/borrow#0, %31:value/borrow#1][0m
    %34 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %35 = ptr_offset %34, %33[2m : Ptr[0m[2m ; ops [%34:value/borrow#0, %33:index/borrow#1][0m
    %36 = store %35, %30[2m : ()[0m[2m ; ops [%35:value/borrow#0, %30:element/consume#1][0m
    %37 = fsub %5, %30[2m ; ops [%5:value/borrow#0, %30:value/borrow#1][0m
    return %37[2m ; ops [%37:return/consume#0][0m
}
fn hpf1p.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = fn_ref $hpf1p.kernel
    %18 = call %17(%0, %1, %3, %4, %10, %16)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5][0m
    %19 = fn_ref $ylc_get_output_buf
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = ptr_offset %20, %3[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %3:index/borrow#1][0m
    %22 = store %21, %18[2m : ()[0m[2m ; ops [%21:value/borrow#0, %18:element/consume#1][0m
    %23 = const.void
    return %23[2m ; ops [%23:return/consume#0][0m
}
fn ladder.cons(%0 cutoff[2m: Double @borrow[0m, %1 res[2m: Double @borrow[0m, %2 drive[2m: Double @borrow[0m, %3 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = fn_ref $ylc_create_audio_frame_node
    %5 = fn_ref $ladder.frame
    %6 = const.string "ladder"
    %7 = const.int 4
    %8 = const.int 1
    %9 = const.int 64
    %10 = call %4(%5, %7, %8, %9, %6)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %5:value/borrow#0, %7:value/borrow#1, %8:value/borrow#2, %9:value/borrow#3, %6:value/borrow#4][0m
    %11 = fn_ref $ylc_audio_node_set_state_init
    %12 = fn_ref $ladder.init
    %13 = call %11(%10, %12)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %12:value/borrow#1][0m
    %14 = fn_ref $ylc_audio_node_inline_state
    %15 = call %14(%10)[2m : Ptr[0m[2m ; ops [%14:callee/borrow#0, %10:value/borrow#0][0m
    %16 = fn_ref $ladder.init
    %17 = call %16(%15)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %15:value/borrow#0][0m
    %18 = fn_ref $node_connect_input
    %19 = fn_ref $const_sig
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 0
    %22 = call %18(%21, %10, %20)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %10:value/borrow#1, %20:value/borrow#2][0m
    %23 = fn_ref $const_sig
    %24 = call %23(%1)[2m : Ptr[0m[2m ; ops [%23:callee/borrow#0, %1:value/borrow#0][0m
    %25 = const.int 1
    %26 = call %18(%25, %10, %24)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %25:value/borrow#0, %10:value/borrow#1, %24:value/borrow#2][0m
    %27 = fn_ref $const_sig
    %28 = call %27(%2)[2m : Ptr[0m[2m ; ops [%27:callee/borrow#0, %2:value/borrow#0][0m
    %29 = const.int 2
    %30 = call %18(%29, %10, %28)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %29:value/borrow#0, %10:value/borrow#1, %28:value/borrow#2][0m
    %31 = fn_ref $const_sig
    %32 = call %31(%3)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %3:value/borrow#0][0m
    %33 = const.int 3
    %34 = call %18(%33, %10, %32)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %33:value/borrow#0, %10:value/borrow#1, %32:value/borrow#2][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn ladder.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 56
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 48
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 40
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 32
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 0
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = primitive_cast %18 from Ptr of Char to Ptr of Double[2m ; ops [%18:value/borrow#0][0m
    %20 = const.double 0
    %21 = store %19, %20[2m : ()[0m[2m ; ops [%19:value/borrow#0, %20:element/consume#1][0m
    %22 = const.int 24
    %23 = ptr_offset %0, %22[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %22:index/borrow#1][0m
    %24 = primitive_cast %23 from Ptr of Char to Ptr of Double[2m ; ops [%23:value/borrow#0][0m
    %25 = const.double 0
    %26 = store %24, %25[2m : ()[0m[2m ; ops [%24:value/borrow#0, %25:element/consume#1][0m
    %27 = const.int 16
    %28 = ptr_offset %0, %27[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %27:index/borrow#1][0m
    %29 = primitive_cast %28 from Ptr of Char to Ptr of Double[2m ; ops [%28:value/borrow#0][0m
    %30 = const.double 0
    %31 = store %29, %30[2m : ()[0m[2m ; ops [%29:value/borrow#0, %30:element/consume#1][0m
    %32 = const.int 8
    %33 = ptr_offset %0, %32[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %32:index/borrow#1][0m
    %34 = primitive_cast %33 from Ptr of Char to Ptr of Double[2m ; ops [%33:value/borrow#0][0m
    %35 = const.double 0
    %36 = store %34, %35[2m : ()[0m[2m ; ops [%34:value/borrow#0, %35:element/consume#1][0m
    %37 = const.int 0
    %38 = ptr_offset %0, %37[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %37:index/borrow#1][0m
    %39 = primitive_cast %38 from Ptr of Char to Ptr of Double[2m ; ops [%38:value/borrow#0][0m
    %40 = const.double 0
    %41 = store %39, %40[2m : ()[0m[2m ; ops [%39:value/borrow#0, %40:element/consume#1][0m
    %42 = const.int 32
    %43 = ptr_offset %0, %42[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %42:index/borrow#1][0m
    %44 = fn_ref $pole.init
    %45 = call %44(%43)[2m : ()[0m[2m ; ops [%44:callee/borrow#0, %43:value/consume#0][0m
    %46 = const.int 40
    %47 = ptr_offset %0, %46[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %46:index/borrow#1][0m
    %48 = call %44(%47)[2m : ()[0m[2m ; ops [%44:callee/borrow#0, %47:value/consume#0][0m
    %49 = const.int 48
    %50 = ptr_offset %0, %49[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %49:index/borrow#1][0m
    %51 = call %44(%50)[2m : ()[0m[2m ; ops [%44:callee/borrow#0, %50:value/consume#0][0m
    %52 = const.int 56
    %53 = ptr_offset %0, %52[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %52:index/borrow#1][0m
    %54 = call %44(%53)[2m : ()[0m[2m ; ops [%44:callee/borrow#0, %53:value/consume#0][0m
    %55 = const.void
    return %55[2m ; ops [%55:return/consume#0][0m
}
fn ladder.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 cutoff[2m: Double @borrow[0m, %5 res[2m: Double @borrow[0m, %6 drive[2m: Double @borrow[0m, %7 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.int 0
    %9 = ptr_offset %1, %8[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %8:index/borrow#1][0m
    %10 = primitive_cast %9 from Ptr of Char to Ptr of Double[2m ; ops [%9:value/borrow#0][0m
    %11 = const.int 4
    %12 = const.int 0
    %13 = construct.tuple { %11, %12, %10 }[2m : Array of Double[0m[2m ; ops [%11:field/borrow#0, %12:field/borrow#1, %10:field/borrow#2][0m
    %14 = const.double 2
    %15 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %16 = fmul %14, %15[2m ; ops [%14:value/borrow#0, %15:value/borrow#1][0m
    %17 = fmul %16, %4[2m ; ops [%16:value/borrow#0, %4:value/borrow#1][0m
    %18 = fmul %17, %3[2m ; ops [%17:value/borrow#0, %3:value/borrow#1][0m
    %19 = fn_ref $DSP.DSP_common.Filter._min
    %20 = const.double 0.99
    %21 = call %19(%20, %18)[2m : Double[0m[2m ; ops [%19:callee/borrow#0, %20:value/consume#0, %18:value/consume#1][0m
    %22 = fn_ref $tanh
    %23 = fmul %6, %7[2m ; ops [%6:value/borrow#0, %7:value/borrow#1][0m
    %24 = const.int 3
    %25 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %26 = iadd %25, %24[2m ; ops [%25:value/borrow#0, %24:value/borrow#1][0m
    %27 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %28 = ptr_offset %27, %26[2m : Ptr[0m[2m ; ops [%27:value/borrow#0, %26:index/borrow#1][0m
    %29 = load %28[2m : Double[0m[2m ; ops [%28:value/borrow#0][0m
    %30 = fmul %5, %29[2m ; ops [%5:value/borrow#0, %29:value/borrow#1][0m
    %31 = fsub %23, %30[2m ; ops [%23:value/borrow#0, %30:value/borrow#1][0m
    %32 = call %22(%31)[2m : Double[0m[2m ; ops [%22:callee/borrow#0, %31:value/borrow#0][0m
    %33 = const.int 0
    %34 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %35 = iadd %34, %33[2m ; ops [%34:value/borrow#0, %33:value/borrow#1][0m
    %36 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %37 = ptr_offset %36, %35[2m : Ptr[0m[2m ; ops [%36:value/borrow#0, %35:index/borrow#1][0m
    %38 = load %37[2m : Double[0m[2m ; ops [%37:value/borrow#0][0m
    %39 = fn_ref $pole.kernel
    %40 = const.int 32
    %41 = ptr_offset %1, %40[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %40:index/borrow#1][0m
    %42 = primitive_cast %41 from Ptr of Char to Ptr of Char[2m ; ops [%41:value/borrow#0][0m
    %43 = call %39(%0, %42, %2, %3, %21, %32, %38)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %42:value/consume#1, %2:value/consume#2, %3:value/consume#3, %21:value/borrow#4, %32:value/borrow#5, %38:value/borrow#6][0m
    %44 = const.int 1
    %45 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %46 = iadd %45, %44[2m ; ops [%45:value/borrow#0, %44:value/borrow#1][0m
    %47 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %48 = ptr_offset %47, %46[2m : Ptr[0m[2m ; ops [%47:value/borrow#0, %46:index/borrow#1][0m
    %49 = load %48[2m : Double[0m[2m ; ops [%48:value/borrow#0][0m
    %50 = const.int 40
    %51 = ptr_offset %1, %50[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %50:index/borrow#1][0m
    %52 = primitive_cast %51 from Ptr of Char to Ptr of Char[2m ; ops [%51:value/borrow#0][0m
    %53 = call %39(%0, %52, %2, %3, %21, %43, %49)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %52:value/consume#1, %2:value/consume#2, %3:value/consume#3, %21:value/borrow#4, %43:value/borrow#5, %49:value/borrow#6][0m
    %54 = const.int 2
    %55 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %56 = iadd %55, %54[2m ; ops [%55:value/borrow#0, %54:value/borrow#1][0m
    %57 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %58 = ptr_offset %57, %56[2m : Ptr[0m[2m ; ops [%57:value/borrow#0, %56:index/borrow#1][0m
    %59 = load %58[2m : Double[0m[2m ; ops [%58:value/borrow#0][0m
    %60 = const.int 48
    %61 = ptr_offset %1, %60[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %60:index/borrow#1][0m
    %62 = primitive_cast %61 from Ptr of Char to Ptr of Char[2m ; ops [%61:value/borrow#0][0m
    %63 = call %39(%0, %62, %2, %3, %21, %53, %59)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %62:value/consume#1, %2:value/consume#2, %3:value/consume#3, %21:value/borrow#4, %53:value/borrow#5, %59:value/borrow#6][0m
    %64 = const.int 3
    %65 = extract.field %13, 1[2m : Int[0m[2m ; ops [%13:container/borrow#0][0m
    %66 = iadd %65, %64[2m ; ops [%65:value/borrow#0, %64:value/borrow#1][0m
    %67 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %68 = ptr_offset %67, %66[2m : Ptr[0m[2m ; ops [%67:value/borrow#0, %66:index/borrow#1][0m
    %69 = load %68[2m : Double[0m[2m ; ops [%68:value/borrow#0][0m
    %70 = const.int 56
    %71 = ptr_offset %1, %70[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %70:index/borrow#1][0m
    %72 = primitive_cast %71 from Ptr of Char to Ptr of Char[2m ; ops [%71:value/borrow#0][0m
    %73 = call %39(%0, %72, %2, %3, %21, %63, %69)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %72:value/consume#1, %2:value/consume#2, %3:value/consume#3, %21:value/borrow#4, %63:value/borrow#5, %69:value/borrow#6][0m
    return %73[2m ; ops [%73:return/consume#0][0m
}
fn ladder.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = fn_ref $ladder.kernel
    %30 = call %29(%0, %1, %3, %4, %10, %16, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/borrow#7][0m
    %31 = fn_ref $ylc_get_output_buf
    %32 = call %31(%0)[2m : Ptr[0m[2m ; ops [%31:callee/borrow#0, %0:value/borrow#0][0m
    %33 = ptr_offset %32, %3[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %3:index/borrow#1][0m
    %34 = store %33, %30[2m : ()[0m[2m ; ops [%33:value/borrow#0, %30:element/consume#1][0m
    %35 = const.void
    return %35[2m ; ops [%35:return/consume#0][0m
}
fn allpass2.cons(%0 freq[2m: Double @borrow[0m, %1 q[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $allpass2.frame
    %5 = const.string "allpass2"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 32
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $allpass2.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $allpass2.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn allpass2.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $biquad.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn allpass2.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 q[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 2
    %8 = global_load @$global.DSP.DSP_common.Math.PI[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    %10 = fmul %9, %4[2m ; ops [%9:value/borrow#0, %4:value/borrow#1][0m
    %11 = fmul %10, %3[2m ; ops [%10:value/borrow#0, %3:value/borrow#1][0m
    %12 = fn_ref $cos
    %13 = call %12(%11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    %14 = fn_ref $sin
    %15 = call %14(%11)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %11:value/borrow#0][0m
    %16 = const.double 2
    %17 = fmul %16, %5[2m ; ops [%16:value/borrow#0, %5:value/borrow#1][0m
    %18 = fdiv %15, %17[2m ; ops [%15:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.double 1
    %20 = fadd %19, %18[2m ; ops [%19:value/borrow#0, %18:value/borrow#1][0m
    %21 = const.double -2
    %22 = fmul %21, %13[2m ; ops [%21:value/borrow#0, %13:value/borrow#1][0m
    %23 = fdiv %22, %20[2m ; ops [%22:value/borrow#0, %20:value/borrow#1][0m
    %24 = const.double 1
    %25 = fsub %24, %18[2m ; ops [%24:value/borrow#0, %18:value/borrow#1][0m
    %26 = fdiv %25, %20[2m ; ops [%25:value/borrow#0, %20:value/borrow#1][0m
    %27 = const.double 1
    %28 = fn_ref $biquad.kernel
    %29 = const.int 0
    %30 = ptr_offset %1, %29[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Char[2m ; ops [%30:value/borrow#0][0m
    %32 = call %28(%0, %31, %2, %3, %26, %23, %27, %23, %26, %6)[2m : Double[0m[2m ; ops [%28:callee/borrow#0, %0:value/consume#0, %31:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %26:value/borrow#4, %23:value/borrow#5, %27:value/borrow#6, %23:value/borrow#7, %26:value/borrow#8, %6:value/consume#9][0m
    return %32[2m ; ops [%32:return/consume#0][0m
}
fn allpass2.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $allpass2.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn dispersion32.cons(%0 base[2m: Double @borrow[0m, %1 amount[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $dispersion32.frame
    %5 = const.string "dispersion32"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 1024
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $dispersion32.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $dispersion32.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn dispersion32.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 992
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 960
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 928
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 896
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 864
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = const.char '\x00'
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 832
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = const.char '\x00'
    %24 = store %22, %23[2m : ()[0m[2m ; ops [%22:value/borrow#0, %23:element/consume#1][0m
    %25 = const.int 800
    %26 = ptr_offset %0, %25[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %25:index/borrow#1][0m
    %27 = const.char '\x00'
    %28 = store %26, %27[2m : ()[0m[2m ; ops [%26:value/borrow#0, %27:element/consume#1][0m
    %29 = const.int 768
    %30 = ptr_offset %0, %29[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %29:index/borrow#1][0m
    %31 = const.char '\x00'
    %32 = store %30, %31[2m : ()[0m[2m ; ops [%30:value/borrow#0, %31:element/consume#1][0m
    %33 = const.int 736
    %34 = ptr_offset %0, %33[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %33:index/borrow#1][0m
    %35 = const.char '\x00'
    %36 = store %34, %35[2m : ()[0m[2m ; ops [%34:value/borrow#0, %35:element/consume#1][0m
    %37 = const.int 704
    %38 = ptr_offset %0, %37[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %37:index/borrow#1][0m
    %39 = const.char '\x00'
    %40 = store %38, %39[2m : ()[0m[2m ; ops [%38:value/borrow#0, %39:element/consume#1][0m
    %41 = const.int 672
    %42 = ptr_offset %0, %41[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %41:index/borrow#1][0m
    %43 = const.char '\x00'
    %44 = store %42, %43[2m : ()[0m[2m ; ops [%42:value/borrow#0, %43:element/consume#1][0m
    %45 = const.int 640
    %46 = ptr_offset %0, %45[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %45:index/borrow#1][0m
    %47 = const.char '\x00'
    %48 = store %46, %47[2m : ()[0m[2m ; ops [%46:value/borrow#0, %47:element/consume#1][0m
    %49 = const.int 608
    %50 = ptr_offset %0, %49[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %49:index/borrow#1][0m
    %51 = const.char '\x00'
    %52 = store %50, %51[2m : ()[0m[2m ; ops [%50:value/borrow#0, %51:element/consume#1][0m
    %53 = const.int 576
    %54 = ptr_offset %0, %53[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %53:index/borrow#1][0m
    %55 = const.char '\x00'
    %56 = store %54, %55[2m : ()[0m[2m ; ops [%54:value/borrow#0, %55:element/consume#1][0m
    %57 = const.int 544
    %58 = ptr_offset %0, %57[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %57:index/borrow#1][0m
    %59 = const.char '\x00'
    %60 = store %58, %59[2m : ()[0m[2m ; ops [%58:value/borrow#0, %59:element/consume#1][0m
    %61 = const.int 512
    %62 = ptr_offset %0, %61[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %61:index/borrow#1][0m
    %63 = const.char '\x00'
    %64 = store %62, %63[2m : ()[0m[2m ; ops [%62:value/borrow#0, %63:element/consume#1][0m
    %65 = const.int 480
    %66 = ptr_offset %0, %65[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %65:index/borrow#1][0m
    %67 = const.char '\x00'
    %68 = store %66, %67[2m : ()[0m[2m ; ops [%66:value/borrow#0, %67:element/consume#1][0m
    %69 = const.int 448
    %70 = ptr_offset %0, %69[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %69:index/borrow#1][0m
    %71 = const.char '\x00'
    %72 = store %70, %71[2m : ()[0m[2m ; ops [%70:value/borrow#0, %71:element/consume#1][0m
    %73 = const.int 416
    %74 = ptr_offset %0, %73[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %73:index/borrow#1][0m
    %75 = const.char '\x00'
    %76 = store %74, %75[2m : ()[0m[2m ; ops [%74:value/borrow#0, %75:element/consume#1][0m
    %77 = const.int 384
    %78 = ptr_offset %0, %77[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %77:index/borrow#1][0m
    %79 = const.char '\x00'
    %80 = store %78, %79[2m : ()[0m[2m ; ops [%78:value/borrow#0, %79:element/consume#1][0m
    %81 = const.int 352
    %82 = ptr_offset %0, %81[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %81:index/borrow#1][0m
    %83 = const.char '\x00'
    %84 = store %82, %83[2m : ()[0m[2m ; ops [%82:value/borrow#0, %83:element/consume#1][0m
    %85 = const.int 320
    %86 = ptr_offset %0, %85[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %85:index/borrow#1][0m
    %87 = const.char '\x00'
    %88 = store %86, %87[2m : ()[0m[2m ; ops [%86:value/borrow#0, %87:element/consume#1][0m
    %89 = const.int 288
    %90 = ptr_offset %0, %89[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %89:index/borrow#1][0m
    %91 = const.char '\x00'
    %92 = store %90, %91[2m : ()[0m[2m ; ops [%90:value/borrow#0, %91:element/consume#1][0m
    %93 = const.int 256
    %94 = ptr_offset %0, %93[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %93:index/borrow#1][0m
    %95 = const.char '\x00'
    %96 = store %94, %95[2m : ()[0m[2m ; ops [%94:value/borrow#0, %95:element/consume#1][0m
    %97 = const.int 224
    %98 = ptr_offset %0, %97[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %97:index/borrow#1][0m
    %99 = const.char '\x00'
    %100 = store %98, %99[2m : ()[0m[2m ; ops [%98:value/borrow#0, %99:element/consume#1][0m
    %101 = const.int 192
    %102 = ptr_offset %0, %101[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %101:index/borrow#1][0m
    %103 = const.char '\x00'
    %104 = store %102, %103[2m : ()[0m[2m ; ops [%102:value/borrow#0, %103:element/consume#1][0m
    %105 = const.int 160
    %106 = ptr_offset %0, %105[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %105:index/borrow#1][0m
    %107 = const.char '\x00'
    %108 = store %106, %107[2m : ()[0m[2m ; ops [%106:value/borrow#0, %107:element/consume#1][0m
    %109 = const.int 128
    %110 = ptr_offset %0, %109[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %109:index/borrow#1][0m
    %111 = const.char '\x00'
    %112 = store %110, %111[2m : ()[0m[2m ; ops [%110:value/borrow#0, %111:element/consume#1][0m
    %113 = const.int 96
    %114 = ptr_offset %0, %113[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %113:index/borrow#1][0m
    %115 = const.char '\x00'
    %116 = store %114, %115[2m : ()[0m[2m ; ops [%114:value/borrow#0, %115:element/consume#1][0m
    %117 = const.int 64
    %118 = ptr_offset %0, %117[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %117:index/borrow#1][0m
    %119 = const.char '\x00'
    %120 = store %118, %119[2m : ()[0m[2m ; ops [%118:value/borrow#0, %119:element/consume#1][0m
    %121 = const.int 32
    %122 = ptr_offset %0, %121[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %121:index/borrow#1][0m
    %123 = const.char '\x00'
    %124 = store %122, %123[2m : ()[0m[2m ; ops [%122:value/borrow#0, %123:element/consume#1][0m
    %125 = const.int 0
    %126 = ptr_offset %0, %125[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %125:index/borrow#1][0m
    %127 = const.char '\x00'
    %128 = store %126, %127[2m : ()[0m[2m ; ops [%126:value/borrow#0, %127:element/consume#1][0m
    %129 = const.int 0
    %130 = ptr_offset %0, %129[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %129:index/borrow#1][0m
    %131 = fn_ref $dispersion32.int_fold.callback.0.init
    %132 = call %131(%130)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %130:value/borrow#0][0m
    %133 = const.int 32
    %134 = ptr_offset %0, %133[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %133:index/borrow#1][0m
    %135 = call %131(%134)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %134:value/borrow#0][0m
    %136 = const.int 64
    %137 = ptr_offset %0, %136[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %136:index/borrow#1][0m
    %138 = call %131(%137)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %137:value/borrow#0][0m
    %139 = const.int 96
    %140 = ptr_offset %0, %139[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %139:index/borrow#1][0m
    %141 = call %131(%140)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %140:value/borrow#0][0m
    %142 = const.int 128
    %143 = ptr_offset %0, %142[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %142:index/borrow#1][0m
    %144 = call %131(%143)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %143:value/borrow#0][0m
    %145 = const.int 160
    %146 = ptr_offset %0, %145[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %145:index/borrow#1][0m
    %147 = call %131(%146)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %146:value/borrow#0][0m
    %148 = const.int 192
    %149 = ptr_offset %0, %148[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %148:index/borrow#1][0m
    %150 = call %131(%149)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %149:value/borrow#0][0m
    %151 = const.int 224
    %152 = ptr_offset %0, %151[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %151:index/borrow#1][0m
    %153 = call %131(%152)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %152:value/borrow#0][0m
    %154 = const.int 256
    %155 = ptr_offset %0, %154[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %154:index/borrow#1][0m
    %156 = call %131(%155)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %155:value/borrow#0][0m
    %157 = const.int 288
    %158 = ptr_offset %0, %157[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %157:index/borrow#1][0m
    %159 = call %131(%158)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %158:value/borrow#0][0m
    %160 = const.int 320
    %161 = ptr_offset %0, %160[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %160:index/borrow#1][0m
    %162 = call %131(%161)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %161:value/borrow#0][0m
    %163 = const.int 352
    %164 = ptr_offset %0, %163[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %163:index/borrow#1][0m
    %165 = call %131(%164)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %164:value/borrow#0][0m
    %166 = const.int 384
    %167 = ptr_offset %0, %166[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %166:index/borrow#1][0m
    %168 = call %131(%167)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %167:value/borrow#0][0m
    %169 = const.int 416
    %170 = ptr_offset %0, %169[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %169:index/borrow#1][0m
    %171 = call %131(%170)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %170:value/borrow#0][0m
    %172 = const.int 448
    %173 = ptr_offset %0, %172[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %172:index/borrow#1][0m
    %174 = call %131(%173)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %173:value/borrow#0][0m
    %175 = const.int 480
    %176 = ptr_offset %0, %175[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %175:index/borrow#1][0m
    %177 = call %131(%176)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %176:value/borrow#0][0m
    %178 = const.int 512
    %179 = ptr_offset %0, %178[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %178:index/borrow#1][0m
    %180 = call %131(%179)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %179:value/borrow#0][0m
    %181 = const.int 544
    %182 = ptr_offset %0, %181[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %181:index/borrow#1][0m
    %183 = call %131(%182)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %182:value/borrow#0][0m
    %184 = const.int 576
    %185 = ptr_offset %0, %184[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %184:index/borrow#1][0m
    %186 = call %131(%185)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %185:value/borrow#0][0m
    %187 = const.int 608
    %188 = ptr_offset %0, %187[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %187:index/borrow#1][0m
    %189 = call %131(%188)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %188:value/borrow#0][0m
    %190 = const.int 640
    %191 = ptr_offset %0, %190[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %190:index/borrow#1][0m
    %192 = call %131(%191)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %191:value/borrow#0][0m
    %193 = const.int 672
    %194 = ptr_offset %0, %193[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %193:index/borrow#1][0m
    %195 = call %131(%194)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %194:value/borrow#0][0m
    %196 = const.int 704
    %197 = ptr_offset %0, %196[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %196:index/borrow#1][0m
    %198 = call %131(%197)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %197:value/borrow#0][0m
    %199 = const.int 736
    %200 = ptr_offset %0, %199[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %199:index/borrow#1][0m
    %201 = call %131(%200)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %200:value/borrow#0][0m
    %202 = const.int 768
    %203 = ptr_offset %0, %202[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %202:index/borrow#1][0m
    %204 = call %131(%203)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %203:value/borrow#0][0m
    %205 = const.int 800
    %206 = ptr_offset %0, %205[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %205:index/borrow#1][0m
    %207 = call %131(%206)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %206:value/borrow#0][0m
    %208 = const.int 832
    %209 = ptr_offset %0, %208[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %208:index/borrow#1][0m
    %210 = call %131(%209)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %209:value/borrow#0][0m
    %211 = const.int 864
    %212 = ptr_offset %0, %211[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %211:index/borrow#1][0m
    %213 = call %131(%212)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %212:value/borrow#0][0m
    %214 = const.int 896
    %215 = ptr_offset %0, %214[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %214:index/borrow#1][0m
    %216 = call %131(%215)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %215:value/borrow#0][0m
    %217 = const.int 928
    %218 = ptr_offset %0, %217[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %217:index/borrow#1][0m
    %219 = call %131(%218)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %218:value/borrow#0][0m
    %220 = const.int 960
    %221 = ptr_offset %0, %220[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %220:index/borrow#1][0m
    %222 = call %131(%221)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %221:value/borrow#0][0m
    %223 = const.int 992
    %224 = ptr_offset %0, %223[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %223:index/borrow#1][0m
    %225 = call %131(%224)[2m : ()[0m[2m ; ops [%131:callee/borrow#0, %224:value/borrow#0][0m
    %226 = const.void
    return %226[2m ; ops [%226:return/consume#0][0m
}
fn dispersion32.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 base[2m: Double @borrow[0m, %5 amount[2m: Double @borrow[0m, %6 x[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.int 0
    %8 = fn_ref $dispersion32.int_fold.callback.0.kernel
    %9 = const.int 0
    %10 = ptr_offset %1, %9[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %9:index/borrow#1][0m
    %11 = primitive_cast %10 from Ptr of Char to Ptr of Char[2m ; ops [%10:value/borrow#0][0m
    %12 = call %8(%0, %11, %2, %3, %5, %4, %7, %6)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %11:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %7:value/borrow#6, %6:value/consume#7][0m
    %13 = const.int 1
    %14 = const.int 32
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Char[2m ; ops [%15:value/borrow#0][0m
    %17 = call %8(%0, %16, %2, %3, %5, %4, %13, %12)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %16:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %13:value/borrow#6, %12:value/consume#7][0m
    %18 = const.int 2
    %19 = const.int 64
    %20 = ptr_offset %1, %19[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %19:index/borrow#1][0m
    %21 = primitive_cast %20 from Ptr of Char to Ptr of Char[2m ; ops [%20:value/borrow#0][0m
    %22 = call %8(%0, %21, %2, %3, %5, %4, %18, %17)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %21:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %18:value/borrow#6, %17:value/consume#7][0m
    %23 = const.int 3
    %24 = const.int 96
    %25 = ptr_offset %1, %24[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Char[2m ; ops [%25:value/borrow#0][0m
    %27 = call %8(%0, %26, %2, %3, %5, %4, %23, %22)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %26:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %23:value/borrow#6, %22:value/consume#7][0m
    %28 = const.int 4
    %29 = const.int 128
    %30 = ptr_offset %1, %29[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Char[2m ; ops [%30:value/borrow#0][0m
    %32 = call %8(%0, %31, %2, %3, %5, %4, %28, %27)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %31:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %28:value/borrow#6, %27:value/consume#7][0m
    %33 = const.int 5
    %34 = const.int 160
    %35 = ptr_offset %1, %34[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %34:index/borrow#1][0m
    %36 = primitive_cast %35 from Ptr of Char to Ptr of Char[2m ; ops [%35:value/borrow#0][0m
    %37 = call %8(%0, %36, %2, %3, %5, %4, %33, %32)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %36:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %33:value/borrow#6, %32:value/consume#7][0m
    %38 = const.int 6
    %39 = const.int 192
    %40 = ptr_offset %1, %39[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %39:index/borrow#1][0m
    %41 = primitive_cast %40 from Ptr of Char to Ptr of Char[2m ; ops [%40:value/borrow#0][0m
    %42 = call %8(%0, %41, %2, %3, %5, %4, %38, %37)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %41:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %38:value/borrow#6, %37:value/consume#7][0m
    %43 = const.int 7
    %44 = const.int 224
    %45 = ptr_offset %1, %44[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %44:index/borrow#1][0m
    %46 = primitive_cast %45 from Ptr of Char to Ptr of Char[2m ; ops [%45:value/borrow#0][0m
    %47 = call %8(%0, %46, %2, %3, %5, %4, %43, %42)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %46:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %43:value/borrow#6, %42:value/consume#7][0m
    %48 = const.int 8
    %49 = const.int 256
    %50 = ptr_offset %1, %49[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %49:index/borrow#1][0m
    %51 = primitive_cast %50 from Ptr of Char to Ptr of Char[2m ; ops [%50:value/borrow#0][0m
    %52 = call %8(%0, %51, %2, %3, %5, %4, %48, %47)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %51:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %48:value/borrow#6, %47:value/consume#7][0m
    %53 = const.int 9
    %54 = const.int 288
    %55 = ptr_offset %1, %54[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %54:index/borrow#1][0m
    %56 = primitive_cast %55 from Ptr of Char to Ptr of Char[2m ; ops [%55:value/borrow#0][0m
    %57 = call %8(%0, %56, %2, %3, %5, %4, %53, %52)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %56:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %53:value/borrow#6, %52:value/consume#7][0m
    %58 = const.int 10
    %59 = const.int 320
    %60 = ptr_offset %1, %59[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %59:index/borrow#1][0m
    %61 = primitive_cast %60 from Ptr of Char to Ptr of Char[2m ; ops [%60:value/borrow#0][0m
    %62 = call %8(%0, %61, %2, %3, %5, %4, %58, %57)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %61:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %58:value/borrow#6, %57:value/consume#7][0m
    %63 = const.int 11
    %64 = const.int 352
    %65 = ptr_offset %1, %64[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %64:index/borrow#1][0m
    %66 = primitive_cast %65 from Ptr of Char to Ptr of Char[2m ; ops [%65:value/borrow#0][0m
    %67 = call %8(%0, %66, %2, %3, %5, %4, %63, %62)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %66:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %63:value/borrow#6, %62:value/consume#7][0m
    %68 = const.int 12
    %69 = const.int 384
    %70 = ptr_offset %1, %69[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %69:index/borrow#1][0m
    %71 = primitive_cast %70 from Ptr of Char to Ptr of Char[2m ; ops [%70:value/borrow#0][0m
    %72 = call %8(%0, %71, %2, %3, %5, %4, %68, %67)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %71:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %68:value/borrow#6, %67:value/consume#7][0m
    %73 = const.int 13
    %74 = const.int 416
    %75 = ptr_offset %1, %74[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %74:index/borrow#1][0m
    %76 = primitive_cast %75 from Ptr of Char to Ptr of Char[2m ; ops [%75:value/borrow#0][0m
    %77 = call %8(%0, %76, %2, %3, %5, %4, %73, %72)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %76:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %73:value/borrow#6, %72:value/consume#7][0m
    %78 = const.int 14
    %79 = const.int 448
    %80 = ptr_offset %1, %79[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %79:index/borrow#1][0m
    %81 = primitive_cast %80 from Ptr of Char to Ptr of Char[2m ; ops [%80:value/borrow#0][0m
    %82 = call %8(%0, %81, %2, %3, %5, %4, %78, %77)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %81:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %78:value/borrow#6, %77:value/consume#7][0m
    %83 = const.int 15
    %84 = const.int 480
    %85 = ptr_offset %1, %84[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %84:index/borrow#1][0m
    %86 = primitive_cast %85 from Ptr of Char to Ptr of Char[2m ; ops [%85:value/borrow#0][0m
    %87 = call %8(%0, %86, %2, %3, %5, %4, %83, %82)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %86:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %83:value/borrow#6, %82:value/consume#7][0m
    %88 = const.int 16
    %89 = const.int 512
    %90 = ptr_offset %1, %89[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %89:index/borrow#1][0m
    %91 = primitive_cast %90 from Ptr of Char to Ptr of Char[2m ; ops [%90:value/borrow#0][0m
    %92 = call %8(%0, %91, %2, %3, %5, %4, %88, %87)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %91:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %88:value/borrow#6, %87:value/consume#7][0m
    %93 = const.int 17
    %94 = const.int 544
    %95 = ptr_offset %1, %94[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %94:index/borrow#1][0m
    %96 = primitive_cast %95 from Ptr of Char to Ptr of Char[2m ; ops [%95:value/borrow#0][0m
    %97 = call %8(%0, %96, %2, %3, %5, %4, %93, %92)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %96:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %93:value/borrow#6, %92:value/consume#7][0m
    %98 = const.int 18
    %99 = const.int 576
    %100 = ptr_offset %1, %99[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %99:index/borrow#1][0m
    %101 = primitive_cast %100 from Ptr of Char to Ptr of Char[2m ; ops [%100:value/borrow#0][0m
    %102 = call %8(%0, %101, %2, %3, %5, %4, %98, %97)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %101:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %98:value/borrow#6, %97:value/consume#7][0m
    %103 = const.int 19
    %104 = const.int 608
    %105 = ptr_offset %1, %104[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %104:index/borrow#1][0m
    %106 = primitive_cast %105 from Ptr of Char to Ptr of Char[2m ; ops [%105:value/borrow#0][0m
    %107 = call %8(%0, %106, %2, %3, %5, %4, %103, %102)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %106:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %103:value/borrow#6, %102:value/consume#7][0m
    %108 = const.int 20
    %109 = const.int 640
    %110 = ptr_offset %1, %109[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %109:index/borrow#1][0m
    %111 = primitive_cast %110 from Ptr of Char to Ptr of Char[2m ; ops [%110:value/borrow#0][0m
    %112 = call %8(%0, %111, %2, %3, %5, %4, %108, %107)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %111:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %108:value/borrow#6, %107:value/consume#7][0m
    %113 = const.int 21
    %114 = const.int 672
    %115 = ptr_offset %1, %114[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %114:index/borrow#1][0m
    %116 = primitive_cast %115 from Ptr of Char to Ptr of Char[2m ; ops [%115:value/borrow#0][0m
    %117 = call %8(%0, %116, %2, %3, %5, %4, %113, %112)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %116:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %113:value/borrow#6, %112:value/consume#7][0m
    %118 = const.int 22
    %119 = const.int 704
    %120 = ptr_offset %1, %119[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %119:index/borrow#1][0m
    %121 = primitive_cast %120 from Ptr of Char to Ptr of Char[2m ; ops [%120:value/borrow#0][0m
    %122 = call %8(%0, %121, %2, %3, %5, %4, %118, %117)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %121:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %118:value/borrow#6, %117:value/consume#7][0m
    %123 = const.int 23
    %124 = const.int 736
    %125 = ptr_offset %1, %124[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %124:index/borrow#1][0m
    %126 = primitive_cast %125 from Ptr of Char to Ptr of Char[2m ; ops [%125:value/borrow#0][0m
    %127 = call %8(%0, %126, %2, %3, %5, %4, %123, %122)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %126:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %123:value/borrow#6, %122:value/consume#7][0m
    %128 = const.int 24
    %129 = const.int 768
    %130 = ptr_offset %1, %129[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %129:index/borrow#1][0m
    %131 = primitive_cast %130 from Ptr of Char to Ptr of Char[2m ; ops [%130:value/borrow#0][0m
    %132 = call %8(%0, %131, %2, %3, %5, %4, %128, %127)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %131:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %128:value/borrow#6, %127:value/consume#7][0m
    %133 = const.int 25
    %134 = const.int 800
    %135 = ptr_offset %1, %134[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %134:index/borrow#1][0m
    %136 = primitive_cast %135 from Ptr of Char to Ptr of Char[2m ; ops [%135:value/borrow#0][0m
    %137 = call %8(%0, %136, %2, %3, %5, %4, %133, %132)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %136:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %133:value/borrow#6, %132:value/consume#7][0m
    %138 = const.int 26
    %139 = const.int 832
    %140 = ptr_offset %1, %139[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %139:index/borrow#1][0m
    %141 = primitive_cast %140 from Ptr of Char to Ptr of Char[2m ; ops [%140:value/borrow#0][0m
    %142 = call %8(%0, %141, %2, %3, %5, %4, %138, %137)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %141:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %138:value/borrow#6, %137:value/consume#7][0m
    %143 = const.int 27
    %144 = const.int 864
    %145 = ptr_offset %1, %144[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %144:index/borrow#1][0m
    %146 = primitive_cast %145 from Ptr of Char to Ptr of Char[2m ; ops [%145:value/borrow#0][0m
    %147 = call %8(%0, %146, %2, %3, %5, %4, %143, %142)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %146:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %143:value/borrow#6, %142:value/consume#7][0m
    %148 = const.int 28
    %149 = const.int 896
    %150 = ptr_offset %1, %149[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %149:index/borrow#1][0m
    %151 = primitive_cast %150 from Ptr of Char to Ptr of Char[2m ; ops [%150:value/borrow#0][0m
    %152 = call %8(%0, %151, %2, %3, %5, %4, %148, %147)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %151:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %148:value/borrow#6, %147:value/consume#7][0m
    %153 = const.int 29
    %154 = const.int 928
    %155 = ptr_offset %1, %154[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %154:index/borrow#1][0m
    %156 = primitive_cast %155 from Ptr of Char to Ptr of Char[2m ; ops [%155:value/borrow#0][0m
    %157 = call %8(%0, %156, %2, %3, %5, %4, %153, %152)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %156:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %153:value/borrow#6, %152:value/consume#7][0m
    %158 = const.int 30
    %159 = const.int 960
    %160 = ptr_offset %1, %159[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %159:index/borrow#1][0m
    %161 = primitive_cast %160 from Ptr of Char to Ptr of Char[2m ; ops [%160:value/borrow#0][0m
    %162 = call %8(%0, %161, %2, %3, %5, %4, %158, %157)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %161:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %158:value/borrow#6, %157:value/consume#7][0m
    %163 = const.int 31
    %164 = const.int 992
    %165 = ptr_offset %1, %164[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %164:index/borrow#1][0m
    %166 = primitive_cast %165 from Ptr of Char to Ptr of Char[2m ; ops [%165:value/borrow#0][0m
    %167 = call %8(%0, %166, %2, %3, %5, %4, %163, %162)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %166:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/borrow#4, %4:value/borrow#5, %163:value/borrow#6, %162:value/consume#7][0m
    return %167[2m ; ops [%167:return/consume#0][0m
}
fn dispersion32.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $dispersion32.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/consume#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn dispersion32.int_fold.callback.0.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn dispersion32.int_fold.callback.0.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $allpass2.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn dispersion32.int_fold.callback.0.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 amount[2m: Double @borrow[0m, %5 base[2m: Double @borrow[0m, %6 i[2m: Int @borrow[0m, %7 acc[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %8 = const.int 1
    %9 = primitive_cast %6 from Int to Double[2m ; ops [%6:value/borrow#0][0m
    %10 = primitive_cast %8 from Int to Double[2m ; ops [%8:value/borrow#0][0m
    %11 = fadd %9, %10[2m ; ops [%9:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $pow
    %13 = const.double 1.18
    %14 = call %12(%13, %11)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %13:value/borrow#0, %11:value/borrow#1][0m
    %15 = fmul %5, %14[2m ; ops [%5:value/borrow#0, %14:value/borrow#1][0m
    %16 = const.double 1.2
    %17 = const.double 24
    %18 = fmul %4, %17[2m ; ops [%4:value/borrow#0, %17:value/borrow#1][0m
    %19 = fdiv %18, %11[2m ; ops [%18:value/borrow#0, %11:value/borrow#1][0m
    %20 = fadd %16, %19[2m ; ops [%16:value/borrow#0, %19:value/borrow#1][0m
    %21 = fn_ref $allpass2.kernel
    %22 = const.int 0
    %23 = ptr_offset %1, %22[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %22:index/borrow#1][0m
    %24 = primitive_cast %23 from Ptr of Char to Ptr of Char[2m ; ops [%23:value/borrow#0][0m
    %25 = call %21(%0, %24, %2, %3, %15, %20, %7)[2m : Double[0m[2m ; ops [%21:callee/borrow#0, %0:value/consume#0, %24:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %15:value/borrow#4, %20:value/borrow#5, %7:value/consume#6][0m
    return %25[2m ; ops [%25:return/consume#0][0m
}
extern fn moogff(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn limiter(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn white.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $white.frame
    %2 = const.string "white"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 0
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $white.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $white.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/consume#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn white.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn white.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = global_load @$global.DSP.DSP_common.Math.rrand[2m : (Double -> (Double -> Double))[0m
    %5 = const.double -1
    %6 = const.double 1
    %7 = call %4(%5, %6)[2m : Double[0m[2m ; ops [%4:callee/borrow#0, %5:value/consume#0, %6:value/consume#1][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn white.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $white.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/consume#0, %1:value/consume#1, %3:value/consume#2, %4:value/consume#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn pink.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $pink.frame
    %2 = const.string "pink"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 56
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $pink.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $pink.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn pink.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 48
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 40
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 32
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 24
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.int 16
    %27 = ptr_offset %0, %26[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = const.double 0
    %30 = store %28, %29[2m : ()[0m[2m ; ops [%28:value/borrow#0, %29:element/consume#1][0m
    %31 = const.int 8
    %32 = ptr_offset %0, %31[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %31:index/borrow#1][0m
    %33 = primitive_cast %32 from Ptr of Char to Ptr of Double[2m ; ops [%32:value/borrow#0][0m
    %34 = const.double 0
    %35 = store %33, %34[2m : ()[0m[2m ; ops [%33:value/borrow#0, %34:element/consume#1][0m
    %36 = const.int 0
    %37 = ptr_offset %0, %36[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %36:index/borrow#1][0m
    %38 = primitive_cast %37 from Ptr of Char to Ptr of Double[2m ; ops [%37:value/borrow#0][0m
    %39 = const.double 0
    %40 = store %38, %39[2m : ()[0m[2m ; ops [%38:value/borrow#0, %39:element/consume#1][0m
    %41 = const.void
    return %41[2m ; ops [%41:return/consume#0][0m
}
fn pink.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = const.int 0
    %5 = ptr_offset %1, %4[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %4:index/borrow#1][0m
    %6 = primitive_cast %5 from Ptr of Char to Ptr of Double[2m ; ops [%5:value/borrow#0][0m
    %7 = const.int 7
    %8 = const.int 0
    %9 = construct.tuple { %7, %8, %6 }[2m : Array of Double[0m[2m ; ops [%7:field/borrow#0, %8:field/borrow#1, %6:field/borrow#2][0m
    %10 = global_load @$global.DSP.DSP_common.Math.rrand[2m : (Double -> (Double -> Double))[0m
    %11 = const.double -1
    %12 = const.double 1
    %13 = call %10(%11, %12)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %11:value/consume#0, %12:value/consume#1][0m
    %14 = const.double 0.99886
    %15 = const.int 0
    %16 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %17 = iadd %16, %15[2m ; ops [%16:value/borrow#0, %15:value/borrow#1][0m
    %18 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %19 = ptr_offset %18, %17[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %17:index/borrow#1][0m
    %20 = load %19[2m : Double[0m[2m ; ops [%19:value/borrow#0][0m
    %21 = fmul %14, %20[2m ; ops [%14:value/borrow#0, %20:value/borrow#1][0m
    %22 = const.double 0.0555179
    %23 = fmul %13, %22[2m ; ops [%13:value/borrow#0, %22:value/borrow#1][0m
    %24 = fadd %21, %23[2m ; ops [%21:value/borrow#0, %23:value/borrow#1][0m
    %25 = const.double 0.99332
    %26 = const.int 1
    %27 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %28 = iadd %27, %26[2m ; ops [%27:value/borrow#0, %26:value/borrow#1][0m
    %29 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %30 = ptr_offset %29, %28[2m : Ptr[0m[2m ; ops [%29:value/borrow#0, %28:index/borrow#1][0m
    %31 = load %30[2m : Double[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fmul %25, %31[2m ; ops [%25:value/borrow#0, %31:value/borrow#1][0m
    %33 = const.double 0.0750759
    %34 = fmul %13, %33[2m ; ops [%13:value/borrow#0, %33:value/borrow#1][0m
    %35 = fadd %32, %34[2m ; ops [%32:value/borrow#0, %34:value/borrow#1][0m
    %36 = const.double 0.969
    %37 = const.int 2
    %38 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %39 = iadd %38, %37[2m ; ops [%38:value/borrow#0, %37:value/borrow#1][0m
    %40 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %41 = ptr_offset %40, %39[2m : Ptr[0m[2m ; ops [%40:value/borrow#0, %39:index/borrow#1][0m
    %42 = load %41[2m : Double[0m[2m ; ops [%41:value/borrow#0][0m
    %43 = fmul %36, %42[2m ; ops [%36:value/borrow#0, %42:value/borrow#1][0m
    %44 = const.double 0.153852
    %45 = fmul %13, %44[2m ; ops [%13:value/borrow#0, %44:value/borrow#1][0m
    %46 = fadd %43, %45[2m ; ops [%43:value/borrow#0, %45:value/borrow#1][0m
    %47 = const.double 0.8665
    %48 = const.int 3
    %49 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %50 = iadd %49, %48[2m ; ops [%49:value/borrow#0, %48:value/borrow#1][0m
    %51 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %52 = ptr_offset %51, %50[2m : Ptr[0m[2m ; ops [%51:value/borrow#0, %50:index/borrow#1][0m
    %53 = load %52[2m : Double[0m[2m ; ops [%52:value/borrow#0][0m
    %54 = fmul %47, %53[2m ; ops [%47:value/borrow#0, %53:value/borrow#1][0m
    %55 = const.double 0.310486
    %56 = fmul %13, %55[2m ; ops [%13:value/borrow#0, %55:value/borrow#1][0m
    %57 = fadd %54, %56[2m ; ops [%54:value/borrow#0, %56:value/borrow#1][0m
    %58 = const.double 0.55
    %59 = const.int 4
    %60 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %61 = iadd %60, %59[2m ; ops [%60:value/borrow#0, %59:value/borrow#1][0m
    %62 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %63 = ptr_offset %62, %61[2m : Ptr[0m[2m ; ops [%62:value/borrow#0, %61:index/borrow#1][0m
    %64 = load %63[2m : Double[0m[2m ; ops [%63:value/borrow#0][0m
    %65 = fmul %58, %64[2m ; ops [%58:value/borrow#0, %64:value/borrow#1][0m
    %66 = const.double 0.532952
    %67 = fmul %13, %66[2m ; ops [%13:value/borrow#0, %66:value/borrow#1][0m
    %68 = fadd %65, %67[2m ; ops [%65:value/borrow#0, %67:value/borrow#1][0m
    %69 = const.double -0.7616
    %70 = const.int 5
    %71 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %72 = iadd %71, %70[2m ; ops [%71:value/borrow#0, %70:value/borrow#1][0m
    %73 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %74 = ptr_offset %73, %72[2m : Ptr[0m[2m ; ops [%73:value/borrow#0, %72:index/borrow#1][0m
    %75 = load %74[2m : Double[0m[2m ; ops [%74:value/borrow#0][0m
    %76 = fmul %69, %75[2m ; ops [%69:value/borrow#0, %75:value/borrow#1][0m
    %77 = const.double 0.016898
    %78 = fmul %13, %77[2m ; ops [%13:value/borrow#0, %77:value/borrow#1][0m
    %79 = fsub %76, %78[2m ; ops [%76:value/borrow#0, %78:value/borrow#1][0m
    %80 = fadd %24, %35[2m ; ops [%24:value/borrow#0, %35:value/borrow#1][0m
    %81 = fadd %80, %46[2m ; ops [%80:value/borrow#0, %46:value/borrow#1][0m
    %82 = fadd %81, %57[2m ; ops [%81:value/borrow#0, %57:value/borrow#1][0m
    %83 = fadd %82, %68[2m ; ops [%82:value/borrow#0, %68:value/borrow#1][0m
    %84 = fadd %83, %79[2m ; ops [%83:value/borrow#0, %79:value/borrow#1][0m
    %85 = const.int 6
    %86 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %87 = iadd %86, %85[2m ; ops [%86:value/borrow#0, %85:value/borrow#1][0m
    %88 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %89 = ptr_offset %88, %87[2m : Ptr[0m[2m ; ops [%88:value/borrow#0, %87:index/borrow#1][0m
    %90 = load %89[2m : Double[0m[2m ; ops [%89:value/borrow#0][0m
    %91 = fadd %84, %90[2m ; ops [%84:value/borrow#0, %90:value/borrow#1][0m
    %92 = const.double 0.5362
    %93 = fmul %13, %92[2m ; ops [%13:value/borrow#0, %92:value/borrow#1][0m
    %94 = fadd %91, %93[2m ; ops [%91:value/borrow#0, %93:value/borrow#1][0m
    %95 = const.double 0.11
    %96 = fmul %94, %95[2m ; ops [%94:value/borrow#0, %95:value/borrow#1][0m
    %97 = const.int 0
    %98 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %99 = iadd %98, %97[2m ; ops [%98:value/borrow#0, %97:value/borrow#1][0m
    %100 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %101 = ptr_offset %100, %99[2m : Ptr[0m[2m ; ops [%100:value/borrow#0, %99:index/borrow#1][0m
    %102 = store %101, %24[2m : ()[0m[2m ; ops [%101:value/borrow#0, %24:element/consume#1][0m
    %103 = const.int 1
    %104 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %105 = iadd %104, %103[2m ; ops [%104:value/borrow#0, %103:value/borrow#1][0m
    %106 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %107 = ptr_offset %106, %105[2m : Ptr[0m[2m ; ops [%106:value/borrow#0, %105:index/borrow#1][0m
    %108 = store %107, %35[2m : ()[0m[2m ; ops [%107:value/borrow#0, %35:element/consume#1][0m
    %109 = const.int 2
    %110 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %111 = iadd %110, %109[2m ; ops [%110:value/borrow#0, %109:value/borrow#1][0m
    %112 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %113 = ptr_offset %112, %111[2m : Ptr[0m[2m ; ops [%112:value/borrow#0, %111:index/borrow#1][0m
    %114 = store %113, %46[2m : ()[0m[2m ; ops [%113:value/borrow#0, %46:element/consume#1][0m
    %115 = const.int 3
    %116 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %117 = iadd %116, %115[2m ; ops [%116:value/borrow#0, %115:value/borrow#1][0m
    %118 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %119 = ptr_offset %118, %117[2m : Ptr[0m[2m ; ops [%118:value/borrow#0, %117:index/borrow#1][0m
    %120 = store %119, %57[2m : ()[0m[2m ; ops [%119:value/borrow#0, %57:element/consume#1][0m
    %121 = const.int 4
    %122 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %123 = iadd %122, %121[2m ; ops [%122:value/borrow#0, %121:value/borrow#1][0m
    %124 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %125 = ptr_offset %124, %123[2m : Ptr[0m[2m ; ops [%124:value/borrow#0, %123:index/borrow#1][0m
    %126 = store %125, %68[2m : ()[0m[2m ; ops [%125:value/borrow#0, %68:element/consume#1][0m
    %127 = const.int 5
    %128 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %129 = iadd %128, %127[2m ; ops [%128:value/borrow#0, %127:value/borrow#1][0m
    %130 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %131 = ptr_offset %130, %129[2m : Ptr[0m[2m ; ops [%130:value/borrow#0, %129:index/borrow#1][0m
    %132 = store %131, %79[2m : ()[0m[2m ; ops [%131:value/borrow#0, %79:element/consume#1][0m
    %133 = const.int 6
    %134 = const.double 0.115926
    %135 = fmul %13, %134[2m ; ops [%13:value/borrow#0, %134:value/borrow#1][0m
    %136 = extract.field %9, 1[2m : Int[0m[2m ; ops [%9:container/borrow#0][0m
    %137 = iadd %136, %133[2m ; ops [%136:value/borrow#0, %133:value/borrow#1][0m
    %138 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %139 = ptr_offset %138, %137[2m : Ptr[0m[2m ; ops [%138:value/borrow#0, %137:index/borrow#1][0m
    %140 = store %139, %135[2m : ()[0m[2m ; ops [%139:value/borrow#0, %135:element/consume#1][0m
    return %96[2m ; ops [%96:return/consume#0][0m
}
fn pink.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $pink.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn $DSP.DSP_common.Reverb_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $schroeder.cons
    %1 = fn_ref $early_refl.cons
    %2 = fn_ref $reverb.cons
    %3 = fn_ref $plate.cons
    %4 = fn_ref $absorb_comb.cons
    %5 = fn_ref $matrix_vec_mul_double
    %6 = fn_ref $vec_dot_double
    %7 = fn_ref $fdn4.cons
    %8 = fn_ref $room_fdn4.cons
    %9 = fn_ref $concert_fdn4.cons
    %10 = fn_ref $cathedral_fdn4.cons
    %11 = fn_ref $fdn8.cons
    %12 = fn_ref $room.cons
    %13 = fn_ref $concert.cons
    %14 = fn_ref $cathedral.cons
    %15 = fn_ref $mega_cathedral.cons
    %16 = const.void
    return %16[2m ; ops [%16:return/consume#0][0m
}
fn schroeder.cons(%0 room[2m: Double @borrow[0m, %1 damp[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $schroeder.frame
    %5 = const.string "schroeder"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 6182544
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $schroeder.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $schroeder.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn schroeder.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 6163320
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = fn_ref $ylc_audio_memzero
    %4 = const.int 19224
    %5 = call %3(%2, %4)[2m : ()[0m[2m ; ops [%3:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.int 6144096
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = fn_ref $ylc_audio_memzero
    %9 = const.int 19224
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = const.int 4608072
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = const.char '\x00'
    %14 = store %12, %13[2m : ()[0m[2m ; ops [%12:value/borrow#0, %13:element/consume#1][0m
    %15 = const.int 3072048
    %16 = ptr_offset %0, %15[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %15:index/borrow#1][0m
    %17 = const.char '\x00'
    %18 = store %16, %17[2m : ()[0m[2m ; ops [%16:value/borrow#0, %17:element/consume#1][0m
    %19 = const.int 1536024
    %20 = ptr_offset %0, %19[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %19:index/borrow#1][0m
    %21 = const.char '\x00'
    %22 = store %20, %21[2m : ()[0m[2m ; ops [%20:value/borrow#0, %21:element/consume#1][0m
    %23 = const.int 0
    %24 = ptr_offset %0, %23[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %23:index/borrow#1][0m
    %25 = const.char '\x00'
    %26 = store %24, %25[2m : ()[0m[2m ; ops [%24:value/borrow#0, %25:element/consume#1][0m
    %27 = const.int 0
    %28 = ptr_offset %0, %27[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %27:index/borrow#1][0m
    %29 = fn_ref $schroeder.list_foldi.callback.1.init
    %30 = call %29(%28)[2m : ()[0m[2m ; ops [%29:callee/borrow#0, %28:value/borrow#0][0m
    %31 = const.int 1536024
    %32 = ptr_offset %0, %31[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %31:index/borrow#1][0m
    %33 = call %29(%32)[2m : ()[0m[2m ; ops [%29:callee/borrow#0, %32:value/borrow#0][0m
    %34 = const.int 3072048
    %35 = ptr_offset %0, %34[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %34:index/borrow#1][0m
    %36 = call %29(%35)[2m : ()[0m[2m ; ops [%29:callee/borrow#0, %35:value/borrow#0][0m
    %37 = const.int 4608072
    %38 = ptr_offset %0, %37[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %37:index/borrow#1][0m
    %39 = call %29(%38)[2m : ()[0m[2m ; ops [%29:callee/borrow#0, %38:value/borrow#0][0m
    %40 = const.void
    return %40[2m ; ops [%40:return/consume#0][0m
}
fn schroeder.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 room[2m: Double @borrow[0m, %5 damp[2m: Double @borrow[0m, %6 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 0.25
    %8 = const.double 0
    %9 = const.int 0
    %10 = const.double 0.0297
    %11 = fn_ref $schroeder.list_foldi.callback.1.kernel
    %12 = const.int 0
    %13 = ptr_offset %1, %12[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %12:index/borrow#1][0m
    %14 = primitive_cast %13 from Ptr of Char to Ptr of Char[2m ; ops [%13:value/borrow#0][0m
    %15 = call %11(%0, %14, %2, %3, %6, %5, %4, %9, %8, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %14:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %6:value/borrow#4, %5:value/borrow#5, %4:value/borrow#6, %9:value/consume#7, %8:value/borrow#8, %10:value/borrow#9][0m
    %16 = const.int 1
    %17 = const.double 0.0371
    %18 = const.int 1536024
    %19 = ptr_offset %1, %18[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %18:index/borrow#1][0m
    %20 = primitive_cast %19 from Ptr of Char to Ptr of Char[2m ; ops [%19:value/borrow#0][0m
    %21 = call %11(%0, %20, %2, %3, %6, %5, %4, %16, %15, %17)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %20:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %6:value/borrow#4, %5:value/borrow#5, %4:value/borrow#6, %16:value/consume#7, %15:value/borrow#8, %17:value/borrow#9][0m
    %22 = const.int 2
    %23 = const.double 0.0411
    %24 = const.int 3072048
    %25 = ptr_offset %1, %24[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Char[2m ; ops [%25:value/borrow#0][0m
    %27 = call %11(%0, %26, %2, %3, %6, %5, %4, %22, %21, %23)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %26:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %6:value/borrow#4, %5:value/borrow#5, %4:value/borrow#6, %22:value/consume#7, %21:value/borrow#8, %23:value/borrow#9][0m
    %28 = const.int 3
    %29 = const.double 0.0437
    %30 = const.int 4608072
    %31 = ptr_offset %1, %30[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %30:index/borrow#1][0m
    %32 = primitive_cast %31 from Ptr of Char to Ptr of Char[2m ; ops [%31:value/borrow#0][0m
    %33 = call %11(%0, %32, %2, %3, %6, %5, %4, %28, %27, %29)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %32:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %6:value/borrow#4, %5:value/borrow#5, %4:value/borrow#6, %28:value/consume#7, %27:value/borrow#8, %29:value/borrow#9][0m
    %34 = fmul %7, %33[2m ; ops [%7:value/borrow#0, %33:value/borrow#1][0m
    %35 = const.double 0.0017
    %36 = const.double 0.05
    %37 = const.double 0.5
    %38 = const.double 0.005
    %39 = const.double 0.05
    %40 = const.double 0.5
    %41 = fn_ref $ylc_audio_dl_allpass_kernel
    %42 = const.int 6144096
    %43 = ptr_offset %1, %42[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %42:index/borrow#1][0m
    %44 = const.int 2402
    %45 = call %41(%43, %3, %44, %38, %40, %34)[2m : Double[0m[2m ; ops [%41:callee/borrow#0, %43:value/borrow#0, %3:value/borrow#1, %44:value/borrow#2, %38:value/borrow#3, %40:value/borrow#4, %34:value/borrow#5][0m
    %46 = fn_ref $ylc_audio_dl_allpass_kernel
    %47 = const.int 6163320
    %48 = ptr_offset %1, %47[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %47:index/borrow#1][0m
    %49 = const.int 2402
    %50 = call %46(%48, %3, %49, %35, %37, %45)[2m : Double[0m[2m ; ops [%46:callee/borrow#0, %48:value/borrow#0, %3:value/borrow#1, %49:value/borrow#2, %35:value/borrow#3, %37:value/borrow#4, %45:value/borrow#5][0m
    return %50[2m ; ops [%50:return/consume#0][0m
}
fn schroeder.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $schroeder.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn schroeder.list_foldi.callback.1.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn schroeder.list_foldi.callback.1.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = fn_ref $ylc_audio_memzero
    %4 = const.int 1536024
    %5 = call %3(%2, %4)[2m : ()[0m[2m ; ops [%3:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn schroeder.list_foldi.callback.1.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 x[2m: Double @borrow[0m, %5 fb[2m: Double @borrow[0m, %6 room_scale[2m: Double @borrow[0m, %7 i[2m: Int @consume[0m, %8 acc[2m: Double @borrow[0m, %9 t[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = fmul %9, %6[2m ; ops [%9:value/borrow#0, %6:value/borrow#1][0m
    %11 = const.double 4
    %12 = fn_ref $ylc_audio_comb_kernel
    %13 = const.int 0
    %14 = ptr_offset %1, %13[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.int 192002
    %16 = call %12(%14, %3, %15, %10, %5, %4)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %14:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2, %10:value/borrow#3, %5:value/borrow#4, %4:value/borrow#5][0m
    %17 = fadd %8, %16[2m ; ops [%8:value/borrow#0, %16:value/borrow#1][0m
    return %17[2m ; ops [%17:return/consume#0][0m
}
extern fn ylc_audio_comb_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_memzero(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Int @borrow[0m)[2m -> () @none[0m;
extern fn ylc_audio_dl_allpass_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn early_refl.cons(%0 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $early_refl.frame
    %3 = const.string "early_refl"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 230544
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $early_refl.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $early_refl.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn early_refl.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 192120
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 153696
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 115272
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 76848
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 38424
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = const.char '\x00'
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 0
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = const.char '\x00'
    %24 = store %22, %23[2m : ()[0m[2m ; ops [%22:value/borrow#0, %23:element/consume#1][0m
    %25 = const.int 0
    %26 = ptr_offset %0, %25[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %25:index/borrow#1][0m
    %27 = fn_ref $early_refl.list_fold.callback.2.init
    %28 = call %27(%26)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %26:value/borrow#0][0m
    %29 = const.int 38424
    %30 = ptr_offset %0, %29[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %29:index/borrow#1][0m
    %31 = call %27(%30)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %30:value/borrow#0][0m
    %32 = const.int 76848
    %33 = ptr_offset %0, %32[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %32:index/borrow#1][0m
    %34 = call %27(%33)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %33:value/borrow#0][0m
    %35 = const.int 115272
    %36 = ptr_offset %0, %35[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %35:index/borrow#1][0m
    %37 = call %27(%36)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %36:value/borrow#0][0m
    %38 = const.int 153696
    %39 = ptr_offset %0, %38[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %38:index/borrow#1][0m
    %40 = call %27(%39)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %39:value/borrow#0][0m
    %41 = const.int 192120
    %42 = ptr_offset %0, %41[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %41:index/borrow#1][0m
    %43 = call %27(%42)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %42:value/borrow#0][0m
    %44 = const.void
    return %44[2m ; ops [%44:return/consume#0][0m
}
fn early_refl.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.167
    %6 = const.double 0
    %7 = const.double 0.0073
    %8 = fn_ref $early_refl.list_fold.callback.2.kernel
    %9 = const.int 0
    %10 = ptr_offset %1, %9[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %9:index/borrow#1][0m
    %11 = primitive_cast %10 from Ptr of Char to Ptr of Char[2m ; ops [%10:value/borrow#0][0m
    %12 = call %8(%0, %11, %2, %3, %4, %6, %7)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %11:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %6:value/borrow#5, %7:value/borrow#6][0m
    %13 = const.double 0.0119
    %14 = const.int 38424
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Char[2m ; ops [%15:value/borrow#0][0m
    %17 = call %8(%0, %16, %2, %3, %4, %12, %13)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %16:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %12:value/borrow#5, %13:value/borrow#6][0m
    %18 = const.double 0.0169
    %19 = const.int 76848
    %20 = ptr_offset %1, %19[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %19:index/borrow#1][0m
    %21 = primitive_cast %20 from Ptr of Char to Ptr of Char[2m ; ops [%20:value/borrow#0][0m
    %22 = call %8(%0, %21, %2, %3, %4, %17, %18)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %21:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %17:value/borrow#5, %18:value/borrow#6][0m
    %23 = const.double 0.0223
    %24 = const.int 115272
    %25 = ptr_offset %1, %24[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Char[2m ; ops [%25:value/borrow#0][0m
    %27 = call %8(%0, %26, %2, %3, %4, %22, %23)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %26:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %22:value/borrow#5, %23:value/borrow#6][0m
    %28 = const.double 0.0281
    %29 = const.int 153696
    %30 = ptr_offset %1, %29[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Char[2m ; ops [%30:value/borrow#0][0m
    %32 = call %8(%0, %31, %2, %3, %4, %27, %28)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %31:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %27:value/borrow#5, %28:value/borrow#6][0m
    %33 = const.double 0.0341
    %34 = const.int 192120
    %35 = ptr_offset %1, %34[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %34:index/borrow#1][0m
    %36 = primitive_cast %35 from Ptr of Char to Ptr of Char[2m ; ops [%35:value/borrow#0][0m
    %37 = call %8(%0, %36, %2, %3, %4, %32, %33)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %0:value/consume#0, %36:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %32:value/borrow#5, %33:value/borrow#6][0m
    %38 = fmul %5, %37[2m ; ops [%5:value/borrow#0, %37:value/borrow#1][0m
    return %38[2m ; ops [%38:return/consume#0][0m
}
fn early_refl.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $early_refl.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn early_refl.list_fold.callback.2.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn early_refl.list_fold.callback.2.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = fn_ref $ylc_audio_memzero
    %4 = const.int 38424
    %5 = call %3(%2, %4)[2m : ()[0m[2m ; ops [%3:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn early_refl.list_fold.callback.2.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 x[2m: Double @borrow[0m, %5 acc[2m: Double @borrow[0m, %6 t[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 0.1
    %8 = const.double 0
    %9 = fn_ref $ylc_audio_dl_allpass_kernel
    %10 = const.int 0
    %11 = ptr_offset %1, %10[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %10:index/borrow#1][0m
    %12 = const.int 4802
    %13 = call %9(%11, %3, %12, %6, %8, %4)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %11:value/borrow#0, %3:value/borrow#1, %12:value/borrow#2, %6:value/borrow#3, %8:value/borrow#4, %4:value/borrow#5][0m
    %14 = fadd %5, %13[2m ; ops [%5:value/borrow#0, %13:value/borrow#1][0m
    return %14[2m ; ops [%14:return/consume#0][0m
}
fn reverb.cons(%0 room[2m: Double @borrow[0m, %1 damp[2m: Double @borrow[0m, %2 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $reverb.frame
    %5 = const.string "reverb"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 6413088
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $reverb.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $reverb.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn reverb.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 230544
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 0
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = fn_ref $early_refl.init
    %12 = call %11(%10)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0][0m
    %13 = const.int 230544
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = fn_ref $schroeder.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn reverb.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 room[2m: Double @borrow[0m, %5 damp[2m: Double @borrow[0m, %6 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 0.4
    %8 = fmul %6, %7[2m ; ops [%6:value/borrow#0, %7:value/borrow#1][0m
    %9 = fn_ref $early_refl.kernel
    %10 = const.int 0
    %11 = ptr_offset %1, %10[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Char[2m ; ops [%11:value/borrow#0][0m
    %13 = call %9(%0, %12, %2, %3, %8)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %0:value/consume#0, %12:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %8:value/borrow#4][0m
    %14 = const.double 0.6
    %15 = fmul %6, %14[2m ; ops [%6:value/borrow#0, %14:value/borrow#1][0m
    %16 = fn_ref $schroeder.kernel
    %17 = const.int 230544
    %18 = ptr_offset %1, %17[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %17:index/borrow#1][0m
    %19 = primitive_cast %18 from Ptr of Char to Ptr of Char[2m ; ops [%18:value/borrow#0][0m
    %20 = call %16(%0, %19, %2, %3, %4, %5, %15)[2m : Double[0m[2m ; ops [%16:callee/borrow#0, %0:value/consume#0, %19:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %5:value/borrow#5, %15:value/borrow#6][0m
    %21 = fadd %13, %20[2m ; ops [%13:value/borrow#0, %20:value/borrow#1][0m
    return %21[2m ; ops [%21:return/consume#0][0m
}
fn reverb.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $reverb.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
fn plate.cons(%0 fb[2m: Double @borrow[0m, %1 x[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = fn_ref $ylc_create_audio_frame_node
    %3 = fn_ref $plate.frame
    %4 = const.string "plate"
    %5 = const.int 2
    %6 = const.int 1
    %7 = const.int 403416
    %8 = call %2(%3, %5, %6, %7, %4)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %5:value/borrow#1, %6:value/borrow#2, %7:value/borrow#3, %4:value/borrow#4][0m
    %9 = fn_ref $ylc_audio_node_set_state_init
    %10 = fn_ref $plate.init
    %11 = call %9(%8, %10)[2m : ()[0m[2m ; ops [%9:callee/borrow#0, %8:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $ylc_audio_node_inline_state
    %13 = call %12(%8)[2m : Ptr[0m[2m ; ops [%12:callee/borrow#0, %8:value/borrow#0][0m
    %14 = fn_ref $plate.init
    %15 = call %14(%13)[2m : ()[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0][0m
    %16 = fn_ref $node_connect_input
    %17 = fn_ref $const_sig
    %18 = call %17(%0)[2m : Ptr[0m[2m ; ops [%17:callee/borrow#0, %0:value/borrow#0][0m
    %19 = const.int 0
    %20 = call %16(%19, %8, %18)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %19:value/borrow#0, %8:value/borrow#1, %18:value/borrow#2][0m
    %21 = fn_ref $const_sig
    %22 = call %21(%1)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %1:value/borrow#0][0m
    %23 = const.int 1
    %24 = call %16(%23, %8, %22)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %23:value/borrow#0, %8:value/borrow#1, %22:value/borrow#2][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn plate.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 345792
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 288168
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 230544
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 172920
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 115296
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = const.char '\x00'
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 57672
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = const.char '\x00'
    %24 = store %22, %23[2m : ()[0m[2m ; ops [%22:value/borrow#0, %23:element/consume#1][0m
    %25 = const.int 38448
    %26 = ptr_offset %0, %25[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %25:index/borrow#1][0m
    %27 = fn_ref $ylc_audio_memzero
    %28 = const.int 19224
    %29 = call %27(%26, %28)[2m : ()[0m[2m ; ops [%27:callee/borrow#0, %26:value/borrow#0, %28:value/borrow#1][0m
    %30 = const.int 19224
    %31 = ptr_offset %0, %30[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %30:index/borrow#1][0m
    %32 = fn_ref $ylc_audio_memzero
    %33 = const.int 19224
    %34 = call %32(%31, %33)[2m : ()[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %33:value/borrow#1][0m
    %35 = const.int 0
    %36 = ptr_offset %0, %35[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %35:index/borrow#1][0m
    %37 = fn_ref $ylc_audio_memzero
    %38 = const.int 19224
    %39 = call %37(%36, %38)[2m : ()[0m[2m ; ops [%37:callee/borrow#0, %36:value/borrow#0, %38:value/borrow#1][0m
    %40 = const.int 57672
    %41 = ptr_offset %0, %40[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %40:index/borrow#1][0m
    %42 = fn_ref $plate.list_fold.callback.3.init
    %43 = call %42(%41)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %41:value/borrow#0][0m
    %44 = const.int 115296
    %45 = ptr_offset %0, %44[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %44:index/borrow#1][0m
    %46 = call %42(%45)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %45:value/borrow#0][0m
    %47 = const.int 172920
    %48 = ptr_offset %0, %47[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %47:index/borrow#1][0m
    %49 = call %42(%48)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %48:value/borrow#0][0m
    %50 = const.int 230544
    %51 = ptr_offset %0, %50[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %50:index/borrow#1][0m
    %52 = call %42(%51)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %51:value/borrow#0][0m
    %53 = const.int 288168
    %54 = ptr_offset %0, %53[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %53:index/borrow#1][0m
    %55 = call %42(%54)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %54:value/borrow#0][0m
    %56 = const.int 345792
    %57 = ptr_offset %0, %56[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %56:index/borrow#1][0m
    %58 = call %42(%57)[2m : ()[0m[2m ; ops [%42:callee/borrow#0, %57:value/borrow#0][0m
    %59 = const.void
    return %59[2m ; ops [%59:return/consume#0][0m
}
fn plate.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 fb[2m: Double @borrow[0m, %5 x[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = const.double 0.0127
    %7 = const.double 0.05
    %8 = const.double 0.7
    %9 = const.double 0.0036
    %10 = const.double 0.05
    %11 = const.double 0.625
    %12 = const.double 0.0048
    %13 = const.double 0.05
    %14 = const.double 0.75
    %15 = fn_ref $ylc_audio_dl_allpass_kernel
    %16 = const.int 0
    %17 = ptr_offset %1, %16[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %16:index/borrow#1][0m
    %18 = const.int 2402
    %19 = call %15(%17, %3, %18, %12, %14, %5)[2m : Double[0m[2m ; ops [%15:callee/borrow#0, %17:value/borrow#0, %3:value/borrow#1, %18:value/borrow#2, %12:value/borrow#3, %14:value/borrow#4, %5:value/borrow#5][0m
    %20 = fn_ref $ylc_audio_dl_allpass_kernel
    %21 = const.int 19224
    %22 = ptr_offset %1, %21[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %21:index/borrow#1][0m
    %23 = const.int 2402
    %24 = call %20(%22, %3, %23, %9, %11, %19)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %22:value/borrow#0, %3:value/borrow#1, %23:value/borrow#2, %9:value/borrow#3, %11:value/borrow#4, %19:value/borrow#5][0m
    %25 = fn_ref $ylc_audio_dl_allpass_kernel
    %26 = const.int 38448
    %27 = ptr_offset %1, %26[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %26:index/borrow#1][0m
    %28 = const.int 2402
    %29 = call %25(%27, %3, %28, %6, %8, %24)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %27:value/borrow#0, %3:value/borrow#1, %28:value/borrow#2, %6:value/borrow#3, %8:value/borrow#4, %24:value/borrow#5][0m
    %30 = const.double 0.0672
    %31 = fn_ref $plate.list_fold.callback.3.kernel
    %32 = const.int 57672
    %33 = ptr_offset %1, %32[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %32:index/borrow#1][0m
    %34 = primitive_cast %33 from Ptr of Char to Ptr of Char[2m ; ops [%33:value/borrow#0][0m
    %35 = call %31(%0, %34, %2, %3, %4, %29, %30)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %34:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %29:value/borrow#5, %30:value/borrow#6][0m
    %36 = const.double 0.01
    %37 = const.int 115296
    %38 = ptr_offset %1, %37[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %37:index/borrow#1][0m
    %39 = primitive_cast %38 from Ptr of Char to Ptr of Char[2m ; ops [%38:value/borrow#0][0m
    %40 = call %31(%0, %39, %2, %3, %4, %35, %36)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %39:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %35:value/borrow#5, %36:value/borrow#6][0m
    %41 = const.double 0.0442
    %42 = const.int 172920
    %43 = ptr_offset %1, %42[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %42:index/borrow#1][0m
    %44 = primitive_cast %43 from Ptr of Char to Ptr of Char[2m ; ops [%43:value/borrow#0][0m
    %45 = call %31(%0, %44, %2, %3, %4, %40, %41)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %44:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %40:value/borrow#5, %41:value/borrow#6][0m
    %46 = const.double 0.0908
    %47 = const.int 230544
    %48 = ptr_offset %1, %47[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %47:index/borrow#1][0m
    %49 = primitive_cast %48 from Ptr of Char to Ptr of Char[2m ; ops [%48:value/borrow#0][0m
    %50 = call %31(%0, %49, %2, %3, %4, %45, %46)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %49:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %45:value/borrow#5, %46:value/borrow#6][0m
    %51 = const.double 0.01
    %52 = const.int 288168
    %53 = ptr_offset %1, %52[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %52:index/borrow#1][0m
    %54 = primitive_cast %53 from Ptr of Char to Ptr of Char[2m ; ops [%53:value/borrow#0][0m
    %55 = call %31(%0, %54, %2, %3, %4, %50, %51)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %54:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %50:value/borrow#5, %51:value/borrow#6][0m
    %56 = const.double 0.04
    %57 = const.int 345792
    %58 = ptr_offset %1, %57[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %57:index/borrow#1][0m
    %59 = primitive_cast %58 from Ptr of Char to Ptr of Char[2m ; ops [%58:value/borrow#0][0m
    %60 = call %31(%0, %59, %2, %3, %4, %55, %56)[2m : Double[0m[2m ; ops [%31:callee/borrow#0, %0:value/consume#0, %59:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %4:value/borrow#4, %55:value/borrow#5, %56:value/borrow#6][0m
    return %60[2m ; ops [%60:return/consume#0][0m
}
fn plate.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = fn_ref $plate.kernel
    %18 = call %17(%0, %1, %3, %4, %10, %16)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4, %16:value/borrow#5][0m
    %19 = fn_ref $ylc_get_output_buf
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = ptr_offset %20, %3[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %3:index/borrow#1][0m
    %22 = store %21, %18[2m : ()[0m[2m ; ops [%21:value/borrow#0, %18:element/consume#1][0m
    %23 = const.void
    return %23[2m ; ops [%23:return/consume#0][0m
}
fn plate.list_fold.callback.3.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn plate.list_fold.callback.3.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = fn_ref $ylc_audio_memzero
    %4 = const.int 57624
    %5 = call %3(%2, %4)[2m : ()[0m[2m ; ops [%3:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn plate.list_fold.callback.3.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 fb[2m: Double @borrow[0m, %5 acc[2m: Double @borrow[0m, %6 t[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.double 0.15
    %8 = fn_ref $ylc_audio_dl_allpass_kernel
    %9 = const.int 0
    %10 = ptr_offset %1, %9[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.int 7202
    %12 = call %8(%10, %3, %11, %6, %4, %5)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %10:value/borrow#0, %3:value/borrow#1, %11:value/borrow#2, %6:value/borrow#3, %4:value/borrow#4, %5:value/borrow#5][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn absorb_comb.cons(%0 t[2m: Double @borrow[0m, %1 damp[2m: Double @borrow[0m, %2 input[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $absorb_comb.frame
    %5 = const.string "absorb_comb"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 38448
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $absorb_comb.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $absorb_comb.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn absorb_comb.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 24
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = fn_ref $ylc_audio_memzero
    %4 = const.int 38424
    %5 = call %3(%2, %4)[2m : ()[0m[2m ; ops [%3:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.int 8
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = const.char '\x00'
    %9 = store %7, %8[2m : ()[0m[2m ; ops [%7:value/borrow#0, %8:element/consume#1][0m
    %10 = const.int 0
    %11 = ptr_offset %0, %10[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Double[2m ; ops [%11:value/borrow#0][0m
    %13 = const.double 0
    %14 = store %12, %13[2m : ()[0m[2m ; ops [%12:value/borrow#0, %13:element/consume#1][0m
    %15 = const.int 0
    %16 = ptr_offset %0, %15[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Double[2m ; ops [%16:value/borrow#0][0m
    %18 = const.double 0
    %19 = store %17, %18[2m : ()[0m[2m ; ops [%17:value/borrow#0, %18:element/consume#1][0m
    %20 = const.int 8
    %21 = ptr_offset %0, %20[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %20:index/borrow#1][0m
    %22 = fn_ref $lpf1p.init
    %23 = call %22(%21)[2m : ()[0m[2m ; ops [%22:callee/borrow#0, %21:value/borrow#0][0m
    %24 = const.void
    return %24[2m ; ops [%24:return/consume#0][0m
}
fn absorb_comb.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 t[2m: Double @borrow[0m, %5 damp[2m: Double @borrow[0m, %6 input[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.int 0
    %8 = ptr_offset %1, %7[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %7:index/borrow#1][0m
    %9 = primitive_cast %8 from Ptr of Char to Ptr of Double[2m ; ops [%8:value/borrow#0][0m
    %10 = const.int 1
    %11 = const.int 0
    %12 = construct.tuple { %10, %11, %9 }[2m : Array of Double[0m[2m ; ops [%10:field/borrow#0, %11:field/borrow#1, %9:field/borrow#2][0m
    %13 = const.double 8000
    %14 = const.double 1
    %15 = fsub %14, %5[2m ; ops [%14:value/borrow#0, %5:value/borrow#1][0m
    %16 = fmul %13, %15[2m ; ops [%13:value/borrow#0, %15:value/borrow#1][0m
    %17 = const.int 0
    %18 = extract.field %12, 1[2m : Int[0m[2m ; ops [%12:container/borrow#0][0m
    %19 = iadd %18, %17[2m ; ops [%18:value/borrow#0, %17:value/borrow#1][0m
    %20 = extract.field %12, 2[2m : Ptr[0m[2m ; ops [%12:container/borrow#0][0m
    %21 = ptr_offset %20, %19[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %19:index/borrow#1][0m
    %22 = load %21[2m : Double[0m[2m ; ops [%21:value/borrow#0][0m
    %23 = fn_ref $lpf1p.kernel
    %24 = const.int 8
    %25 = ptr_offset %1, %24[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Char[2m ; ops [%25:value/borrow#0][0m
    %27 = call %23(%0, %26, %2, %3, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %26:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %16:value/borrow#4, %22:value/borrow#5][0m
    %28 = fadd %6, %27[2m ; ops [%6:value/borrow#0, %27:value/borrow#1][0m
    %29 = const.int 0
    %30 = const.double 0.1
    %31 = const.double 0.8
    %32 = fn_ref $ylc_audio_comb_kernel
    %33 = const.int 24
    %34 = ptr_offset %1, %33[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %33:index/borrow#1][0m
    %35 = const.int 4802
    %36 = call %32(%34, %3, %35, %4, %31, %28)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %34:value/borrow#0, %3:value/borrow#1, %35:value/borrow#2, %4:value/borrow#3, %31:value/borrow#4, %28:value/borrow#5][0m
    %37 = extract.field %12, 1[2m : Int[0m[2m ; ops [%12:container/borrow#0][0m
    %38 = iadd %37, %29[2m ; ops [%37:value/borrow#0, %29:value/borrow#1][0m
    %39 = extract.field %12, 2[2m : Ptr[0m[2m ; ops [%12:container/borrow#0][0m
    %40 = ptr_offset %39, %38[2m : Ptr[0m[2m ; ops [%39:value/borrow#0, %38:index/borrow#1][0m
    %41 = store %40, %36[2m : ()[0m[2m ; ops [%40:value/borrow#0, %36:element/consume#1][0m
    return %28[2m ; ops [%28:return/consume#0][0m
}
fn absorb_comb.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $absorb_comb.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
extern fn matrix_vec_mul_double(%0 arg0[2m: Int @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: Array of Double @borrow[0m, %3 arg3[2m: Array of Double @borrow[0m, %4 arg4[2m: Array of Double @borrow[0m)[2m -> Array of Double @owned[0m;
extern fn vec_dot_double(%0 arg0[2m: Array of Double @borrow[0m, %1 arg1[2m: Array of Double @borrow[0m)[2m -> Double @owned[0m;
fn fdn4.cons(%0 d1[2m: Double @borrow[0m, %1 d2[2m: Double @borrow[0m, %2 d3[2m: Double @borrow[0m, %3 d4[2m: Double @borrow[0m, %4 rt60[2m: Double @borrow[0m, %5 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = fn_ref $ylc_create_audio_frame_node
    %7 = fn_ref $fdn4.frame
    %8 = const.string "fdn4"
    %9 = const.int 6
    %10 = const.int 1
    %11 = const.int 1536392
    %12 = call %6(%7, %9, %10, %11, %8)[2m : Ptr[0m[2m ; ops [%6:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1, %10:value/borrow#2, %11:value/borrow#3, %8:value/borrow#4][0m
    %13 = fn_ref $ylc_audio_node_set_state_init
    %14 = fn_ref $fdn4.init
    %15 = call %13(%12, %14)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0, %14:value/borrow#1][0m
    %16 = fn_ref $ylc_audio_node_inline_state
    %17 = call %16(%12)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %12:value/borrow#0][0m
    %18 = fn_ref $fdn4.init
    %19 = call %18(%17)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %17:value/borrow#0][0m
    %20 = fn_ref $node_connect_input
    %21 = fn_ref $const_sig
    %22 = call %21(%0)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %0:value/borrow#0][0m
    %23 = const.int 0
    %24 = call %20(%23, %12, %22)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %12:value/borrow#1, %22:value/borrow#2][0m
    %25 = fn_ref $const_sig
    %26 = call %25(%1)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %1:value/borrow#0][0m
    %27 = const.int 1
    %28 = call %20(%27, %12, %26)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %27:value/borrow#0, %12:value/borrow#1, %26:value/borrow#2][0m
    %29 = fn_ref $const_sig
    %30 = call %29(%2)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %2:value/borrow#0][0m
    %31 = const.int 2
    %32 = call %20(%31, %12, %30)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %31:value/borrow#0, %12:value/borrow#1, %30:value/borrow#2][0m
    %33 = fn_ref $const_sig
    %34 = call %33(%3)[2m : Ptr[0m[2m ; ops [%33:callee/borrow#0, %3:value/borrow#0][0m
    %35 = const.int 3
    %36 = call %20(%35, %12, %34)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %35:value/borrow#0, %12:value/borrow#1, %34:value/borrow#2][0m
    %37 = fn_ref $const_sig
    %38 = call %37(%4)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %4:value/borrow#0][0m
    %39 = const.int 4
    %40 = call %20(%39, %12, %38)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %39:value/borrow#0, %12:value/borrow#1, %38:value/borrow#2][0m
    %41 = fn_ref $const_sig
    %42 = call %41(%5)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %5:value/borrow#0][0m
    %43 = const.int 5
    %44 = call %20(%43, %12, %42)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %43:value/borrow#0, %12:value/borrow#1, %42:value/borrow#2][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn fdn4.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 1536384
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 1536376
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 1536368
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 1536360
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 1536328
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = primitive_cast %18 from Ptr of Char to Ptr of Double[2m ; ops [%18:value/borrow#0][0m
    %20 = const.double 0
    %21 = store %19, %20[2m : ()[0m[2m ; ops [%19:value/borrow#0, %20:element/consume#1][0m
    %22 = const.int 1536320
    %23 = ptr_offset %0, %22[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %22:index/borrow#1][0m
    %24 = const.char '\x00'
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.int 1536312
    %27 = ptr_offset %0, %26[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %26:index/borrow#1][0m
    %28 = const.char '\x00'
    %29 = store %27, %28[2m : ()[0m[2m ; ops [%27:value/borrow#0, %28:element/consume#1][0m
    %30 = const.int 1536304
    %31 = ptr_offset %0, %30[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %30:index/borrow#1][0m
    %32 = const.char '\x00'
    %33 = store %31, %32[2m : ()[0m[2m ; ops [%31:value/borrow#0, %32:element/consume#1][0m
    %34 = const.int 1536296
    %35 = ptr_offset %0, %34[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %34:index/borrow#1][0m
    %36 = const.char '\x00'
    %37 = store %35, %36[2m : ()[0m[2m ; ops [%35:value/borrow#0, %36:element/consume#1][0m
    %38 = const.int 1536288
    %39 = ptr_offset %0, %38[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %38:index/borrow#1][0m
    %40 = primitive_cast %39 from Ptr of Char to Ptr of Int[2m ; ops [%39:value/borrow#0][0m
    %41 = const.int 0
    %42 = store %40, %41[2m : ()[0m[2m ; ops [%40:value/borrow#0, %41:element/consume#1][0m
    %43 = const.int 1152288
    %44 = ptr_offset %0, %43[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %43:index/borrow#1][0m
    %45 = fn_ref $ylc_audio_memzero
    %46 = const.int 384000
    %47 = call %45(%44, %46)[2m : ()[0m[2m ; ops [%45:callee/borrow#0, %44:value/borrow#0, %46:value/borrow#1][0m
    %48 = const.int 768288
    %49 = ptr_offset %0, %48[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %48:index/borrow#1][0m
    %50 = fn_ref $ylc_audio_memzero
    %51 = const.int 384000
    %52 = call %50(%49, %51)[2m : ()[0m[2m ; ops [%50:callee/borrow#0, %49:value/borrow#0, %51:value/borrow#1][0m
    %53 = const.int 384288
    %54 = ptr_offset %0, %53[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %53:index/borrow#1][0m
    %55 = fn_ref $ylc_audio_memzero
    %56 = const.int 384000
    %57 = call %55(%54, %56)[2m : ()[0m[2m ; ops [%55:callee/borrow#0, %54:value/borrow#0, %56:value/borrow#1][0m
    %58 = const.int 288
    %59 = ptr_offset %0, %58[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %58:index/borrow#1][0m
    %60 = fn_ref $ylc_audio_memzero
    %61 = const.int 384000
    %62 = call %60(%59, %61)[2m : ()[0m[2m ; ops [%60:callee/borrow#0, %59:value/borrow#0, %61:value/borrow#1][0m
    %63 = const.int 192
    %64 = ptr_offset %0, %63[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %63:index/borrow#1][0m
    %65 = primitive_cast %64 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%64:value/borrow#0][0m
    %66 = const.undef
    %255 = load_owned %65[2m : (Array of Double, Double, )[0m[2m ; ops [%65:value/borrow#0][0m
    %67 = store %65, %66[2m : ()[0m[2m ; ops [%65:value/borrow#0, %66:element/consume#1][0m
[35m    %256 = drop %255[2m : ()[0m[2m ; ops [%255:value/borrow#0][0m[0m
    %68 = const.int 160
    %69 = ptr_offset %0, %68[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %68:index/borrow#1][0m
    %70 = primitive_cast %69 from Ptr of Char to Ptr of Double[2m ; ops [%69:value/borrow#0][0m
    %71 = const.double 0
    %72 = store %70, %71[2m : ()[0m[2m ; ops [%70:value/borrow#0, %71:element/consume#1][0m
    %73 = const.int 128
    %74 = ptr_offset %0, %73[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %73:index/borrow#1][0m
    %75 = primitive_cast %74 from Ptr of Char to Ptr of Double[2m ; ops [%74:value/borrow#0][0m
    %76 = const.double 0
    %77 = store %75, %76[2m : ()[0m[2m ; ops [%75:value/borrow#0, %76:element/consume#1][0m
    %78 = const.int 0
    %79 = ptr_offset %0, %78[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %78:index/borrow#1][0m
    %80 = primitive_cast %79 from Ptr of Char to Ptr of Double[2m ; ops [%79:value/borrow#0][0m
    %81 = const.double 0
    %82 = store %80, %81[2m : ()[0m[2m ; ops [%80:value/borrow#0, %81:element/consume#1][0m
    %83 = const.int 1536352
    %84 = ptr_offset %0, %83[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %83:index/borrow#1][0m
    %85 = primitive_cast %84 from Ptr of Char to Ptr of Double[2m ; ops [%84:value/borrow#0][0m
    %86 = const.double 0
    %87 = store %85, %86[2m : ()[0m[2m ; ops [%85:value/borrow#0, %86:element/consume#1][0m
    %88 = const.int 1536344
    %89 = ptr_offset %0, %88[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %88:index/borrow#1][0m
    %90 = primitive_cast %89 from Ptr of Char to Ptr of Double[2m ; ops [%89:value/borrow#0][0m
    %91 = const.double 0
    %92 = store %90, %91[2m : ()[0m[2m ; ops [%90:value/borrow#0, %91:element/consume#1][0m
    %93 = const.int 1536336
    %94 = ptr_offset %0, %93[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %93:index/borrow#1][0m
    %95 = primitive_cast %94 from Ptr of Char to Ptr of Double[2m ; ops [%94:value/borrow#0][0m
    %96 = const.double 0
    %97 = store %95, %96[2m : ()[0m[2m ; ops [%95:value/borrow#0, %96:element/consume#1][0m
    %98 = const.int 1536328
    %99 = ptr_offset %0, %98[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %98:index/borrow#1][0m
    %100 = primitive_cast %99 from Ptr of Char to Ptr of Double[2m ; ops [%99:value/borrow#0][0m
    %101 = const.double 0
    %102 = store %100, %101[2m : ()[0m[2m ; ops [%100:value/borrow#0, %101:element/consume#1][0m
    %103 = const.int 1536288
    %104 = ptr_offset %0, %103[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %103:index/borrow#1][0m
    %105 = primitive_cast %104 from Ptr of Char to Ptr of Int[2m ; ops [%104:value/borrow#0][0m
    %106 = const.int 0
    %107 = store %105, %106[2m : ()[0m[2m ; ops [%105:value/borrow#0, %106:element/consume#1][0m
    %108 = const.int 184
    %109 = ptr_offset %0, %108[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %108:index/borrow#1][0m
    %110 = primitive_cast %109 from Ptr of Char to Ptr of Double[2m ; ops [%109:value/borrow#0][0m
    %111 = const.double 0.25
    %112 = store %110, %111[2m : ()[0m[2m ; ops [%110:value/borrow#0, %111:element/consume#1][0m
    %113 = const.int 176
    %114 = ptr_offset %0, %113[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %113:index/borrow#1][0m
    %115 = primitive_cast %114 from Ptr of Char to Ptr of Double[2m ; ops [%114:value/borrow#0][0m
    %116 = const.double 0.25
    %117 = store %115, %116[2m : ()[0m[2m ; ops [%115:value/borrow#0, %116:element/consume#1][0m
    %118 = const.int 168
    %119 = ptr_offset %0, %118[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %118:index/borrow#1][0m
    %120 = primitive_cast %119 from Ptr of Char to Ptr of Double[2m ; ops [%119:value/borrow#0][0m
    %121 = const.double 0.25
    %122 = store %120, %121[2m : ()[0m[2m ; ops [%120:value/borrow#0, %121:element/consume#1][0m
    %123 = const.int 160
    %124 = ptr_offset %0, %123[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %123:index/borrow#1][0m
    %125 = primitive_cast %124 from Ptr of Char to Ptr of Double[2m ; ops [%124:value/borrow#0][0m
    %126 = const.double 0.25
    %127 = store %125, %126[2m : ()[0m[2m ; ops [%125:value/borrow#0, %126:element/consume#1][0m
    %128 = const.int 152
    %129 = ptr_offset %0, %128[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %128:index/borrow#1][0m
    %130 = primitive_cast %129 from Ptr of Char to Ptr of Double[2m ; ops [%129:value/borrow#0][0m
    %131 = const.double 1
    %132 = store %130, %131[2m : ()[0m[2m ; ops [%130:value/borrow#0, %131:element/consume#1][0m
    %133 = const.int 144
    %134 = ptr_offset %0, %133[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %133:index/borrow#1][0m
    %135 = primitive_cast %134 from Ptr of Char to Ptr of Double[2m ; ops [%134:value/borrow#0][0m
    %136 = const.double 1
    %137 = store %135, %136[2m : ()[0m[2m ; ops [%135:value/borrow#0, %136:element/consume#1][0m
    %138 = const.int 136
    %139 = ptr_offset %0, %138[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %138:index/borrow#1][0m
    %140 = primitive_cast %139 from Ptr of Char to Ptr of Double[2m ; ops [%139:value/borrow#0][0m
    %141 = const.double 1
    %142 = store %140, %141[2m : ()[0m[2m ; ops [%140:value/borrow#0, %141:element/consume#1][0m
    %143 = const.int 128
    %144 = ptr_offset %0, %143[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %143:index/borrow#1][0m
    %145 = primitive_cast %144 from Ptr of Char to Ptr of Double[2m ; ops [%144:value/borrow#0][0m
    %146 = const.double 1
    %147 = store %145, %146[2m : ()[0m[2m ; ops [%145:value/borrow#0, %146:element/consume#1][0m
    %148 = const.int 120
    %149 = ptr_offset %0, %148[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %148:index/borrow#1][0m
    %150 = primitive_cast %149 from Ptr of Char to Ptr of Double[2m ; ops [%149:value/borrow#0][0m
    %151 = const.double -0.5
    %152 = store %150, %151[2m : ()[0m[2m ; ops [%150:value/borrow#0, %151:element/consume#1][0m
    %153 = const.int 112
    %154 = ptr_offset %0, %153[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %153:index/borrow#1][0m
    %155 = primitive_cast %154 from Ptr of Char to Ptr of Double[2m ; ops [%154:value/borrow#0][0m
    %156 = const.double 0.5
    %157 = store %155, %156[2m : ()[0m[2m ; ops [%155:value/borrow#0, %156:element/consume#1][0m
    %158 = const.int 104
    %159 = ptr_offset %0, %158[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %158:index/borrow#1][0m
    %160 = primitive_cast %159 from Ptr of Char to Ptr of Double[2m ; ops [%159:value/borrow#0][0m
    %161 = const.double 0.5
    %162 = store %160, %161[2m : ()[0m[2m ; ops [%160:value/borrow#0, %161:element/consume#1][0m
    %163 = const.int 96
    %164 = ptr_offset %0, %163[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %163:index/borrow#1][0m
    %165 = primitive_cast %164 from Ptr of Char to Ptr of Double[2m ; ops [%164:value/borrow#0][0m
    %166 = const.double 0.5
    %167 = store %165, %166[2m : ()[0m[2m ; ops [%165:value/borrow#0, %166:element/consume#1][0m
    %168 = const.int 88
    %169 = ptr_offset %0, %168[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %168:index/borrow#1][0m
    %170 = primitive_cast %169 from Ptr of Char to Ptr of Double[2m ; ops [%169:value/borrow#0][0m
    %171 = const.double 0.5
    %172 = store %170, %171[2m : ()[0m[2m ; ops [%170:value/borrow#0, %171:element/consume#1][0m
    %173 = const.int 80
    %174 = ptr_offset %0, %173[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %173:index/borrow#1][0m
    %175 = primitive_cast %174 from Ptr of Char to Ptr of Double[2m ; ops [%174:value/borrow#0][0m
    %176 = const.double -0.5
    %177 = store %175, %176[2m : ()[0m[2m ; ops [%175:value/borrow#0, %176:element/consume#1][0m
    %178 = const.int 72
    %179 = ptr_offset %0, %178[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %178:index/borrow#1][0m
    %180 = primitive_cast %179 from Ptr of Char to Ptr of Double[2m ; ops [%179:value/borrow#0][0m
    %181 = const.double 0.5
    %182 = store %180, %181[2m : ()[0m[2m ; ops [%180:value/borrow#0, %181:element/consume#1][0m
    %183 = const.int 64
    %184 = ptr_offset %0, %183[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %183:index/borrow#1][0m
    %185 = primitive_cast %184 from Ptr of Char to Ptr of Double[2m ; ops [%184:value/borrow#0][0m
    %186 = const.double 0.5
    %187 = store %185, %186[2m : ()[0m[2m ; ops [%185:value/borrow#0, %186:element/consume#1][0m
    %188 = const.int 56
    %189 = ptr_offset %0, %188[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %188:index/borrow#1][0m
    %190 = primitive_cast %189 from Ptr of Char to Ptr of Double[2m ; ops [%189:value/borrow#0][0m
    %191 = const.double 0.5
    %192 = store %190, %191[2m : ()[0m[2m ; ops [%190:value/borrow#0, %191:element/consume#1][0m
    %193 = const.int 48
    %194 = ptr_offset %0, %193[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %193:index/borrow#1][0m
    %195 = primitive_cast %194 from Ptr of Char to Ptr of Double[2m ; ops [%194:value/borrow#0][0m
    %196 = const.double 0.5
    %197 = store %195, %196[2m : ()[0m[2m ; ops [%195:value/borrow#0, %196:element/consume#1][0m
    %198 = const.int 40
    %199 = ptr_offset %0, %198[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %198:index/borrow#1][0m
    %200 = primitive_cast %199 from Ptr of Char to Ptr of Double[2m ; ops [%199:value/borrow#0][0m
    %201 = const.double -0.5
    %202 = store %200, %201[2m : ()[0m[2m ; ops [%200:value/borrow#0, %201:element/consume#1][0m
    %203 = const.int 32
    %204 = ptr_offset %0, %203[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %203:index/borrow#1][0m
    %205 = primitive_cast %204 from Ptr of Char to Ptr of Double[2m ; ops [%204:value/borrow#0][0m
    %206 = const.double 0.5
    %207 = store %205, %206[2m : ()[0m[2m ; ops [%205:value/borrow#0, %206:element/consume#1][0m
    %208 = const.int 24
    %209 = ptr_offset %0, %208[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %208:index/borrow#1][0m
    %210 = primitive_cast %209 from Ptr of Char to Ptr of Double[2m ; ops [%209:value/borrow#0][0m
    %211 = const.double 0.5
    %212 = store %210, %211[2m : ()[0m[2m ; ops [%210:value/borrow#0, %211:element/consume#1][0m
    %213 = const.int 16
    %214 = ptr_offset %0, %213[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %213:index/borrow#1][0m
    %215 = primitive_cast %214 from Ptr of Char to Ptr of Double[2m ; ops [%214:value/borrow#0][0m
    %216 = const.double 0.5
    %217 = store %215, %216[2m : ()[0m[2m ; ops [%215:value/borrow#0, %216:element/consume#1][0m
    %218 = const.int 8
    %219 = ptr_offset %0, %218[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %218:index/borrow#1][0m
    %220 = primitive_cast %219 from Ptr of Char to Ptr of Double[2m ; ops [%219:value/borrow#0][0m
    %221 = const.double 0.5
    %222 = store %220, %221[2m : ()[0m[2m ; ops [%220:value/borrow#0, %221:element/consume#1][0m
    %223 = const.int 0
    %224 = ptr_offset %0, %223[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %223:index/borrow#1][0m
    %225 = primitive_cast %224 from Ptr of Char to Ptr of Double[2m ; ops [%224:value/borrow#0][0m
    %226 = const.double -0.5
    %227 = store %225, %226[2m : ()[0m[2m ; ops [%225:value/borrow#0, %226:element/consume#1][0m
    %228 = const.int 1536296
    %229 = ptr_offset %0, %228[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %228:index/borrow#1][0m
    %230 = fn_ref $fdn4.array_map.callback.4.init
    %231 = call %230(%229)[2m : ()[0m[2m ; ops [%230:callee/borrow#0, %229:value/consume#0][0m
    %232 = const.int 1536304
    %233 = ptr_offset %0, %232[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %232:index/borrow#1][0m
    %234 = call %230(%233)[2m : ()[0m[2m ; ops [%230:callee/borrow#0, %233:value/consume#0][0m
    %235 = const.int 1536312
    %236 = ptr_offset %0, %235[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %235:index/borrow#1][0m
    %237 = call %230(%236)[2m : ()[0m[2m ; ops [%230:callee/borrow#0, %236:value/consume#0][0m
    %238 = const.int 1536320
    %239 = ptr_offset %0, %238[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %238:index/borrow#1][0m
    %240 = call %230(%239)[2m : ()[0m[2m ; ops [%230:callee/borrow#0, %239:value/consume#0][0m
    %241 = const.int 1536360
    %242 = ptr_offset %0, %241[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %241:index/borrow#1][0m
    %243 = fn_ref $fdn4.array_mapi.callback.5.init
    %244 = call %243(%242)[2m : ()[0m[2m ; ops [%243:callee/borrow#0, %242:value/consume#0][0m
    %245 = const.int 1536368
    %246 = ptr_offset %0, %245[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %245:index/borrow#1][0m
    %247 = call %243(%246)[2m : ()[0m[2m ; ops [%243:callee/borrow#0, %246:value/consume#0][0m
    %248 = const.int 1536376
    %249 = ptr_offset %0, %248[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %248:index/borrow#1][0m
    %250 = call %243(%249)[2m : ()[0m[2m ; ops [%243:callee/borrow#0, %249:value/consume#0][0m
    %251 = const.int 1536384
    %252 = ptr_offset %0, %251[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %251:index/borrow#1][0m
    %253 = call %243(%252)[2m : ()[0m[2m ; ops [%243:callee/borrow#0, %252:value/consume#0][0m
    %254 = const.void
    return %254[2m ; ops [%254:return/consume#0][0m
}
fn fdn4.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 d1[2m: Double @consume[0m, %5 d2[2m: Double @consume[0m, %6 d3[2m: Double @consume[0m, %7 d4[2m: Double @consume[0m, %8 rt60[2m: Double @borrow[0m, %9 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = const.int 0
    %11 = ptr_offset %1, %10[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %10:index/borrow#1][0m
    %12 = primitive_cast %11 from Ptr of Char to Ptr of Double[2m ; ops [%11:value/borrow#0][0m
    %13 = const.int 16
    %14 = const.int 0
    %15 = construct.tuple { %13, %14, %12 }[2m : Array of Double[0m[2m ; ops [%13:field/borrow#0, %14:field/borrow#1, %12:field/borrow#2][0m
    %16 = const.int 128
    %17 = ptr_offset %1, %16[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.int 4
    %20 = const.int 0
    %21 = construct.tuple { %19, %20, %18 }[2m : Array of Double[0m[2m ; ops [%19:field/borrow#0, %20:field/borrow#1, %18:field/borrow#2][0m
    %22 = const.int 160
    %23 = ptr_offset %1, %22[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %22:index/borrow#1][0m
    %24 = primitive_cast %23 from Ptr of Char to Ptr of Double[2m ; ops [%23:value/borrow#0][0m
    %25 = const.int 4
    %26 = const.int 0
    %27 = construct.tuple { %25, %26, %24 }[2m : Array of Double[0m[2m ; ops [%25:field/borrow#0, %26:field/borrow#1, %24:field/borrow#2][0m
    %28 = const.double 1
    %29 = const.int 288
    %30 = ptr_offset %1, %29[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Double[2m ; ops [%30:value/borrow#0][0m
    %32 = const.int 48000
    %33 = const.int 0
    %34 = construct.tuple { %32, %33, %31 }[2m : Array of Double[0m[2m ; ops [%32:field/borrow#0, %33:field/borrow#1, %31:field/borrow#2][0m
[35m    %215 = dup %34[2m : ()[0m[2m ; ops [%34:value/borrow#0][0m[0m
    %35 = construct.tuple { %34, %4 }[2m : (Array of Double, Double, )[0m[2m ; ops [%34:field/consume#0, %4:field/consume#1][0m
    %36 = const.int 192
    %37 = ptr_offset %1, %36[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %36:index/borrow#1][0m
    %38 = primitive_cast %37 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%37:value/borrow#0][0m
    %216 = load_owned %38[2m : (Array of Double, Double, )[0m[2m ; ops [%38:value/borrow#0][0m
    %39 = store %38, %35[2m : ()[0m[2m ; ops [%38:value/borrow#0, %35:element/consume#1][0m
[35m    %217 = drop %216[2m : ()[0m[2m ; ops [%216:value/borrow#0][0m[0m
    %40 = const.double 1
    %41 = const.int 384288
    %42 = ptr_offset %1, %41[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %41:index/borrow#1][0m
    %43 = primitive_cast %42 from Ptr of Char to Ptr of Double[2m ; ops [%42:value/borrow#0][0m
    %44 = const.int 48000
    %45 = const.int 0
    %46 = construct.tuple { %44, %45, %43 }[2m : Array of Double[0m[2m ; ops [%44:field/borrow#0, %45:field/borrow#1, %43:field/borrow#2][0m
[35m    %218 = dup %46[2m : ()[0m[2m ; ops [%46:value/borrow#0][0m[0m
    %47 = construct.tuple { %46, %5 }[2m : (Array of Double, Double, )[0m[2m ; ops [%46:field/consume#0, %5:field/consume#1][0m
    %48 = const.int 216
    %49 = ptr_offset %1, %48[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %48:index/borrow#1][0m
    %50 = primitive_cast %49 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%49:value/borrow#0][0m
    %219 = load_owned %50[2m : (Array of Double, Double, )[0m[2m ; ops [%50:value/borrow#0][0m
    %51 = store %50, %47[2m : ()[0m[2m ; ops [%50:value/borrow#0, %47:element/consume#1][0m
[35m    %220 = drop %219[2m : ()[0m[2m ; ops [%219:value/borrow#0][0m[0m
    %52 = const.double 1
    %53 = const.int 768288
    %54 = ptr_offset %1, %53[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %53:index/borrow#1][0m
    %55 = primitive_cast %54 from Ptr of Char to Ptr of Double[2m ; ops [%54:value/borrow#0][0m
    %56 = const.int 48000
    %57 = const.int 0
    %58 = construct.tuple { %56, %57, %55 }[2m : Array of Double[0m[2m ; ops [%56:field/borrow#0, %57:field/borrow#1, %55:field/borrow#2][0m
[35m    %221 = dup %58[2m : ()[0m[2m ; ops [%58:value/borrow#0][0m[0m
    %59 = construct.tuple { %58, %6 }[2m : (Array of Double, Double, )[0m[2m ; ops [%58:field/consume#0, %6:field/consume#1][0m
    %60 = const.int 240
    %61 = ptr_offset %1, %60[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %60:index/borrow#1][0m
    %62 = primitive_cast %61 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%61:value/borrow#0][0m
    %222 = load_owned %62[2m : (Array of Double, Double, )[0m[2m ; ops [%62:value/borrow#0][0m
    %63 = store %62, %59[2m : ()[0m[2m ; ops [%62:value/borrow#0, %59:element/consume#1][0m
[35m    %223 = drop %222[2m : ()[0m[2m ; ops [%222:value/borrow#0][0m[0m
    %64 = const.double 1
    %65 = const.int 1152288
    %66 = ptr_offset %1, %65[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %65:index/borrow#1][0m
    %67 = primitive_cast %66 from Ptr of Char to Ptr of Double[2m ; ops [%66:value/borrow#0][0m
    %68 = const.int 48000
    %69 = const.int 0
    %70 = construct.tuple { %68, %69, %67 }[2m : Array of Double[0m[2m ; ops [%68:field/borrow#0, %69:field/borrow#1, %67:field/borrow#2][0m
[35m    %224 = dup %70[2m : ()[0m[2m ; ops [%70:value/borrow#0][0m[0m
    %71 = construct.tuple { %70, %7 }[2m : (Array of Double, Double, )[0m[2m ; ops [%70:field/consume#0, %7:field/consume#1][0m
    %72 = const.int 264
    %73 = ptr_offset %1, %72[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %72:index/borrow#1][0m
    %74 = primitive_cast %73 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%73:value/borrow#0][0m
    %225 = load_owned %74[2m : (Array of Double, Double, )[0m[2m ; ops [%74:value/borrow#0][0m
    %75 = store %74, %71[2m : ()[0m[2m ; ops [%74:value/borrow#0, %71:element/consume#1][0m
[35m    %226 = drop %225[2m : ()[0m[2m ; ops [%225:value/borrow#0][0m[0m
    %76 = const.int 192
    %77 = ptr_offset %1, %76[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %76:index/borrow#1][0m
    %78 = primitive_cast %77 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%77:value/borrow#0][0m
    %79 = const.int 4
    %80 = const.int 0
    %81 = construct.tuple { %79, %80, %78 }[2m : Array of (Array of Double, Double, )[0m[2m ; ops [%79:field/borrow#0, %80:field/borrow#1, %78:field/borrow#2][0m
    %82 = const.int 0
    %83 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %84 = ptr_offset %83, %82[2m : Ptr[0m[2m ; ops [%83:value/borrow#0, %82:index/borrow#1][0m
    %85 = load %84[2m : (Array of Double, Double, )[0m[2m ; ops [%84:value/borrow#0][0m
    %86 = extract.field %85, 0[2m : Array of Double[0m[2m ; ops [%85:container/borrow#0][0m
    %87 = extract.field %85, 1[2m : Double[0m[2m ; ops [%85:container/borrow#0][0m
    %88 = extract.field %86, 0[2m : Int[0m[2m ; ops [%86:container/borrow#0][0m
    %89 = const.int 1536288
    %90 = ptr_offset %1, %89[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %89:index/borrow#1][0m
    %91 = primitive_cast %90 from Ptr of Char to Ptr of Int[2m ; ops [%90:value/borrow#0][0m
    %92 = const.int 1
    %93 = const.int 0
    %94 = construct.tuple { %92, %93, %91 }[2m : Array of Int[0m[2m ; ops [%92:field/borrow#0, %93:field/borrow#1, %91:field/borrow#2][0m
    %95 = const.int 0
    %96 = extract.field %94, 1[2m : Int[0m[2m ; ops [%94:container/borrow#0][0m
    %97 = iadd %96, %95[2m ; ops [%96:value/borrow#0, %95:value/borrow#1][0m
    %98 = extract.field %94, 2[2m : Ptr[0m[2m ; ops [%94:container/borrow#0][0m
    %99 = ptr_offset %98, %97[2m : Ptr[0m[2m ; ops [%98:value/borrow#0, %97:index/borrow#1][0m
    %100 = load %99[2m : Int[0m[2m ; ops [%99:value/borrow#0][0m
    %101 = const.int 0
    %102 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %103 = iadd %102, %101[2m ; ops [%102:value/borrow#0, %101:value/borrow#1][0m
    %104 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %105 = ptr_offset %104, %103[2m : Ptr[0m[2m ; ops [%104:value/borrow#0, %103:index/borrow#1][0m
    %106 = load %105[2m : (Array of Double, Double, )[0m[2m ; ops [%105:value/borrow#0][0m
    %107 = fn_ref $fdn4.array_map.callback.4.kernel
    %108 = const.int 1536296
    %109 = ptr_offset %1, %108[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %108:index/borrow#1][0m
    %110 = primitive_cast %109 from Ptr of Char to Ptr of Char[2m ; ops [%109:value/borrow#0][0m
    %111 = call %107(%0, %110, %2, %3, %100, %106)[2m : Double[0m[2m ; ops [%107:callee/borrow#0, %0:value/consume#0, %110:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %100:value/borrow#4, %106:value/borrow#5][0m
    %112 = const.int 1
    %113 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %114 = iadd %113, %112[2m ; ops [%113:value/borrow#0, %112:value/borrow#1][0m
    %115 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %116 = ptr_offset %115, %114[2m : Ptr[0m[2m ; ops [%115:value/borrow#0, %114:index/borrow#1][0m
    %117 = load %116[2m : (Array of Double, Double, )[0m[2m ; ops [%116:value/borrow#0][0m
    %118 = const.int 1536304
    %119 = ptr_offset %1, %118[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %118:index/borrow#1][0m
    %120 = primitive_cast %119 from Ptr of Char to Ptr of Char[2m ; ops [%119:value/borrow#0][0m
    %121 = call %107(%0, %120, %2, %3, %100, %117)[2m : Double[0m[2m ; ops [%107:callee/borrow#0, %0:value/consume#0, %120:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %100:value/borrow#4, %117:value/borrow#5][0m
    %122 = const.int 2
    %123 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %124 = iadd %123, %122[2m ; ops [%123:value/borrow#0, %122:value/borrow#1][0m
    %125 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %126 = ptr_offset %125, %124[2m : Ptr[0m[2m ; ops [%125:value/borrow#0, %124:index/borrow#1][0m
    %127 = load %126[2m : (Array of Double, Double, )[0m[2m ; ops [%126:value/borrow#0][0m
    %128 = const.int 1536312
    %129 = ptr_offset %1, %128[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %128:index/borrow#1][0m
    %130 = primitive_cast %129 from Ptr of Char to Ptr of Char[2m ; ops [%129:value/borrow#0][0m
    %131 = call %107(%0, %130, %2, %3, %100, %127)[2m : Double[0m[2m ; ops [%107:callee/borrow#0, %0:value/consume#0, %130:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %100:value/borrow#4, %127:value/borrow#5][0m
    %132 = const.int 3
    %133 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %134 = iadd %133, %132[2m ; ops [%133:value/borrow#0, %132:value/borrow#1][0m
    %135 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %136 = ptr_offset %135, %134[2m : Ptr[0m[2m ; ops [%135:value/borrow#0, %134:index/borrow#1][0m
    %137 = load %136[2m : (Array of Double, Double, )[0m[2m ; ops [%136:value/borrow#0][0m
    %138 = const.int 1536320
    %139 = ptr_offset %1, %138[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %138:index/borrow#1][0m
    %140 = primitive_cast %139 from Ptr of Char to Ptr of Char[2m ; ops [%139:value/borrow#0][0m
    %141 = call %107(%0, %140, %2, %3, %100, %137)[2m : Double[0m[2m ; ops [%107:callee/borrow#0, %0:value/consume#0, %140:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %100:value/borrow#4, %137:value/borrow#5][0m
    %142 = construct.array_literal { %111, %121, %131, %141 }[2m : Array of Double[0m[2m ; ea stack#0[0m[2m ; ops [%111:element/consume#0, %121:element/consume#1, %131:element/consume#2, %141:element/consume#3][0m
    %143 = fn_ref $matrix_vec_mul_double
    %144 = const.int 4
    %145 = const.int 4
    %146 = const.int 1536328
    %147 = ptr_offset %1, %146[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %146:index/borrow#1][0m
    %148 = primitive_cast %147 from Ptr of Char to Ptr of Double[2m ; ops [%147:value/borrow#0][0m
    %149 = const.int 4
    %150 = const.int 0
    %151 = construct.tuple { %149, %150, %148 }[2m : Array of Double[0m[2m ; ops [%149:field/borrow#0, %150:field/borrow#1, %148:field/borrow#2][0m
    %152 = call %143(%144, %145, %15, %142, %151)[2m : Array of Double[0m[2m ; ops [%143:callee/borrow#0, %144:value/borrow#0, %145:value/borrow#1, %15:value/borrow#2, %142:value/borrow#3, %151:value/borrow#4][0m
    %153 = const.int 0
    %154 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %155 = iadd %154, %153[2m ; ops [%154:value/borrow#0, %153:value/borrow#1][0m
    %156 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %157 = ptr_offset %156, %155[2m : Ptr[0m[2m ; ops [%156:value/borrow#0, %155:index/borrow#1][0m
    %158 = load %157[2m : (Array of Double, Double, )[0m[2m ; ops [%157:value/borrow#0][0m
    %159 = const.int 0
    %160 = fn_ref $fdn4.array_mapi.callback.5.kernel
    %161 = const.int 1536360
    %162 = ptr_offset %1, %161[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %161:index/borrow#1][0m
    %163 = primitive_cast %162 from Ptr of Char to Ptr of Char[2m ; ops [%162:value/borrow#0][0m
    %164 = call %160(%0, %163, %2, %3, %152, %8, %21, %9, %100, %159, %158)[2m : Array of Double[0m[2m ; ops [%160:callee/borrow#0, %0:value/consume#0, %163:value/consume#1, %2:value/consume#2, %3:value/consume#3, %152:value/borrow#4, %8:value/borrow#5, %21:value/borrow#6, %9:value/borrow#7, %100:value/borrow#8, %159:value/borrow#9, %158:value/borrow#10][0m
    %165 = const.int 1
    %166 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %167 = iadd %166, %165[2m ; ops [%166:value/borrow#0, %165:value/borrow#1][0m
    %168 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %169 = ptr_offset %168, %167[2m : Ptr[0m[2m ; ops [%168:value/borrow#0, %167:index/borrow#1][0m
    %170 = load %169[2m : (Array of Double, Double, )[0m[2m ; ops [%169:value/borrow#0][0m
    %171 = const.int 1
    %172 = const.int 1536368
    %173 = ptr_offset %1, %172[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %172:index/borrow#1][0m
    %174 = primitive_cast %173 from Ptr of Char to Ptr of Char[2m ; ops [%173:value/borrow#0][0m
    %175 = call %160(%0, %174, %2, %3, %152, %8, %21, %9, %100, %171, %170)[2m : Array of Double[0m[2m ; ops [%160:callee/borrow#0, %0:value/consume#0, %174:value/consume#1, %2:value/consume#2, %3:value/consume#3, %152:value/borrow#4, %8:value/borrow#5, %21:value/borrow#6, %9:value/borrow#7, %100:value/borrow#8, %171:value/borrow#9, %170:value/borrow#10][0m
    %176 = const.int 2
    %177 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %178 = iadd %177, %176[2m ; ops [%177:value/borrow#0, %176:value/borrow#1][0m
    %179 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %180 = ptr_offset %179, %178[2m : Ptr[0m[2m ; ops [%179:value/borrow#0, %178:index/borrow#1][0m
    %181 = load %180[2m : (Array of Double, Double, )[0m[2m ; ops [%180:value/borrow#0][0m
    %182 = const.int 2
    %183 = const.int 1536376
    %184 = ptr_offset %1, %183[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %183:index/borrow#1][0m
    %185 = primitive_cast %184 from Ptr of Char to Ptr of Char[2m ; ops [%184:value/borrow#0][0m
    %186 = call %160(%0, %185, %2, %3, %152, %8, %21, %9, %100, %182, %181)[2m : Array of Double[0m[2m ; ops [%160:callee/borrow#0, %0:value/consume#0, %185:value/consume#1, %2:value/consume#2, %3:value/consume#3, %152:value/borrow#4, %8:value/borrow#5, %21:value/borrow#6, %9:value/borrow#7, %100:value/borrow#8, %182:value/borrow#9, %181:value/borrow#10][0m
    %187 = const.int 3
    %188 = extract.field %81, 1[2m : Int[0m[2m ; ops [%81:container/borrow#0][0m
    %189 = iadd %188, %187[2m ; ops [%188:value/borrow#0, %187:value/borrow#1][0m
    %190 = extract.field %81, 2[2m : Ptr[0m[2m ; ops [%81:container/borrow#0][0m
    %191 = ptr_offset %190, %189[2m : Ptr[0m[2m ; ops [%190:value/borrow#0, %189:index/borrow#1][0m
    %192 = load %191[2m : (Array of Double, Double, )[0m[2m ; ops [%191:value/borrow#0][0m
    %193 = const.int 3
    %194 = const.int 1536384
    %195 = ptr_offset %1, %194[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %194:index/borrow#1][0m
    %196 = primitive_cast %195 from Ptr of Char to Ptr of Char[2m ; ops [%195:value/borrow#0][0m
    %197 = call %160(%0, %196, %2, %3, %152, %8, %21, %9, %100, %193, %192)[2m : Array of Double[0m[2m ; ops [%160:callee/borrow#0, %0:value/consume#0, %196:value/consume#1, %2:value/consume#2, %3:value/consume#3, %152:value/borrow#4, %8:value/borrow#5, %21:value/borrow#6, %9:value/borrow#7, %100:value/borrow#8, %193:value/borrow#9, %192:value/borrow#10][0m
[35m    %227 = drop %152[2m : ()[0m[2m ; ops [%152:value/borrow#0][0m[0m
    %198 = construct.array_literal { %164, %175, %186, %197 }[2m : Array of Array of Double[0m[2m ; ea stack#1[0m[2m ; ops [%164:element/consume#0, %175:element/consume#1, %186:element/consume#2, %197:element/consume#3][0m
    %199 = const.int 0
    %200 = const.int 1
    %201 = primitive_cast %100 from Int to Double[2m ; ops [%100:value/borrow#0][0m
    %202 = primitive_cast %200 from Int to Double[2m ; ops [%200:value/borrow#0][0m
    %203 = fadd %201, %202[2m ; ops [%201:value/borrow#0, %202:value/borrow#1][0m
    %204 = primitive_cast %88 from Int to Double[2m ; ops [%88:value/borrow#0][0m
    %205 = fmod %203, %204[2m ; ops [%203:value/borrow#0, %204:value/borrow#1][0m
    %206 = extract.field %94, 1[2m : Int[0m[2m ; ops [%94:container/borrow#0][0m
    %207 = iadd %206, %199[2m ; ops [%206:value/borrow#0, %199:value/borrow#1][0m
    %208 = primitive_cast %205 from Double to Int[2m ; ops [%205:value/borrow#0][0m
    %209 = extract.field %94, 2[2m : Ptr[0m[2m ; ops [%94:container/borrow#0][0m
    %210 = ptr_offset %209, %207[2m : Ptr[0m[2m ; ops [%209:value/borrow#0, %207:index/borrow#1][0m
    %211 = store %210, %208[2m : ()[0m[2m ; ops [%210:value/borrow#0, %208:element/consume#1][0m
    %212 = fn_ref $vec_dot_double
    %213 = call %212(%27, %142)[2m : Double[0m[2m ; ops [%212:callee/borrow#0, %27:value/borrow#0, %142:value/borrow#1][0m
    %214 = fadd %9, %213[2m ; ops [%9:value/borrow#0, %213:value/borrow#1][0m
    return %214[2m ; ops [%214:return/consume#0][0m
}
fn fdn4.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = const.int 4
    %30 = ptr_offset %2, %29[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %29:index/borrow#1][0m
    %31 = load %30[2m : Ptr[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fn_ref $ylc_read_inlet_node_lane_i32
    %33 = const.int 0
    %34 = call %32(%31, %3, %33)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %33:value/borrow#2][0m
    %35 = const.int 5
    %36 = ptr_offset %2, %35[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %35:index/borrow#1][0m
    %37 = load %36[2m : Ptr[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = fn_ref $ylc_read_inlet_node_lane_i32
    %39 = const.int 0
    %40 = call %38(%37, %3, %39)[2m : Double[0m[2m ; ops [%38:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %39:value/borrow#2][0m
    %41 = fn_ref $fdn4.kernel
    %42 = call %41(%0, %1, %3, %4, %10, %16, %22, %28, %34, %40)[2m : Double[0m[2m ; ops [%41:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/consume#4, %16:value/consume#5, %22:value/consume#6, %28:value/consume#7, %34:value/borrow#8, %40:value/borrow#9][0m
    %43 = fn_ref $ylc_get_output_buf
    %44 = call %43(%0)[2m : Ptr[0m[2m ; ops [%43:callee/borrow#0, %0:value/borrow#0][0m
    %45 = ptr_offset %44, %3[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %3:index/borrow#1][0m
    %46 = store %45, %42[2m : ()[0m[2m ; ops [%45:value/borrow#0, %42:element/consume#1][0m
    %47 = const.void
    return %47[2m ; ops [%47:return/consume#0][0m
}
fn fdn4.array_map.callback.4.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn fdn4.array_map.callback.4.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn fdn4.array_map.callback.4.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 w[2m: Int @borrow[0m, %5 arg0[2m: (Array of Double, Double, ) @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = extract.field %5, 0[2m : Array of Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = extract.field %5, 1[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = fdiv %7, %3[2m ; ops [%7:value/borrow#0, %3:value/borrow#1][0m
    %9 = primitive_cast %4 from Int to Double[2m ; ops [%4:value/borrow#0][0m
    %10 = fsub %9, %8[2m ; ops [%9:value/borrow#0, %8:value/borrow#1][0m
    %11 = extract.field %6, 0[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %12 = extract.field %6, 1[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %13 = extract.field %6, 2[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    %14 = fn_ref $ylc_audio_tabread_samp_kernel
    %15 = primitive_cast %1 from Ptr of Char to Ptr of Double[2m ; ops [%1:value/borrow#0][0m
    %16 = call %14(%15, %3, %11, %12, %13, %10)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %15:value/borrow#0, %3:value/borrow#1, %11:value/borrow#2, %12:value/borrow#3, %13:value/borrow#4, %10:value/borrow#5][0m
    return %16[2m ; ops [%16:return/consume#0][0m
}
extern fn ylc_audio_tabread_samp_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Int @borrow[0m, %4 arg4[2m: Ptr @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn fdn4.array_mapi.callback.5.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn fdn4.array_mapi.callback.5.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn fdn4.array_mapi.callback.5.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 mmy[2m: Array of Double @borrow[0m, %5 rt60[2m: Double @borrow[0m, %6 b[2m: Array of Double @borrow[0m, %7 inp[2m: Double @borrow[0m, %8 w[2m: Int @borrow[0m, %9 i[2m: Int @borrow[0m, %10 arg1[2m: (Array of Double, Double, ) @borrow[0m)[2m -> Array of Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %11 = extract.field %10, 0[2m : Array of Double[0m[2m ; ops [%10:container/borrow#0][0m
    %12 = extract.field %10, 1[2m : Double[0m[2m ; ops [%10:container/borrow#0][0m
    %13 = extract.field %6, 1[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %14 = iadd %13, %9[2m ; ops [%13:value/borrow#0, %9:value/borrow#1][0m
    %15 = extract.field %6, 2[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    %16 = ptr_offset %15, %14[2m : Ptr[0m[2m ; ops [%15:value/borrow#0, %14:index/borrow#1][0m
    %17 = load %16[2m : Double[0m[2m ; ops [%16:value/borrow#0][0m
    %18 = fmul %7, %17[2m ; ops [%7:value/borrow#0, %17:value/borrow#1][0m
    %19 = fn_ref $pow
    %20 = const.double 10
    %21 = const.double -3
    %22 = fmul %21, %12[2m ; ops [%21:value/borrow#0, %12:value/borrow#1][0m
    %23 = fdiv %22, %5[2m ; ops [%22:value/borrow#0, %5:value/borrow#1][0m
    %24 = call %19(%20, %23)[2m : Double[0m[2m ; ops [%19:callee/borrow#0, %20:value/borrow#0, %23:value/borrow#1][0m
    %25 = extract.field %4, 1[2m : Int[0m[2m ; ops [%4:container/borrow#0][0m
    %26 = iadd %25, %9[2m ; ops [%25:value/borrow#0, %9:value/borrow#1][0m
    %27 = extract.field %4, 2[2m : Ptr[0m[2m ; ops [%4:container/borrow#0][0m
    %28 = ptr_offset %27, %26[2m : Ptr[0m[2m ; ops [%27:value/borrow#0, %26:index/borrow#1][0m
    %29 = load %28[2m : Double[0m[2m ; ops [%28:value/borrow#0][0m
    %30 = fmul %24, %29[2m ; ops [%24:value/borrow#0, %29:value/borrow#1][0m
    %31 = fadd %18, %30[2m ; ops [%18:value/borrow#0, %30:value/borrow#1][0m
    %32 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %33 = iadd %32, %8[2m ; ops [%32:value/borrow#0, %8:value/borrow#1][0m
    %34 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %35 = ptr_offset %34, %33[2m : Ptr[0m[2m ; ops [%34:value/borrow#0, %33:index/borrow#1][0m
    %36 = store %35, %31[2m : ()[0m[2m ; ops [%35:value/borrow#0, %31:element/consume#1][0m
[35m    %37 = dup %11[2m : ()[0m[2m ; ops [%11:value/borrow#0][0m[0m
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn room_fdn4.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $room_fdn4.frame
    %3 = const.string "room_fdn4"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 1536392
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $room_fdn4.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $room_fdn4.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn room_fdn4.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn4.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn room_fdn4.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.0199
    %6 = const.double 0.0291
    %7 = const.double 0.044
    %8 = const.double 0.0653
    %9 = const.double 0.5
    %10 = fn_ref $fdn4.kernel
    %11 = const.int 0
    %12 = ptr_offset %1, %11[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Char[2m ; ops [%12:value/borrow#0][0m
    %14 = call %10(%0, %13, %2, %3, %5, %6, %7, %8, %9, %4)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %0:value/consume#0, %13:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/borrow#8, %4:value/borrow#9][0m
    return %14[2m ; ops [%14:return/consume#0][0m
}
fn room_fdn4.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $room_fdn4.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn concert_fdn4.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $concert_fdn4.frame
    %3 = const.string "concert_fdn4"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 1536392
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $concert_fdn4.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $concert_fdn4.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn concert_fdn4.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn4.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn concert_fdn4.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.0527
    %6 = const.double 0.0787
    %7 = const.double 0.1171
    %8 = const.double 0.1733
    %9 = const.double 2
    %10 = fn_ref $fdn4.kernel
    %11 = const.int 0
    %12 = ptr_offset %1, %11[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Char[2m ; ops [%12:value/borrow#0][0m
    %14 = call %10(%0, %13, %2, %3, %5, %6, %7, %8, %9, %4)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %0:value/consume#0, %13:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/borrow#8, %4:value/borrow#9][0m
    return %14[2m ; ops [%14:return/consume#0][0m
}
fn concert_fdn4.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $concert_fdn4.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn cathedral_fdn4.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $cathedral_fdn4.frame
    %3 = const.string "cathedral_fdn4"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 1536392
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $cathedral_fdn4.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $cathedral_fdn4.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn cathedral_fdn4.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn4.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn cathedral_fdn4.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.1249
    %6 = const.double 0.1873
    %7 = const.double 0.2797
    %8 = const.double 0.4177
    %9 = const.double 5
    %10 = fn_ref $fdn4.kernel
    %11 = const.int 0
    %12 = ptr_offset %1, %11[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Char[2m ; ops [%12:value/borrow#0][0m
    %14 = call %10(%0, %13, %2, %3, %5, %6, %7, %8, %9, %4)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %0:value/consume#0, %13:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/borrow#8, %4:value/borrow#9][0m
    return %14[2m ; ops [%14:return/consume#0][0m
}
fn cathedral_fdn4.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $cathedral_fdn4.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn fdn8.cons(%0 d1[2m: Double @borrow[0m, %1 d2[2m: Double @borrow[0m, %2 d3[2m: Double @borrow[0m, %3 d4[2m: Double @borrow[0m, %4 d5[2m: Double @borrow[0m, %5 d6[2m: Double @borrow[0m, %6 d7[2m: Double @borrow[0m, %7 d8[2m: Double @borrow[0m, %8 rt60[2m: Double @borrow[0m, %9 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = fn_ref $ylc_create_audio_frame_node
    %11 = fn_ref $fdn8.frame
    %12 = const.string "fdn8"
    %13 = const.int 10
    %14 = const.int 1
    %15 = const.int 3073224
    %16 = call %10(%11, %13, %14, %15, %12)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %11:value/borrow#0, %13:value/borrow#1, %14:value/borrow#2, %15:value/borrow#3, %12:value/borrow#4][0m
    %17 = fn_ref $ylc_audio_node_set_state_init
    %18 = fn_ref $fdn8.init
    %19 = call %17(%16, %18)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %16:value/borrow#0, %18:value/borrow#1][0m
    %20 = fn_ref $ylc_audio_node_inline_state
    %21 = call %20(%16)[2m : Ptr[0m[2m ; ops [%20:callee/borrow#0, %16:value/borrow#0][0m
    %22 = fn_ref $fdn8.init
    %23 = call %22(%21)[2m : ()[0m[2m ; ops [%22:callee/borrow#0, %21:value/borrow#0][0m
    %24 = fn_ref $node_connect_input
    %25 = fn_ref $const_sig
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = const.int 0
    %28 = call %24(%27, %16, %26)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %27:value/borrow#0, %16:value/borrow#1, %26:value/borrow#2][0m
    %29 = fn_ref $const_sig
    %30 = call %29(%1)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %1:value/borrow#0][0m
    %31 = const.int 1
    %32 = call %24(%31, %16, %30)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %31:value/borrow#0, %16:value/borrow#1, %30:value/borrow#2][0m
    %33 = fn_ref $const_sig
    %34 = call %33(%2)[2m : Ptr[0m[2m ; ops [%33:callee/borrow#0, %2:value/borrow#0][0m
    %35 = const.int 2
    %36 = call %24(%35, %16, %34)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %35:value/borrow#0, %16:value/borrow#1, %34:value/borrow#2][0m
    %37 = fn_ref $const_sig
    %38 = call %37(%3)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %3:value/borrow#0][0m
    %39 = const.int 3
    %40 = call %24(%39, %16, %38)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %39:value/borrow#0, %16:value/borrow#1, %38:value/borrow#2][0m
    %41 = fn_ref $const_sig
    %42 = call %41(%4)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %4:value/borrow#0][0m
    %43 = const.int 4
    %44 = call %24(%43, %16, %42)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %43:value/borrow#0, %16:value/borrow#1, %42:value/borrow#2][0m
    %45 = fn_ref $const_sig
    %46 = call %45(%5)[2m : Ptr[0m[2m ; ops [%45:callee/borrow#0, %5:value/borrow#0][0m
    %47 = const.int 5
    %48 = call %24(%47, %16, %46)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %47:value/borrow#0, %16:value/borrow#1, %46:value/borrow#2][0m
    %49 = fn_ref $const_sig
    %50 = call %49(%6)[2m : Ptr[0m[2m ; ops [%49:callee/borrow#0, %6:value/borrow#0][0m
    %51 = const.int 6
    %52 = call %24(%51, %16, %50)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %51:value/borrow#0, %16:value/borrow#1, %50:value/borrow#2][0m
    %53 = fn_ref $const_sig
    %54 = call %53(%7)[2m : Ptr[0m[2m ; ops [%53:callee/borrow#0, %7:value/borrow#0][0m
    %55 = const.int 7
    %56 = call %24(%55, %16, %54)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %55:value/borrow#0, %16:value/borrow#1, %54:value/borrow#2][0m
    %57 = fn_ref $const_sig
    %58 = call %57(%8)[2m : Ptr[0m[2m ; ops [%57:callee/borrow#0, %8:value/borrow#0][0m
    %59 = const.int 8
    %60 = call %24(%59, %16, %58)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %59:value/borrow#0, %16:value/borrow#1, %58:value/borrow#2][0m
    %61 = fn_ref $const_sig
    %62 = call %61(%9)[2m : Ptr[0m[2m ; ops [%61:callee/borrow#0, %9:value/borrow#0][0m
    %63 = const.int 9
    %64 = call %24(%63, %16, %62)[2m : ()[0m[2m ; ops [%24:callee/borrow#0, %63:value/borrow#0, %16:value/borrow#1, %62:value/borrow#2][0m
    return %16[2m ; ops [%16:return/consume#0][0m
}
fn fdn8.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 3073192
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 3073160
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 3073128
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 3073096
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 3073064
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = const.char '\x00'
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 3073032
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = const.char '\x00'
    %24 = store %22, %23[2m : ()[0m[2m ; ops [%22:value/borrow#0, %23:element/consume#1][0m
    %25 = const.int 3073000
    %26 = ptr_offset %0, %25[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %25:index/borrow#1][0m
    %27 = const.char '\x00'
    %28 = store %26, %27[2m : ()[0m[2m ; ops [%26:value/borrow#0, %27:element/consume#1][0m
    %29 = const.int 3072968
    %30 = ptr_offset %0, %29[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %29:index/borrow#1][0m
    %31 = const.char '\x00'
    %32 = store %30, %31[2m : ()[0m[2m ; ops [%30:value/borrow#0, %31:element/consume#1][0m
    %33 = const.int 3072904
    %34 = ptr_offset %0, %33[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %33:index/borrow#1][0m
    %35 = primitive_cast %34 from Ptr of Char to Ptr of Double[2m ; ops [%34:value/borrow#0][0m
    %36 = const.double 0
    %37 = store %35, %36[2m : ()[0m[2m ; ops [%35:value/borrow#0, %36:element/consume#1][0m
    %38 = const.int 3072896
    %39 = ptr_offset %0, %38[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %38:index/borrow#1][0m
    %40 = const.char '\x00'
    %41 = store %39, %40[2m : ()[0m[2m ; ops [%39:value/borrow#0, %40:element/consume#1][0m
    %42 = const.int 3072888
    %43 = ptr_offset %0, %42[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %42:index/borrow#1][0m
    %44 = const.char '\x00'
    %45 = store %43, %44[2m : ()[0m[2m ; ops [%43:value/borrow#0, %44:element/consume#1][0m
    %46 = const.int 3072880
    %47 = ptr_offset %0, %46[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %46:index/borrow#1][0m
    %48 = const.char '\x00'
    %49 = store %47, %48[2m : ()[0m[2m ; ops [%47:value/borrow#0, %48:element/consume#1][0m
    %50 = const.int 3072872
    %51 = ptr_offset %0, %50[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %50:index/borrow#1][0m
    %52 = const.char '\x00'
    %53 = store %51, %52[2m : ()[0m[2m ; ops [%51:value/borrow#0, %52:element/consume#1][0m
    %54 = const.int 3072864
    %55 = ptr_offset %0, %54[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %54:index/borrow#1][0m
    %56 = const.char '\x00'
    %57 = store %55, %56[2m : ()[0m[2m ; ops [%55:value/borrow#0, %56:element/consume#1][0m
    %58 = const.int 3072856
    %59 = ptr_offset %0, %58[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %58:index/borrow#1][0m
    %60 = const.char '\x00'
    %61 = store %59, %60[2m : ()[0m[2m ; ops [%59:value/borrow#0, %60:element/consume#1][0m
    %62 = const.int 3072848
    %63 = ptr_offset %0, %62[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %62:index/borrow#1][0m
    %64 = const.char '\x00'
    %65 = store %63, %64[2m : ()[0m[2m ; ops [%63:value/borrow#0, %64:element/consume#1][0m
    %66 = const.int 3072840
    %67 = ptr_offset %0, %66[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %66:index/borrow#1][0m
    %68 = const.char '\x00'
    %69 = store %67, %68[2m : ()[0m[2m ; ops [%67:value/borrow#0, %68:element/consume#1][0m
    %70 = const.int 3072832
    %71 = ptr_offset %0, %70[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %70:index/borrow#1][0m
    %72 = primitive_cast %71 from Ptr of Char to Ptr of Int[2m ; ops [%71:value/borrow#0][0m
    %73 = const.int 0
    %74 = store %72, %73[2m : ()[0m[2m ; ops [%72:value/borrow#0, %73:element/consume#1][0m
    %75 = const.int 2688832
    %76 = ptr_offset %0, %75[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %75:index/borrow#1][0m
    %77 = fn_ref $ylc_audio_memzero
    %78 = const.int 384000
    %79 = call %77(%76, %78)[2m : ()[0m[2m ; ops [%77:callee/borrow#0, %76:value/borrow#0, %78:value/borrow#1][0m
    %80 = const.int 2304832
    %81 = ptr_offset %0, %80[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %80:index/borrow#1][0m
    %82 = fn_ref $ylc_audio_memzero
    %83 = const.int 384000
    %84 = call %82(%81, %83)[2m : ()[0m[2m ; ops [%82:callee/borrow#0, %81:value/borrow#0, %83:value/borrow#1][0m
    %85 = const.int 1920832
    %86 = ptr_offset %0, %85[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %85:index/borrow#1][0m
    %87 = fn_ref $ylc_audio_memzero
    %88 = const.int 384000
    %89 = call %87(%86, %88)[2m : ()[0m[2m ; ops [%87:callee/borrow#0, %86:value/borrow#0, %88:value/borrow#1][0m
    %90 = const.int 1536832
    %91 = ptr_offset %0, %90[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %90:index/borrow#1][0m
    %92 = fn_ref $ylc_audio_memzero
    %93 = const.int 384000
    %94 = call %92(%91, %93)[2m : ()[0m[2m ; ops [%92:callee/borrow#0, %91:value/borrow#0, %93:value/borrow#1][0m
    %95 = const.int 1152832
    %96 = ptr_offset %0, %95[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %95:index/borrow#1][0m
    %97 = fn_ref $ylc_audio_memzero
    %98 = const.int 384000
    %99 = call %97(%96, %98)[2m : ()[0m[2m ; ops [%97:callee/borrow#0, %96:value/borrow#0, %98:value/borrow#1][0m
    %100 = const.int 768832
    %101 = ptr_offset %0, %100[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %100:index/borrow#1][0m
    %102 = fn_ref $ylc_audio_memzero
    %103 = const.int 384000
    %104 = call %102(%101, %103)[2m : ()[0m[2m ; ops [%102:callee/borrow#0, %101:value/borrow#0, %103:value/borrow#1][0m
    %105 = const.int 384832
    %106 = ptr_offset %0, %105[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %105:index/borrow#1][0m
    %107 = fn_ref $ylc_audio_memzero
    %108 = const.int 384000
    %109 = call %107(%106, %108)[2m : ()[0m[2m ; ops [%107:callee/borrow#0, %106:value/borrow#0, %108:value/borrow#1][0m
    %110 = const.int 832
    %111 = ptr_offset %0, %110[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %110:index/borrow#1][0m
    %112 = fn_ref $ylc_audio_memzero
    %113 = const.int 384000
    %114 = call %112(%111, %113)[2m : ()[0m[2m ; ops [%112:callee/borrow#0, %111:value/borrow#0, %113:value/borrow#1][0m
    %115 = const.int 640
    %116 = ptr_offset %0, %115[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %115:index/borrow#1][0m
    %117 = primitive_cast %116 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%116:value/borrow#0][0m
    %118 = const.undef
    %631 = load_owned %117[2m : (Array of Double, Double, )[0m[2m ; ops [%117:value/borrow#0][0m
    %119 = store %117, %118[2m : ()[0m[2m ; ops [%117:value/borrow#0, %118:element/consume#1][0m
[35m    %632 = drop %631[2m : ()[0m[2m ; ops [%631:value/borrow#0][0m[0m
    %120 = const.int 576
    %121 = ptr_offset %0, %120[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %120:index/borrow#1][0m
    %122 = primitive_cast %121 from Ptr of Char to Ptr of Double[2m ; ops [%121:value/borrow#0][0m
    %123 = const.double 0
    %124 = store %122, %123[2m : ()[0m[2m ; ops [%122:value/borrow#0, %123:element/consume#1][0m
    %125 = const.int 512
    %126 = ptr_offset %0, %125[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %125:index/borrow#1][0m
    %127 = primitive_cast %126 from Ptr of Char to Ptr of Double[2m ; ops [%126:value/borrow#0][0m
    %128 = const.double 0
    %129 = store %127, %128[2m : ()[0m[2m ; ops [%127:value/borrow#0, %128:element/consume#1][0m
    %130 = const.int 0
    %131 = ptr_offset %0, %130[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %130:index/borrow#1][0m
    %132 = primitive_cast %131 from Ptr of Char to Ptr of Double[2m ; ops [%131:value/borrow#0][0m
    %133 = const.double 0
    %134 = store %132, %133[2m : ()[0m[2m ; ops [%132:value/borrow#0, %133:element/consume#1][0m
    %135 = const.int 3072960
    %136 = ptr_offset %0, %135[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %135:index/borrow#1][0m
    %137 = primitive_cast %136 from Ptr of Char to Ptr of Double[2m ; ops [%136:value/borrow#0][0m
    %138 = const.double 0
    %139 = store %137, %138[2m : ()[0m[2m ; ops [%137:value/borrow#0, %138:element/consume#1][0m
    %140 = const.int 3072952
    %141 = ptr_offset %0, %140[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %140:index/borrow#1][0m
    %142 = primitive_cast %141 from Ptr of Char to Ptr of Double[2m ; ops [%141:value/borrow#0][0m
    %143 = const.double 0
    %144 = store %142, %143[2m : ()[0m[2m ; ops [%142:value/borrow#0, %143:element/consume#1][0m
    %145 = const.int 3072944
    %146 = ptr_offset %0, %145[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %145:index/borrow#1][0m
    %147 = primitive_cast %146 from Ptr of Char to Ptr of Double[2m ; ops [%146:value/borrow#0][0m
    %148 = const.double 0
    %149 = store %147, %148[2m : ()[0m[2m ; ops [%147:value/borrow#0, %148:element/consume#1][0m
    %150 = const.int 3072936
    %151 = ptr_offset %0, %150[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %150:index/borrow#1][0m
    %152 = primitive_cast %151 from Ptr of Char to Ptr of Double[2m ; ops [%151:value/borrow#0][0m
    %153 = const.double 0
    %154 = store %152, %153[2m : ()[0m[2m ; ops [%152:value/borrow#0, %153:element/consume#1][0m
    %155 = const.int 3072928
    %156 = ptr_offset %0, %155[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %155:index/borrow#1][0m
    %157 = primitive_cast %156 from Ptr of Char to Ptr of Double[2m ; ops [%156:value/borrow#0][0m
    %158 = const.double 0
    %159 = store %157, %158[2m : ()[0m[2m ; ops [%157:value/borrow#0, %158:element/consume#1][0m
    %160 = const.int 3072920
    %161 = ptr_offset %0, %160[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %160:index/borrow#1][0m
    %162 = primitive_cast %161 from Ptr of Char to Ptr of Double[2m ; ops [%161:value/borrow#0][0m
    %163 = const.double 0
    %164 = store %162, %163[2m : ()[0m[2m ; ops [%162:value/borrow#0, %163:element/consume#1][0m
    %165 = const.int 3072912
    %166 = ptr_offset %0, %165[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %165:index/borrow#1][0m
    %167 = primitive_cast %166 from Ptr of Char to Ptr of Double[2m ; ops [%166:value/borrow#0][0m
    %168 = const.double 0
    %169 = store %167, %168[2m : ()[0m[2m ; ops [%167:value/borrow#0, %168:element/consume#1][0m
    %170 = const.int 3072904
    %171 = ptr_offset %0, %170[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %170:index/borrow#1][0m
    %172 = primitive_cast %171 from Ptr of Char to Ptr of Double[2m ; ops [%171:value/borrow#0][0m
    %173 = const.double 0
    %174 = store %172, %173[2m : ()[0m[2m ; ops [%172:value/borrow#0, %173:element/consume#1][0m
    %175 = const.int 3072832
    %176 = ptr_offset %0, %175[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %175:index/borrow#1][0m
    %177 = primitive_cast %176 from Ptr of Char to Ptr of Int[2m ; ops [%176:value/borrow#0][0m
    %178 = const.int 0
    %179 = store %177, %178[2m : ()[0m[2m ; ops [%177:value/borrow#0, %178:element/consume#1][0m
    %180 = const.int 632
    %181 = ptr_offset %0, %180[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %180:index/borrow#1][0m
    %182 = primitive_cast %181 from Ptr of Char to Ptr of Double[2m ; ops [%181:value/borrow#0][0m
    %183 = const.double 0.125
    %184 = store %182, %183[2m : ()[0m[2m ; ops [%182:value/borrow#0, %183:element/consume#1][0m
    %185 = const.int 624
    %186 = ptr_offset %0, %185[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %185:index/borrow#1][0m
    %187 = primitive_cast %186 from Ptr of Char to Ptr of Double[2m ; ops [%186:value/borrow#0][0m
    %188 = const.double 0.125
    %189 = store %187, %188[2m : ()[0m[2m ; ops [%187:value/borrow#0, %188:element/consume#1][0m
    %190 = const.int 616
    %191 = ptr_offset %0, %190[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %190:index/borrow#1][0m
    %192 = primitive_cast %191 from Ptr of Char to Ptr of Double[2m ; ops [%191:value/borrow#0][0m
    %193 = const.double 0.125
    %194 = store %192, %193[2m : ()[0m[2m ; ops [%192:value/borrow#0, %193:element/consume#1][0m
    %195 = const.int 608
    %196 = ptr_offset %0, %195[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %195:index/borrow#1][0m
    %197 = primitive_cast %196 from Ptr of Char to Ptr of Double[2m ; ops [%196:value/borrow#0][0m
    %198 = const.double 0.125
    %199 = store %197, %198[2m : ()[0m[2m ; ops [%197:value/borrow#0, %198:element/consume#1][0m
    %200 = const.int 600
    %201 = ptr_offset %0, %200[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %200:index/borrow#1][0m
    %202 = primitive_cast %201 from Ptr of Char to Ptr of Double[2m ; ops [%201:value/borrow#0][0m
    %203 = const.double 0.125
    %204 = store %202, %203[2m : ()[0m[2m ; ops [%202:value/borrow#0, %203:element/consume#1][0m
    %205 = const.int 592
    %206 = ptr_offset %0, %205[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %205:index/borrow#1][0m
    %207 = primitive_cast %206 from Ptr of Char to Ptr of Double[2m ; ops [%206:value/borrow#0][0m
    %208 = const.double 0.125
    %209 = store %207, %208[2m : ()[0m[2m ; ops [%207:value/borrow#0, %208:element/consume#1][0m
    %210 = const.int 584
    %211 = ptr_offset %0, %210[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %210:index/borrow#1][0m
    %212 = primitive_cast %211 from Ptr of Char to Ptr of Double[2m ; ops [%211:value/borrow#0][0m
    %213 = const.double 0.125
    %214 = store %212, %213[2m : ()[0m[2m ; ops [%212:value/borrow#0, %213:element/consume#1][0m
    %215 = const.int 576
    %216 = ptr_offset %0, %215[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %215:index/borrow#1][0m
    %217 = primitive_cast %216 from Ptr of Char to Ptr of Double[2m ; ops [%216:value/borrow#0][0m
    %218 = const.double 0.125
    %219 = store %217, %218[2m : ()[0m[2m ; ops [%217:value/borrow#0, %218:element/consume#1][0m
    %220 = const.int 568
    %221 = ptr_offset %0, %220[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %220:index/borrow#1][0m
    %222 = primitive_cast %221 from Ptr of Char to Ptr of Double[2m ; ops [%221:value/borrow#0][0m
    %223 = const.double 1
    %224 = store %222, %223[2m : ()[0m[2m ; ops [%222:value/borrow#0, %223:element/consume#1][0m
    %225 = const.int 560
    %226 = ptr_offset %0, %225[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %225:index/borrow#1][0m
    %227 = primitive_cast %226 from Ptr of Char to Ptr of Double[2m ; ops [%226:value/borrow#0][0m
    %228 = const.double 1
    %229 = store %227, %228[2m : ()[0m[2m ; ops [%227:value/borrow#0, %228:element/consume#1][0m
    %230 = const.int 552
    %231 = ptr_offset %0, %230[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %230:index/borrow#1][0m
    %232 = primitive_cast %231 from Ptr of Char to Ptr of Double[2m ; ops [%231:value/borrow#0][0m
    %233 = const.double 1
    %234 = store %232, %233[2m : ()[0m[2m ; ops [%232:value/borrow#0, %233:element/consume#1][0m
    %235 = const.int 544
    %236 = ptr_offset %0, %235[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %235:index/borrow#1][0m
    %237 = primitive_cast %236 from Ptr of Char to Ptr of Double[2m ; ops [%236:value/borrow#0][0m
    %238 = const.double 1
    %239 = store %237, %238[2m : ()[0m[2m ; ops [%237:value/borrow#0, %238:element/consume#1][0m
    %240 = const.int 536
    %241 = ptr_offset %0, %240[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %240:index/borrow#1][0m
    %242 = primitive_cast %241 from Ptr of Char to Ptr of Double[2m ; ops [%241:value/borrow#0][0m
    %243 = const.double 1
    %244 = store %242, %243[2m : ()[0m[2m ; ops [%242:value/borrow#0, %243:element/consume#1][0m
    %245 = const.int 528
    %246 = ptr_offset %0, %245[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %245:index/borrow#1][0m
    %247 = primitive_cast %246 from Ptr of Char to Ptr of Double[2m ; ops [%246:value/borrow#0][0m
    %248 = const.double 1
    %249 = store %247, %248[2m : ()[0m[2m ; ops [%247:value/borrow#0, %248:element/consume#1][0m
    %250 = const.int 520
    %251 = ptr_offset %0, %250[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %250:index/borrow#1][0m
    %252 = primitive_cast %251 from Ptr of Char to Ptr of Double[2m ; ops [%251:value/borrow#0][0m
    %253 = const.double 1
    %254 = store %252, %253[2m : ()[0m[2m ; ops [%252:value/borrow#0, %253:element/consume#1][0m
    %255 = const.int 512
    %256 = ptr_offset %0, %255[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %255:index/borrow#1][0m
    %257 = primitive_cast %256 from Ptr of Char to Ptr of Double[2m ; ops [%256:value/borrow#0][0m
    %258 = const.double 1
    %259 = store %257, %258[2m : ()[0m[2m ; ops [%257:value/borrow#0, %258:element/consume#1][0m
    %260 = const.int 504
    %261 = ptr_offset %0, %260[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %260:index/borrow#1][0m
    %262 = primitive_cast %261 from Ptr of Char to Ptr of Double[2m ; ops [%261:value/borrow#0][0m
    %263 = const.double -0.75
    %264 = store %262, %263[2m : ()[0m[2m ; ops [%262:value/borrow#0, %263:element/consume#1][0m
    %265 = const.int 496
    %266 = ptr_offset %0, %265[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %265:index/borrow#1][0m
    %267 = primitive_cast %266 from Ptr of Char to Ptr of Double[2m ; ops [%266:value/borrow#0][0m
    %268 = const.double 0.25
    %269 = store %267, %268[2m : ()[0m[2m ; ops [%267:value/borrow#0, %268:element/consume#1][0m
    %270 = const.int 488
    %271 = ptr_offset %0, %270[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %270:index/borrow#1][0m
    %272 = primitive_cast %271 from Ptr of Char to Ptr of Double[2m ; ops [%271:value/borrow#0][0m
    %273 = const.double 0.25
    %274 = store %272, %273[2m : ()[0m[2m ; ops [%272:value/borrow#0, %273:element/consume#1][0m
    %275 = const.int 480
    %276 = ptr_offset %0, %275[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %275:index/borrow#1][0m
    %277 = primitive_cast %276 from Ptr of Char to Ptr of Double[2m ; ops [%276:value/borrow#0][0m
    %278 = const.double 0.25
    %279 = store %277, %278[2m : ()[0m[2m ; ops [%277:value/borrow#0, %278:element/consume#1][0m
    %280 = const.int 472
    %281 = ptr_offset %0, %280[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %280:index/borrow#1][0m
    %282 = primitive_cast %281 from Ptr of Char to Ptr of Double[2m ; ops [%281:value/borrow#0][0m
    %283 = const.double 0.25
    %284 = store %282, %283[2m : ()[0m[2m ; ops [%282:value/borrow#0, %283:element/consume#1][0m
    %285 = const.int 464
    %286 = ptr_offset %0, %285[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %285:index/borrow#1][0m
    %287 = primitive_cast %286 from Ptr of Char to Ptr of Double[2m ; ops [%286:value/borrow#0][0m
    %288 = const.double 0.25
    %289 = store %287, %288[2m : ()[0m[2m ; ops [%287:value/borrow#0, %288:element/consume#1][0m
    %290 = const.int 456
    %291 = ptr_offset %0, %290[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %290:index/borrow#1][0m
    %292 = primitive_cast %291 from Ptr of Char to Ptr of Double[2m ; ops [%291:value/borrow#0][0m
    %293 = const.double 0.25
    %294 = store %292, %293[2m : ()[0m[2m ; ops [%292:value/borrow#0, %293:element/consume#1][0m
    %295 = const.int 448
    %296 = ptr_offset %0, %295[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %295:index/borrow#1][0m
    %297 = primitive_cast %296 from Ptr of Char to Ptr of Double[2m ; ops [%296:value/borrow#0][0m
    %298 = const.double 0.25
    %299 = store %297, %298[2m : ()[0m[2m ; ops [%297:value/borrow#0, %298:element/consume#1][0m
    %300 = const.int 440
    %301 = ptr_offset %0, %300[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %300:index/borrow#1][0m
    %302 = primitive_cast %301 from Ptr of Char to Ptr of Double[2m ; ops [%301:value/borrow#0][0m
    %303 = const.double 0.25
    %304 = store %302, %303[2m : ()[0m[2m ; ops [%302:value/borrow#0, %303:element/consume#1][0m
    %305 = const.int 432
    %306 = ptr_offset %0, %305[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %305:index/borrow#1][0m
    %307 = primitive_cast %306 from Ptr of Char to Ptr of Double[2m ; ops [%306:value/borrow#0][0m
    %308 = const.double -0.75
    %309 = store %307, %308[2m : ()[0m[2m ; ops [%307:value/borrow#0, %308:element/consume#1][0m
    %310 = const.int 424
    %311 = ptr_offset %0, %310[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %310:index/borrow#1][0m
    %312 = primitive_cast %311 from Ptr of Char to Ptr of Double[2m ; ops [%311:value/borrow#0][0m
    %313 = const.double 0.25
    %314 = store %312, %313[2m : ()[0m[2m ; ops [%312:value/borrow#0, %313:element/consume#1][0m
    %315 = const.int 416
    %316 = ptr_offset %0, %315[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %315:index/borrow#1][0m
    %317 = primitive_cast %316 from Ptr of Char to Ptr of Double[2m ; ops [%316:value/borrow#0][0m
    %318 = const.double 0.25
    %319 = store %317, %318[2m : ()[0m[2m ; ops [%317:value/borrow#0, %318:element/consume#1][0m
    %320 = const.int 408
    %321 = ptr_offset %0, %320[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %320:index/borrow#1][0m
    %322 = primitive_cast %321 from Ptr of Char to Ptr of Double[2m ; ops [%321:value/borrow#0][0m
    %323 = const.double 0.25
    %324 = store %322, %323[2m : ()[0m[2m ; ops [%322:value/borrow#0, %323:element/consume#1][0m
    %325 = const.int 400
    %326 = ptr_offset %0, %325[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %325:index/borrow#1][0m
    %327 = primitive_cast %326 from Ptr of Char to Ptr of Double[2m ; ops [%326:value/borrow#0][0m
    %328 = const.double 0.25
    %329 = store %327, %328[2m : ()[0m[2m ; ops [%327:value/borrow#0, %328:element/consume#1][0m
    %330 = const.int 392
    %331 = ptr_offset %0, %330[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %330:index/borrow#1][0m
    %332 = primitive_cast %331 from Ptr of Char to Ptr of Double[2m ; ops [%331:value/borrow#0][0m
    %333 = const.double 0.25
    %334 = store %332, %333[2m : ()[0m[2m ; ops [%332:value/borrow#0, %333:element/consume#1][0m
    %335 = const.int 384
    %336 = ptr_offset %0, %335[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %335:index/borrow#1][0m
    %337 = primitive_cast %336 from Ptr of Char to Ptr of Double[2m ; ops [%336:value/borrow#0][0m
    %338 = const.double 0.25
    %339 = store %337, %338[2m : ()[0m[2m ; ops [%337:value/borrow#0, %338:element/consume#1][0m
    %340 = const.int 376
    %341 = ptr_offset %0, %340[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %340:index/borrow#1][0m
    %342 = primitive_cast %341 from Ptr of Char to Ptr of Double[2m ; ops [%341:value/borrow#0][0m
    %343 = const.double 0.25
    %344 = store %342, %343[2m : ()[0m[2m ; ops [%342:value/borrow#0, %343:element/consume#1][0m
    %345 = const.int 368
    %346 = ptr_offset %0, %345[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %345:index/borrow#1][0m
    %347 = primitive_cast %346 from Ptr of Char to Ptr of Double[2m ; ops [%346:value/borrow#0][0m
    %348 = const.double 0.25
    %349 = store %347, %348[2m : ()[0m[2m ; ops [%347:value/borrow#0, %348:element/consume#1][0m
    %350 = const.int 360
    %351 = ptr_offset %0, %350[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %350:index/borrow#1][0m
    %352 = primitive_cast %351 from Ptr of Char to Ptr of Double[2m ; ops [%351:value/borrow#0][0m
    %353 = const.double -0.75
    %354 = store %352, %353[2m : ()[0m[2m ; ops [%352:value/borrow#0, %353:element/consume#1][0m
    %355 = const.int 352
    %356 = ptr_offset %0, %355[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %355:index/borrow#1][0m
    %357 = primitive_cast %356 from Ptr of Char to Ptr of Double[2m ; ops [%356:value/borrow#0][0m
    %358 = const.double 0.25
    %359 = store %357, %358[2m : ()[0m[2m ; ops [%357:value/borrow#0, %358:element/consume#1][0m
    %360 = const.int 344
    %361 = ptr_offset %0, %360[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %360:index/borrow#1][0m
    %362 = primitive_cast %361 from Ptr of Char to Ptr of Double[2m ; ops [%361:value/borrow#0][0m
    %363 = const.double 0.25
    %364 = store %362, %363[2m : ()[0m[2m ; ops [%362:value/borrow#0, %363:element/consume#1][0m
    %365 = const.int 336
    %366 = ptr_offset %0, %365[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %365:index/borrow#1][0m
    %367 = primitive_cast %366 from Ptr of Char to Ptr of Double[2m ; ops [%366:value/borrow#0][0m
    %368 = const.double 0.25
    %369 = store %367, %368[2m : ()[0m[2m ; ops [%367:value/borrow#0, %368:element/consume#1][0m
    %370 = const.int 328
    %371 = ptr_offset %0, %370[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %370:index/borrow#1][0m
    %372 = primitive_cast %371 from Ptr of Char to Ptr of Double[2m ; ops [%371:value/borrow#0][0m
    %373 = const.double 0.25
    %374 = store %372, %373[2m : ()[0m[2m ; ops [%372:value/borrow#0, %373:element/consume#1][0m
    %375 = const.int 320
    %376 = ptr_offset %0, %375[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %375:index/borrow#1][0m
    %377 = primitive_cast %376 from Ptr of Char to Ptr of Double[2m ; ops [%376:value/borrow#0][0m
    %378 = const.double 0.25
    %379 = store %377, %378[2m : ()[0m[2m ; ops [%377:value/borrow#0, %378:element/consume#1][0m
    %380 = const.int 312
    %381 = ptr_offset %0, %380[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %380:index/borrow#1][0m
    %382 = primitive_cast %381 from Ptr of Char to Ptr of Double[2m ; ops [%381:value/borrow#0][0m
    %383 = const.double 0.25
    %384 = store %382, %383[2m : ()[0m[2m ; ops [%382:value/borrow#0, %383:element/consume#1][0m
    %385 = const.int 304
    %386 = ptr_offset %0, %385[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %385:index/borrow#1][0m
    %387 = primitive_cast %386 from Ptr of Char to Ptr of Double[2m ; ops [%386:value/borrow#0][0m
    %388 = const.double 0.25
    %389 = store %387, %388[2m : ()[0m[2m ; ops [%387:value/borrow#0, %388:element/consume#1][0m
    %390 = const.int 296
    %391 = ptr_offset %0, %390[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %390:index/borrow#1][0m
    %392 = primitive_cast %391 from Ptr of Char to Ptr of Double[2m ; ops [%391:value/borrow#0][0m
    %393 = const.double 0.25
    %394 = store %392, %393[2m : ()[0m[2m ; ops [%392:value/borrow#0, %393:element/consume#1][0m
    %395 = const.int 288
    %396 = ptr_offset %0, %395[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %395:index/borrow#1][0m
    %397 = primitive_cast %396 from Ptr of Char to Ptr of Double[2m ; ops [%396:value/borrow#0][0m
    %398 = const.double -0.75
    %399 = store %397, %398[2m : ()[0m[2m ; ops [%397:value/borrow#0, %398:element/consume#1][0m
    %400 = const.int 280
    %401 = ptr_offset %0, %400[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %400:index/borrow#1][0m
    %402 = primitive_cast %401 from Ptr of Char to Ptr of Double[2m ; ops [%401:value/borrow#0][0m
    %403 = const.double 0.25
    %404 = store %402, %403[2m : ()[0m[2m ; ops [%402:value/borrow#0, %403:element/consume#1][0m
    %405 = const.int 272
    %406 = ptr_offset %0, %405[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %405:index/borrow#1][0m
    %407 = primitive_cast %406 from Ptr of Char to Ptr of Double[2m ; ops [%406:value/borrow#0][0m
    %408 = const.double 0.25
    %409 = store %407, %408[2m : ()[0m[2m ; ops [%407:value/borrow#0, %408:element/consume#1][0m
    %410 = const.int 264
    %411 = ptr_offset %0, %410[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %410:index/borrow#1][0m
    %412 = primitive_cast %411 from Ptr of Char to Ptr of Double[2m ; ops [%411:value/borrow#0][0m
    %413 = const.double 0.25
    %414 = store %412, %413[2m : ()[0m[2m ; ops [%412:value/borrow#0, %413:element/consume#1][0m
    %415 = const.int 256
    %416 = ptr_offset %0, %415[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %415:index/borrow#1][0m
    %417 = primitive_cast %416 from Ptr of Char to Ptr of Double[2m ; ops [%416:value/borrow#0][0m
    %418 = const.double 0.25
    %419 = store %417, %418[2m : ()[0m[2m ; ops [%417:value/borrow#0, %418:element/consume#1][0m
    %420 = const.int 248
    %421 = ptr_offset %0, %420[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %420:index/borrow#1][0m
    %422 = primitive_cast %421 from Ptr of Char to Ptr of Double[2m ; ops [%421:value/borrow#0][0m
    %423 = const.double 0.25
    %424 = store %422, %423[2m : ()[0m[2m ; ops [%422:value/borrow#0, %423:element/consume#1][0m
    %425 = const.int 240
    %426 = ptr_offset %0, %425[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %425:index/borrow#1][0m
    %427 = primitive_cast %426 from Ptr of Char to Ptr of Double[2m ; ops [%426:value/borrow#0][0m
    %428 = const.double 0.25
    %429 = store %427, %428[2m : ()[0m[2m ; ops [%427:value/borrow#0, %428:element/consume#1][0m
    %430 = const.int 232
    %431 = ptr_offset %0, %430[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %430:index/borrow#1][0m
    %432 = primitive_cast %431 from Ptr of Char to Ptr of Double[2m ; ops [%431:value/borrow#0][0m
    %433 = const.double 0.25
    %434 = store %432, %433[2m : ()[0m[2m ; ops [%432:value/borrow#0, %433:element/consume#1][0m
    %435 = const.int 224
    %436 = ptr_offset %0, %435[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %435:index/borrow#1][0m
    %437 = primitive_cast %436 from Ptr of Char to Ptr of Double[2m ; ops [%436:value/borrow#0][0m
    %438 = const.double 0.25
    %439 = store %437, %438[2m : ()[0m[2m ; ops [%437:value/borrow#0, %438:element/consume#1][0m
    %440 = const.int 216
    %441 = ptr_offset %0, %440[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %440:index/borrow#1][0m
    %442 = primitive_cast %441 from Ptr of Char to Ptr of Double[2m ; ops [%441:value/borrow#0][0m
    %443 = const.double -0.75
    %444 = store %442, %443[2m : ()[0m[2m ; ops [%442:value/borrow#0, %443:element/consume#1][0m
    %445 = const.int 208
    %446 = ptr_offset %0, %445[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %445:index/borrow#1][0m
    %447 = primitive_cast %446 from Ptr of Char to Ptr of Double[2m ; ops [%446:value/borrow#0][0m
    %448 = const.double 0.25
    %449 = store %447, %448[2m : ()[0m[2m ; ops [%447:value/borrow#0, %448:element/consume#1][0m
    %450 = const.int 200
    %451 = ptr_offset %0, %450[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %450:index/borrow#1][0m
    %452 = primitive_cast %451 from Ptr of Char to Ptr of Double[2m ; ops [%451:value/borrow#0][0m
    %453 = const.double 0.25
    %454 = store %452, %453[2m : ()[0m[2m ; ops [%452:value/borrow#0, %453:element/consume#1][0m
    %455 = const.int 192
    %456 = ptr_offset %0, %455[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %455:index/borrow#1][0m
    %457 = primitive_cast %456 from Ptr of Char to Ptr of Double[2m ; ops [%456:value/borrow#0][0m
    %458 = const.double 0.25
    %459 = store %457, %458[2m : ()[0m[2m ; ops [%457:value/borrow#0, %458:element/consume#1][0m
    %460 = const.int 184
    %461 = ptr_offset %0, %460[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %460:index/borrow#1][0m
    %462 = primitive_cast %461 from Ptr of Char to Ptr of Double[2m ; ops [%461:value/borrow#0][0m
    %463 = const.double 0.25
    %464 = store %462, %463[2m : ()[0m[2m ; ops [%462:value/borrow#0, %463:element/consume#1][0m
    %465 = const.int 176
    %466 = ptr_offset %0, %465[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %465:index/borrow#1][0m
    %467 = primitive_cast %466 from Ptr of Char to Ptr of Double[2m ; ops [%466:value/borrow#0][0m
    %468 = const.double 0.25
    %469 = store %467, %468[2m : ()[0m[2m ; ops [%467:value/borrow#0, %468:element/consume#1][0m
    %470 = const.int 168
    %471 = ptr_offset %0, %470[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %470:index/borrow#1][0m
    %472 = primitive_cast %471 from Ptr of Char to Ptr of Double[2m ; ops [%471:value/borrow#0][0m
    %473 = const.double 0.25
    %474 = store %472, %473[2m : ()[0m[2m ; ops [%472:value/borrow#0, %473:element/consume#1][0m
    %475 = const.int 160
    %476 = ptr_offset %0, %475[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %475:index/borrow#1][0m
    %477 = primitive_cast %476 from Ptr of Char to Ptr of Double[2m ; ops [%476:value/borrow#0][0m
    %478 = const.double 0.25
    %479 = store %477, %478[2m : ()[0m[2m ; ops [%477:value/borrow#0, %478:element/consume#1][0m
    %480 = const.int 152
    %481 = ptr_offset %0, %480[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %480:index/borrow#1][0m
    %482 = primitive_cast %481 from Ptr of Char to Ptr of Double[2m ; ops [%481:value/borrow#0][0m
    %483 = const.double 0.25
    %484 = store %482, %483[2m : ()[0m[2m ; ops [%482:value/borrow#0, %483:element/consume#1][0m
    %485 = const.int 144
    %486 = ptr_offset %0, %485[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %485:index/borrow#1][0m
    %487 = primitive_cast %486 from Ptr of Char to Ptr of Double[2m ; ops [%486:value/borrow#0][0m
    %488 = const.double -0.75
    %489 = store %487, %488[2m : ()[0m[2m ; ops [%487:value/borrow#0, %488:element/consume#1][0m
    %490 = const.int 136
    %491 = ptr_offset %0, %490[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %490:index/borrow#1][0m
    %492 = primitive_cast %491 from Ptr of Char to Ptr of Double[2m ; ops [%491:value/borrow#0][0m
    %493 = const.double 0.25
    %494 = store %492, %493[2m : ()[0m[2m ; ops [%492:value/borrow#0, %493:element/consume#1][0m
    %495 = const.int 128
    %496 = ptr_offset %0, %495[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %495:index/borrow#1][0m
    %497 = primitive_cast %496 from Ptr of Char to Ptr of Double[2m ; ops [%496:value/borrow#0][0m
    %498 = const.double 0.25
    %499 = store %497, %498[2m : ()[0m[2m ; ops [%497:value/borrow#0, %498:element/consume#1][0m
    %500 = const.int 120
    %501 = ptr_offset %0, %500[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %500:index/borrow#1][0m
    %502 = primitive_cast %501 from Ptr of Char to Ptr of Double[2m ; ops [%501:value/borrow#0][0m
    %503 = const.double 0.25
    %504 = store %502, %503[2m : ()[0m[2m ; ops [%502:value/borrow#0, %503:element/consume#1][0m
    %505 = const.int 112
    %506 = ptr_offset %0, %505[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %505:index/borrow#1][0m
    %507 = primitive_cast %506 from Ptr of Char to Ptr of Double[2m ; ops [%506:value/borrow#0][0m
    %508 = const.double 0.25
    %509 = store %507, %508[2m : ()[0m[2m ; ops [%507:value/borrow#0, %508:element/consume#1][0m
    %510 = const.int 104
    %511 = ptr_offset %0, %510[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %510:index/borrow#1][0m
    %512 = primitive_cast %511 from Ptr of Char to Ptr of Double[2m ; ops [%511:value/borrow#0][0m
    %513 = const.double 0.25
    %514 = store %512, %513[2m : ()[0m[2m ; ops [%512:value/borrow#0, %513:element/consume#1][0m
    %515 = const.int 96
    %516 = ptr_offset %0, %515[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %515:index/borrow#1][0m
    %517 = primitive_cast %516 from Ptr of Char to Ptr of Double[2m ; ops [%516:value/borrow#0][0m
    %518 = const.double 0.25
    %519 = store %517, %518[2m : ()[0m[2m ; ops [%517:value/borrow#0, %518:element/consume#1][0m
    %520 = const.int 88
    %521 = ptr_offset %0, %520[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %520:index/borrow#1][0m
    %522 = primitive_cast %521 from Ptr of Char to Ptr of Double[2m ; ops [%521:value/borrow#0][0m
    %523 = const.double 0.25
    %524 = store %522, %523[2m : ()[0m[2m ; ops [%522:value/borrow#0, %523:element/consume#1][0m
    %525 = const.int 80
    %526 = ptr_offset %0, %525[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %525:index/borrow#1][0m
    %527 = primitive_cast %526 from Ptr of Char to Ptr of Double[2m ; ops [%526:value/borrow#0][0m
    %528 = const.double 0.25
    %529 = store %527, %528[2m : ()[0m[2m ; ops [%527:value/borrow#0, %528:element/consume#1][0m
    %530 = const.int 72
    %531 = ptr_offset %0, %530[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %530:index/borrow#1][0m
    %532 = primitive_cast %531 from Ptr of Char to Ptr of Double[2m ; ops [%531:value/borrow#0][0m
    %533 = const.double -0.75
    %534 = store %532, %533[2m : ()[0m[2m ; ops [%532:value/borrow#0, %533:element/consume#1][0m
    %535 = const.int 64
    %536 = ptr_offset %0, %535[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %535:index/borrow#1][0m
    %537 = primitive_cast %536 from Ptr of Char to Ptr of Double[2m ; ops [%536:value/borrow#0][0m
    %538 = const.double 0.25
    %539 = store %537, %538[2m : ()[0m[2m ; ops [%537:value/borrow#0, %538:element/consume#1][0m
    %540 = const.int 56
    %541 = ptr_offset %0, %540[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %540:index/borrow#1][0m
    %542 = primitive_cast %541 from Ptr of Char to Ptr of Double[2m ; ops [%541:value/borrow#0][0m
    %543 = const.double 0.25
    %544 = store %542, %543[2m : ()[0m[2m ; ops [%542:value/borrow#0, %543:element/consume#1][0m
    %545 = const.int 48
    %546 = ptr_offset %0, %545[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %545:index/borrow#1][0m
    %547 = primitive_cast %546 from Ptr of Char to Ptr of Double[2m ; ops [%546:value/borrow#0][0m
    %548 = const.double 0.25
    %549 = store %547, %548[2m : ()[0m[2m ; ops [%547:value/borrow#0, %548:element/consume#1][0m
    %550 = const.int 40
    %551 = ptr_offset %0, %550[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %550:index/borrow#1][0m
    %552 = primitive_cast %551 from Ptr of Char to Ptr of Double[2m ; ops [%551:value/borrow#0][0m
    %553 = const.double 0.25
    %554 = store %552, %553[2m : ()[0m[2m ; ops [%552:value/borrow#0, %553:element/consume#1][0m
    %555 = const.int 32
    %556 = ptr_offset %0, %555[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %555:index/borrow#1][0m
    %557 = primitive_cast %556 from Ptr of Char to Ptr of Double[2m ; ops [%556:value/borrow#0][0m
    %558 = const.double 0.25
    %559 = store %557, %558[2m : ()[0m[2m ; ops [%557:value/borrow#0, %558:element/consume#1][0m
    %560 = const.int 24
    %561 = ptr_offset %0, %560[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %560:index/borrow#1][0m
    %562 = primitive_cast %561 from Ptr of Char to Ptr of Double[2m ; ops [%561:value/borrow#0][0m
    %563 = const.double 0.25
    %564 = store %562, %563[2m : ()[0m[2m ; ops [%562:value/borrow#0, %563:element/consume#1][0m
    %565 = const.int 16
    %566 = ptr_offset %0, %565[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %565:index/borrow#1][0m
    %567 = primitive_cast %566 from Ptr of Char to Ptr of Double[2m ; ops [%566:value/borrow#0][0m
    %568 = const.double 0.25
    %569 = store %567, %568[2m : ()[0m[2m ; ops [%567:value/borrow#0, %568:element/consume#1][0m
    %570 = const.int 8
    %571 = ptr_offset %0, %570[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %570:index/borrow#1][0m
    %572 = primitive_cast %571 from Ptr of Char to Ptr of Double[2m ; ops [%571:value/borrow#0][0m
    %573 = const.double 0.25
    %574 = store %572, %573[2m : ()[0m[2m ; ops [%572:value/borrow#0, %573:element/consume#1][0m
    %575 = const.int 0
    %576 = ptr_offset %0, %575[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %575:index/borrow#1][0m
    %577 = primitive_cast %576 from Ptr of Char to Ptr of Double[2m ; ops [%576:value/borrow#0][0m
    %578 = const.double -0.75
    %579 = store %577, %578[2m : ()[0m[2m ; ops [%577:value/borrow#0, %578:element/consume#1][0m
    %580 = const.int 3072840
    %581 = ptr_offset %0, %580[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %580:index/borrow#1][0m
    %582 = fn_ref $fdn8.array_map.callback.6.init
    %583 = call %582(%581)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %581:value/consume#0][0m
    %584 = const.int 3072848
    %585 = ptr_offset %0, %584[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %584:index/borrow#1][0m
    %586 = call %582(%585)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %585:value/consume#0][0m
    %587 = const.int 3072856
    %588 = ptr_offset %0, %587[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %587:index/borrow#1][0m
    %589 = call %582(%588)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %588:value/consume#0][0m
    %590 = const.int 3072864
    %591 = ptr_offset %0, %590[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %590:index/borrow#1][0m
    %592 = call %582(%591)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %591:value/consume#0][0m
    %593 = const.int 3072872
    %594 = ptr_offset %0, %593[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %593:index/borrow#1][0m
    %595 = call %582(%594)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %594:value/consume#0][0m
    %596 = const.int 3072880
    %597 = ptr_offset %0, %596[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %596:index/borrow#1][0m
    %598 = call %582(%597)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %597:value/consume#0][0m
    %599 = const.int 3072888
    %600 = ptr_offset %0, %599[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %599:index/borrow#1][0m
    %601 = call %582(%600)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %600:value/consume#0][0m
    %602 = const.int 3072896
    %603 = ptr_offset %0, %602[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %602:index/borrow#1][0m
    %604 = call %582(%603)[2m : ()[0m[2m ; ops [%582:callee/borrow#0, %603:value/consume#0][0m
    %605 = const.int 3072968
    %606 = ptr_offset %0, %605[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %605:index/borrow#1][0m
    %607 = fn_ref $fdn8.array_mapi.callback.7.init
    %608 = call %607(%606)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %606:value/borrow#0][0m
    %609 = const.int 3073000
    %610 = ptr_offset %0, %609[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %609:index/borrow#1][0m
    %611 = call %607(%610)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %610:value/borrow#0][0m
    %612 = const.int 3073032
    %613 = ptr_offset %0, %612[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %612:index/borrow#1][0m
    %614 = call %607(%613)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %613:value/borrow#0][0m
    %615 = const.int 3073064
    %616 = ptr_offset %0, %615[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %615:index/borrow#1][0m
    %617 = call %607(%616)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %616:value/borrow#0][0m
    %618 = const.int 3073096
    %619 = ptr_offset %0, %618[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %618:index/borrow#1][0m
    %620 = call %607(%619)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %619:value/borrow#0][0m
    %621 = const.int 3073128
    %622 = ptr_offset %0, %621[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %621:index/borrow#1][0m
    %623 = call %607(%622)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %622:value/borrow#0][0m
    %624 = const.int 3073160
    %625 = ptr_offset %0, %624[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %624:index/borrow#1][0m
    %626 = call %607(%625)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %625:value/borrow#0][0m
    %627 = const.int 3073192
    %628 = ptr_offset %0, %627[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %627:index/borrow#1][0m
    %629 = call %607(%628)[2m : ()[0m[2m ; ops [%607:callee/borrow#0, %628:value/borrow#0][0m
    %630 = const.void
    return %630[2m ; ops [%630:return/consume#0][0m
}
fn fdn8.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 d1[2m: Double @consume[0m, %5 d2[2m: Double @consume[0m, %6 d3[2m: Double @consume[0m, %7 d4[2m: Double @consume[0m, %8 d5[2m: Double @consume[0m, %9 d6[2m: Double @consume[0m, %10 d7[2m: Double @consume[0m, %11 d8[2m: Double @consume[0m, %12 rt60[2m: Double @borrow[0m, %13 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %14 = const.int 0
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = const.int 64
    %18 = const.int 0
    %19 = construct.tuple { %17, %18, %16 }[2m : Array of Double[0m[2m ; ops [%17:field/borrow#0, %18:field/borrow#1, %16:field/borrow#2][0m
    %20 = const.int 512
    %21 = ptr_offset %1, %20[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %20:index/borrow#1][0m
    %22 = primitive_cast %21 from Ptr of Char to Ptr of Double[2m ; ops [%21:value/borrow#0][0m
    %23 = const.int 8
    %24 = const.int 0
    %25 = construct.tuple { %23, %24, %22 }[2m : Array of Double[0m[2m ; ops [%23:field/borrow#0, %24:field/borrow#1, %22:field/borrow#2][0m
    %26 = const.int 576
    %27 = ptr_offset %1, %26[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = const.int 8
    %30 = const.int 0
    %31 = construct.tuple { %29, %30, %28 }[2m : Array of Double[0m[2m ; ops [%29:field/borrow#0, %30:field/borrow#1, %28:field/borrow#2][0m
    %32 = const.int 1
    %33 = const.int 832
    %34 = ptr_offset %1, %33[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %33:index/borrow#1][0m
    %35 = primitive_cast %34 from Ptr of Char to Ptr of Double[2m ; ops [%34:value/borrow#0][0m
    %36 = const.int 48000
    %37 = const.int 0
    %38 = construct.tuple { %36, %37, %35 }[2m : Array of Double[0m[2m ; ops [%36:field/borrow#0, %37:field/borrow#1, %35:field/borrow#2][0m
[35m    %351 = dup %38[2m : ()[0m[2m ; ops [%38:value/borrow#0][0m[0m
    %39 = construct.tuple { %38, %4 }[2m : (Array of Double, Double, )[0m[2m ; ops [%38:field/consume#0, %4:field/consume#1][0m
    %40 = const.int 640
    %41 = ptr_offset %1, %40[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %40:index/borrow#1][0m
    %42 = primitive_cast %41 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%41:value/borrow#0][0m
    %352 = load_owned %42[2m : (Array of Double, Double, )[0m[2m ; ops [%42:value/borrow#0][0m
    %43 = store %42, %39[2m : ()[0m[2m ; ops [%42:value/borrow#0, %39:element/consume#1][0m
[35m    %353 = drop %352[2m : ()[0m[2m ; ops [%352:value/borrow#0][0m[0m
    %44 = const.int 1
    %45 = const.int 384832
    %46 = ptr_offset %1, %45[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %45:index/borrow#1][0m
    %47 = primitive_cast %46 from Ptr of Char to Ptr of Double[2m ; ops [%46:value/borrow#0][0m
    %48 = const.int 48000
    %49 = const.int 0
    %50 = construct.tuple { %48, %49, %47 }[2m : Array of Double[0m[2m ; ops [%48:field/borrow#0, %49:field/borrow#1, %47:field/borrow#2][0m
[35m    %354 = dup %50[2m : ()[0m[2m ; ops [%50:value/borrow#0][0m[0m
    %51 = construct.tuple { %50, %5 }[2m : (Array of Double, Double, )[0m[2m ; ops [%50:field/consume#0, %5:field/consume#1][0m
    %52 = const.int 664
    %53 = ptr_offset %1, %52[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %52:index/borrow#1][0m
    %54 = primitive_cast %53 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%53:value/borrow#0][0m
    %355 = load_owned %54[2m : (Array of Double, Double, )[0m[2m ; ops [%54:value/borrow#0][0m
    %55 = store %54, %51[2m : ()[0m[2m ; ops [%54:value/borrow#0, %51:element/consume#1][0m
[35m    %356 = drop %355[2m : ()[0m[2m ; ops [%355:value/borrow#0][0m[0m
    %56 = const.int 1
    %57 = const.int 768832
    %58 = ptr_offset %1, %57[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %57:index/borrow#1][0m
    %59 = primitive_cast %58 from Ptr of Char to Ptr of Double[2m ; ops [%58:value/borrow#0][0m
    %60 = const.int 48000
    %61 = const.int 0
    %62 = construct.tuple { %60, %61, %59 }[2m : Array of Double[0m[2m ; ops [%60:field/borrow#0, %61:field/borrow#1, %59:field/borrow#2][0m
[35m    %357 = dup %62[2m : ()[0m[2m ; ops [%62:value/borrow#0][0m[0m
    %63 = construct.tuple { %62, %6 }[2m : (Array of Double, Double, )[0m[2m ; ops [%62:field/consume#0, %6:field/consume#1][0m
    %64 = const.int 688
    %65 = ptr_offset %1, %64[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %64:index/borrow#1][0m
    %66 = primitive_cast %65 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%65:value/borrow#0][0m
    %358 = load_owned %66[2m : (Array of Double, Double, )[0m[2m ; ops [%66:value/borrow#0][0m
    %67 = store %66, %63[2m : ()[0m[2m ; ops [%66:value/borrow#0, %63:element/consume#1][0m
[35m    %359 = drop %358[2m : ()[0m[2m ; ops [%358:value/borrow#0][0m[0m
    %68 = const.int 1
    %69 = const.int 1152832
    %70 = ptr_offset %1, %69[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %69:index/borrow#1][0m
    %71 = primitive_cast %70 from Ptr of Char to Ptr of Double[2m ; ops [%70:value/borrow#0][0m
    %72 = const.int 48000
    %73 = const.int 0
    %74 = construct.tuple { %72, %73, %71 }[2m : Array of Double[0m[2m ; ops [%72:field/borrow#0, %73:field/borrow#1, %71:field/borrow#2][0m
[35m    %360 = dup %74[2m : ()[0m[2m ; ops [%74:value/borrow#0][0m[0m
    %75 = construct.tuple { %74, %7 }[2m : (Array of Double, Double, )[0m[2m ; ops [%74:field/consume#0, %7:field/consume#1][0m
    %76 = const.int 712
    %77 = ptr_offset %1, %76[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %76:index/borrow#1][0m
    %78 = primitive_cast %77 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%77:value/borrow#0][0m
    %361 = load_owned %78[2m : (Array of Double, Double, )[0m[2m ; ops [%78:value/borrow#0][0m
    %79 = store %78, %75[2m : ()[0m[2m ; ops [%78:value/borrow#0, %75:element/consume#1][0m
[35m    %362 = drop %361[2m : ()[0m[2m ; ops [%361:value/borrow#0][0m[0m
    %80 = const.int 1
    %81 = const.int 1536832
    %82 = ptr_offset %1, %81[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %81:index/borrow#1][0m
    %83 = primitive_cast %82 from Ptr of Char to Ptr of Double[2m ; ops [%82:value/borrow#0][0m
    %84 = const.int 48000
    %85 = const.int 0
    %86 = construct.tuple { %84, %85, %83 }[2m : Array of Double[0m[2m ; ops [%84:field/borrow#0, %85:field/borrow#1, %83:field/borrow#2][0m
[35m    %363 = dup %86[2m : ()[0m[2m ; ops [%86:value/borrow#0][0m[0m
    %87 = construct.tuple { %86, %8 }[2m : (Array of Double, Double, )[0m[2m ; ops [%86:field/consume#0, %8:field/consume#1][0m
    %88 = const.int 736
    %89 = ptr_offset %1, %88[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %88:index/borrow#1][0m
    %90 = primitive_cast %89 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%89:value/borrow#0][0m
    %364 = load_owned %90[2m : (Array of Double, Double, )[0m[2m ; ops [%90:value/borrow#0][0m
    %91 = store %90, %87[2m : ()[0m[2m ; ops [%90:value/borrow#0, %87:element/consume#1][0m
[35m    %365 = drop %364[2m : ()[0m[2m ; ops [%364:value/borrow#0][0m[0m
    %92 = const.int 1
    %93 = const.int 1920832
    %94 = ptr_offset %1, %93[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %93:index/borrow#1][0m
    %95 = primitive_cast %94 from Ptr of Char to Ptr of Double[2m ; ops [%94:value/borrow#0][0m
    %96 = const.int 48000
    %97 = const.int 0
    %98 = construct.tuple { %96, %97, %95 }[2m : Array of Double[0m[2m ; ops [%96:field/borrow#0, %97:field/borrow#1, %95:field/borrow#2][0m
[35m    %366 = dup %98[2m : ()[0m[2m ; ops [%98:value/borrow#0][0m[0m
    %99 = construct.tuple { %98, %9 }[2m : (Array of Double, Double, )[0m[2m ; ops [%98:field/consume#0, %9:field/consume#1][0m
    %100 = const.int 760
    %101 = ptr_offset %1, %100[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %100:index/borrow#1][0m
    %102 = primitive_cast %101 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%101:value/borrow#0][0m
    %367 = load_owned %102[2m : (Array of Double, Double, )[0m[2m ; ops [%102:value/borrow#0][0m
    %103 = store %102, %99[2m : ()[0m[2m ; ops [%102:value/borrow#0, %99:element/consume#1][0m
[35m    %368 = drop %367[2m : ()[0m[2m ; ops [%367:value/borrow#0][0m[0m
    %104 = const.int 1
    %105 = const.int 2304832
    %106 = ptr_offset %1, %105[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %105:index/borrow#1][0m
    %107 = primitive_cast %106 from Ptr of Char to Ptr of Double[2m ; ops [%106:value/borrow#0][0m
    %108 = const.int 48000
    %109 = const.int 0
    %110 = construct.tuple { %108, %109, %107 }[2m : Array of Double[0m[2m ; ops [%108:field/borrow#0, %109:field/borrow#1, %107:field/borrow#2][0m
[35m    %369 = dup %110[2m : ()[0m[2m ; ops [%110:value/borrow#0][0m[0m
    %111 = construct.tuple { %110, %10 }[2m : (Array of Double, Double, )[0m[2m ; ops [%110:field/consume#0, %10:field/consume#1][0m
    %112 = const.int 784
    %113 = ptr_offset %1, %112[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %112:index/borrow#1][0m
    %114 = primitive_cast %113 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%113:value/borrow#0][0m
    %370 = load_owned %114[2m : (Array of Double, Double, )[0m[2m ; ops [%114:value/borrow#0][0m
    %115 = store %114, %111[2m : ()[0m[2m ; ops [%114:value/borrow#0, %111:element/consume#1][0m
[35m    %371 = drop %370[2m : ()[0m[2m ; ops [%370:value/borrow#0][0m[0m
    %116 = const.int 1
    %117 = const.int 2688832
    %118 = ptr_offset %1, %117[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %117:index/borrow#1][0m
    %119 = primitive_cast %118 from Ptr of Char to Ptr of Double[2m ; ops [%118:value/borrow#0][0m
    %120 = const.int 48000
    %121 = const.int 0
    %122 = construct.tuple { %120, %121, %119 }[2m : Array of Double[0m[2m ; ops [%120:field/borrow#0, %121:field/borrow#1, %119:field/borrow#2][0m
[35m    %372 = dup %122[2m : ()[0m[2m ; ops [%122:value/borrow#0][0m[0m
    %123 = construct.tuple { %122, %11 }[2m : (Array of Double, Double, )[0m[2m ; ops [%122:field/consume#0, %11:field/consume#1][0m
    %124 = const.int 808
    %125 = ptr_offset %1, %124[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %124:index/borrow#1][0m
    %126 = primitive_cast %125 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%125:value/borrow#0][0m
    %373 = load_owned %126[2m : (Array of Double, Double, )[0m[2m ; ops [%126:value/borrow#0][0m
    %127 = store %126, %123[2m : ()[0m[2m ; ops [%126:value/borrow#0, %123:element/consume#1][0m
[35m    %374 = drop %373[2m : ()[0m[2m ; ops [%373:value/borrow#0][0m[0m
    %128 = const.int 640
    %129 = ptr_offset %1, %128[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %128:index/borrow#1][0m
    %130 = primitive_cast %129 from Ptr of Char to Ptr of (Array of Double, Double, )[2m ; ops [%129:value/borrow#0][0m
    %131 = const.int 8
    %132 = const.int 0
    %133 = construct.tuple { %131, %132, %130 }[2m : Array of (Array of Double, Double, )[0m[2m ; ops [%131:field/borrow#0, %132:field/borrow#1, %130:field/borrow#2][0m
    %134 = const.int 0
    %135 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %136 = ptr_offset %135, %134[2m : Ptr[0m[2m ; ops [%135:value/borrow#0, %134:index/borrow#1][0m
    %137 = load %136[2m : (Array of Double, Double, )[0m[2m ; ops [%136:value/borrow#0][0m
    %138 = extract.field %137, 0[2m : Array of Double[0m[2m ; ops [%137:container/borrow#0][0m
    %139 = extract.field %137, 1[2m : Double[0m[2m ; ops [%137:container/borrow#0][0m
    %140 = extract.field %138, 0[2m : Int[0m[2m ; ops [%138:container/borrow#0][0m
    %141 = const.int 3072832
    %142 = ptr_offset %1, %141[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %141:index/borrow#1][0m
    %143 = primitive_cast %142 from Ptr of Char to Ptr of Int[2m ; ops [%142:value/borrow#0][0m
    %144 = const.int 1
    %145 = const.int 0
    %146 = construct.tuple { %144, %145, %143 }[2m : Array of Int[0m[2m ; ops [%144:field/borrow#0, %145:field/borrow#1, %143:field/borrow#2][0m
    %147 = const.int 0
    %148 = extract.field %146, 1[2m : Int[0m[2m ; ops [%146:container/borrow#0][0m
    %149 = iadd %148, %147[2m ; ops [%148:value/borrow#0, %147:value/borrow#1][0m
    %150 = extract.field %146, 2[2m : Ptr[0m[2m ; ops [%146:container/borrow#0][0m
    %151 = ptr_offset %150, %149[2m : Ptr[0m[2m ; ops [%150:value/borrow#0, %149:index/borrow#1][0m
    %152 = load %151[2m : Int[0m[2m ; ops [%151:value/borrow#0][0m
    %153 = const.int 0
    %154 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %155 = iadd %154, %153[2m ; ops [%154:value/borrow#0, %153:value/borrow#1][0m
    %156 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %157 = ptr_offset %156, %155[2m : Ptr[0m[2m ; ops [%156:value/borrow#0, %155:index/borrow#1][0m
    %158 = load %157[2m : (Array of Double, Double, )[0m[2m ; ops [%157:value/borrow#0][0m
    %159 = fn_ref $fdn8.array_map.callback.6.kernel
    %160 = const.int 3072840
    %161 = ptr_offset %1, %160[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %160:index/borrow#1][0m
    %162 = primitive_cast %161 from Ptr of Char to Ptr of Char[2m ; ops [%161:value/borrow#0][0m
    %163 = call %159(%0, %162, %2, %3, %152, %158)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %162:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %158:value/borrow#5][0m
    %164 = const.int 1
    %165 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %166 = iadd %165, %164[2m ; ops [%165:value/borrow#0, %164:value/borrow#1][0m
    %167 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %168 = ptr_offset %167, %166[2m : Ptr[0m[2m ; ops [%167:value/borrow#0, %166:index/borrow#1][0m
    %169 = load %168[2m : (Array of Double, Double, )[0m[2m ; ops [%168:value/borrow#0][0m
    %170 = const.int 3072848
    %171 = ptr_offset %1, %170[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %170:index/borrow#1][0m
    %172 = primitive_cast %171 from Ptr of Char to Ptr of Char[2m ; ops [%171:value/borrow#0][0m
    %173 = call %159(%0, %172, %2, %3, %152, %169)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %172:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %169:value/borrow#5][0m
    %174 = const.int 2
    %175 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %176 = iadd %175, %174[2m ; ops [%175:value/borrow#0, %174:value/borrow#1][0m
    %177 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %178 = ptr_offset %177, %176[2m : Ptr[0m[2m ; ops [%177:value/borrow#0, %176:index/borrow#1][0m
    %179 = load %178[2m : (Array of Double, Double, )[0m[2m ; ops [%178:value/borrow#0][0m
    %180 = const.int 3072856
    %181 = ptr_offset %1, %180[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %180:index/borrow#1][0m
    %182 = primitive_cast %181 from Ptr of Char to Ptr of Char[2m ; ops [%181:value/borrow#0][0m
    %183 = call %159(%0, %182, %2, %3, %152, %179)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %182:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %179:value/borrow#5][0m
    %184 = const.int 3
    %185 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %186 = iadd %185, %184[2m ; ops [%185:value/borrow#0, %184:value/borrow#1][0m
    %187 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %188 = ptr_offset %187, %186[2m : Ptr[0m[2m ; ops [%187:value/borrow#0, %186:index/borrow#1][0m
    %189 = load %188[2m : (Array of Double, Double, )[0m[2m ; ops [%188:value/borrow#0][0m
    %190 = const.int 3072864
    %191 = ptr_offset %1, %190[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %190:index/borrow#1][0m
    %192 = primitive_cast %191 from Ptr of Char to Ptr of Char[2m ; ops [%191:value/borrow#0][0m
    %193 = call %159(%0, %192, %2, %3, %152, %189)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %192:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %189:value/borrow#5][0m
    %194 = const.int 4
    %195 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %196 = iadd %195, %194[2m ; ops [%195:value/borrow#0, %194:value/borrow#1][0m
    %197 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %198 = ptr_offset %197, %196[2m : Ptr[0m[2m ; ops [%197:value/borrow#0, %196:index/borrow#1][0m
    %199 = load %198[2m : (Array of Double, Double, )[0m[2m ; ops [%198:value/borrow#0][0m
    %200 = const.int 3072872
    %201 = ptr_offset %1, %200[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %200:index/borrow#1][0m
    %202 = primitive_cast %201 from Ptr of Char to Ptr of Char[2m ; ops [%201:value/borrow#0][0m
    %203 = call %159(%0, %202, %2, %3, %152, %199)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %202:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %199:value/borrow#5][0m
    %204 = const.int 5
    %205 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %206 = iadd %205, %204[2m ; ops [%205:value/borrow#0, %204:value/borrow#1][0m
    %207 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %208 = ptr_offset %207, %206[2m : Ptr[0m[2m ; ops [%207:value/borrow#0, %206:index/borrow#1][0m
    %209 = load %208[2m : (Array of Double, Double, )[0m[2m ; ops [%208:value/borrow#0][0m
    %210 = const.int 3072880
    %211 = ptr_offset %1, %210[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %210:index/borrow#1][0m
    %212 = primitive_cast %211 from Ptr of Char to Ptr of Char[2m ; ops [%211:value/borrow#0][0m
    %213 = call %159(%0, %212, %2, %3, %152, %209)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %212:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %209:value/borrow#5][0m
    %214 = const.int 6
    %215 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %216 = iadd %215, %214[2m ; ops [%215:value/borrow#0, %214:value/borrow#1][0m
    %217 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %218 = ptr_offset %217, %216[2m : Ptr[0m[2m ; ops [%217:value/borrow#0, %216:index/borrow#1][0m
    %219 = load %218[2m : (Array of Double, Double, )[0m[2m ; ops [%218:value/borrow#0][0m
    %220 = const.int 3072888
    %221 = ptr_offset %1, %220[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %220:index/borrow#1][0m
    %222 = primitive_cast %221 from Ptr of Char to Ptr of Char[2m ; ops [%221:value/borrow#0][0m
    %223 = call %159(%0, %222, %2, %3, %152, %219)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %222:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %219:value/borrow#5][0m
    %224 = const.int 7
    %225 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %226 = iadd %225, %224[2m ; ops [%225:value/borrow#0, %224:value/borrow#1][0m
    %227 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %228 = ptr_offset %227, %226[2m : Ptr[0m[2m ; ops [%227:value/borrow#0, %226:index/borrow#1][0m
    %229 = load %228[2m : (Array of Double, Double, )[0m[2m ; ops [%228:value/borrow#0][0m
    %230 = const.int 3072896
    %231 = ptr_offset %1, %230[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %230:index/borrow#1][0m
    %232 = primitive_cast %231 from Ptr of Char to Ptr of Char[2m ; ops [%231:value/borrow#0][0m
    %233 = call %159(%0, %232, %2, %3, %152, %229)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %0:value/consume#0, %232:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %152:value/borrow#4, %229:value/borrow#5][0m
    %234 = construct.array_literal { %163, %173, %183, %193, %203, %213, %223, %233 }[2m : Array of Double[0m[2m ; ea stack#0[0m[2m ; ops [%163:element/consume#0, %173:element/consume#1, %183:element/consume#2, %193:element/consume#3, %203:element/consume#4, %213:element/consume#5, %223:element/consume#6, %233:element/consume#7][0m
    %235 = fn_ref $matrix_vec_mul_double
    %236 = const.int 8
    %237 = const.int 8
    %238 = const.int 3072904
    %239 = ptr_offset %1, %238[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %238:index/borrow#1][0m
    %240 = primitive_cast %239 from Ptr of Char to Ptr of Double[2m ; ops [%239:value/borrow#0][0m
    %241 = const.int 8
    %242 = const.int 0
    %243 = construct.tuple { %241, %242, %240 }[2m : Array of Double[0m[2m ; ops [%241:field/borrow#0, %242:field/borrow#1, %240:field/borrow#2][0m
    %244 = call %235(%236, %237, %19, %234, %243)[2m : Array of Double[0m[2m ; ops [%235:callee/borrow#0, %236:value/borrow#0, %237:value/borrow#1, %19:value/borrow#2, %234:value/borrow#3, %243:value/borrow#4][0m
    %245 = const.int 0
    %246 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %247 = iadd %246, %245[2m ; ops [%246:value/borrow#0, %245:value/borrow#1][0m
    %248 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %249 = ptr_offset %248, %247[2m : Ptr[0m[2m ; ops [%248:value/borrow#0, %247:index/borrow#1][0m
    %250 = load %249[2m : (Array of Double, Double, )[0m[2m ; ops [%249:value/borrow#0][0m
    %251 = const.int 0
    %252 = fn_ref $fdn8.array_mapi.callback.7.kernel
    %253 = const.int 3072968
    %254 = ptr_offset %1, %253[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %253:index/borrow#1][0m
    %255 = primitive_cast %254 from Ptr of Char to Ptr of Char[2m ; ops [%254:value/borrow#0][0m
    %256 = call %252(%0, %255, %2, %3, %244, %12, %25, %13, %152, %251, %250)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %255:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %251:value/borrow#9, %250:value/borrow#10][0m
    %257 = const.int 1
    %258 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %259 = iadd %258, %257[2m ; ops [%258:value/borrow#0, %257:value/borrow#1][0m
    %260 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %261 = ptr_offset %260, %259[2m : Ptr[0m[2m ; ops [%260:value/borrow#0, %259:index/borrow#1][0m
    %262 = load %261[2m : (Array of Double, Double, )[0m[2m ; ops [%261:value/borrow#0][0m
    %263 = const.int 1
    %264 = const.int 3073000
    %265 = ptr_offset %1, %264[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %264:index/borrow#1][0m
    %266 = primitive_cast %265 from Ptr of Char to Ptr of Char[2m ; ops [%265:value/borrow#0][0m
    %267 = call %252(%0, %266, %2, %3, %244, %12, %25, %13, %152, %263, %262)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %266:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %263:value/borrow#9, %262:value/borrow#10][0m
    %268 = const.int 2
    %269 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %270 = iadd %269, %268[2m ; ops [%269:value/borrow#0, %268:value/borrow#1][0m
    %271 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %272 = ptr_offset %271, %270[2m : Ptr[0m[2m ; ops [%271:value/borrow#0, %270:index/borrow#1][0m
    %273 = load %272[2m : (Array of Double, Double, )[0m[2m ; ops [%272:value/borrow#0][0m
    %274 = const.int 2
    %275 = const.int 3073032
    %276 = ptr_offset %1, %275[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %275:index/borrow#1][0m
    %277 = primitive_cast %276 from Ptr of Char to Ptr of Char[2m ; ops [%276:value/borrow#0][0m
    %278 = call %252(%0, %277, %2, %3, %244, %12, %25, %13, %152, %274, %273)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %277:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %274:value/borrow#9, %273:value/borrow#10][0m
    %279 = const.int 3
    %280 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %281 = iadd %280, %279[2m ; ops [%280:value/borrow#0, %279:value/borrow#1][0m
    %282 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %283 = ptr_offset %282, %281[2m : Ptr[0m[2m ; ops [%282:value/borrow#0, %281:index/borrow#1][0m
    %284 = load %283[2m : (Array of Double, Double, )[0m[2m ; ops [%283:value/borrow#0][0m
    %285 = const.int 3
    %286 = const.int 3073064
    %287 = ptr_offset %1, %286[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %286:index/borrow#1][0m
    %288 = primitive_cast %287 from Ptr of Char to Ptr of Char[2m ; ops [%287:value/borrow#0][0m
    %289 = call %252(%0, %288, %2, %3, %244, %12, %25, %13, %152, %285, %284)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %288:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %285:value/borrow#9, %284:value/borrow#10][0m
    %290 = const.int 4
    %291 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %292 = iadd %291, %290[2m ; ops [%291:value/borrow#0, %290:value/borrow#1][0m
    %293 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %294 = ptr_offset %293, %292[2m : Ptr[0m[2m ; ops [%293:value/borrow#0, %292:index/borrow#1][0m
    %295 = load %294[2m : (Array of Double, Double, )[0m[2m ; ops [%294:value/borrow#0][0m
    %296 = const.int 4
    %297 = const.int 3073096
    %298 = ptr_offset %1, %297[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %297:index/borrow#1][0m
    %299 = primitive_cast %298 from Ptr of Char to Ptr of Char[2m ; ops [%298:value/borrow#0][0m
    %300 = call %252(%0, %299, %2, %3, %244, %12, %25, %13, %152, %296, %295)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %299:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %296:value/borrow#9, %295:value/borrow#10][0m
    %301 = const.int 5
    %302 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %303 = iadd %302, %301[2m ; ops [%302:value/borrow#0, %301:value/borrow#1][0m
    %304 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %305 = ptr_offset %304, %303[2m : Ptr[0m[2m ; ops [%304:value/borrow#0, %303:index/borrow#1][0m
    %306 = load %305[2m : (Array of Double, Double, )[0m[2m ; ops [%305:value/borrow#0][0m
    %307 = const.int 5
    %308 = const.int 3073128
    %309 = ptr_offset %1, %308[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %308:index/borrow#1][0m
    %310 = primitive_cast %309 from Ptr of Char to Ptr of Char[2m ; ops [%309:value/borrow#0][0m
    %311 = call %252(%0, %310, %2, %3, %244, %12, %25, %13, %152, %307, %306)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %310:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %307:value/borrow#9, %306:value/borrow#10][0m
    %312 = const.int 6
    %313 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %314 = iadd %313, %312[2m ; ops [%313:value/borrow#0, %312:value/borrow#1][0m
    %315 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %316 = ptr_offset %315, %314[2m : Ptr[0m[2m ; ops [%315:value/borrow#0, %314:index/borrow#1][0m
    %317 = load %316[2m : (Array of Double, Double, )[0m[2m ; ops [%316:value/borrow#0][0m
    %318 = const.int 6
    %319 = const.int 3073160
    %320 = ptr_offset %1, %319[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %319:index/borrow#1][0m
    %321 = primitive_cast %320 from Ptr of Char to Ptr of Char[2m ; ops [%320:value/borrow#0][0m
    %322 = call %252(%0, %321, %2, %3, %244, %12, %25, %13, %152, %318, %317)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %321:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %318:value/borrow#9, %317:value/borrow#10][0m
    %323 = const.int 7
    %324 = extract.field %133, 1[2m : Int[0m[2m ; ops [%133:container/borrow#0][0m
    %325 = iadd %324, %323[2m ; ops [%324:value/borrow#0, %323:value/borrow#1][0m
    %326 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %327 = ptr_offset %326, %325[2m : Ptr[0m[2m ; ops [%326:value/borrow#0, %325:index/borrow#1][0m
    %328 = load %327[2m : (Array of Double, Double, )[0m[2m ; ops [%327:value/borrow#0][0m
    %329 = const.int 7
    %330 = const.int 3073192
    %331 = ptr_offset %1, %330[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %330:index/borrow#1][0m
    %332 = primitive_cast %331 from Ptr of Char to Ptr of Char[2m ; ops [%331:value/borrow#0][0m
    %333 = call %252(%0, %332, %2, %3, %244, %12, %25, %13, %152, %329, %328)[2m : Array of Double[0m[2m ; ops [%252:callee/borrow#0, %0:value/consume#0, %332:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %244:value/borrow#4, %12:value/borrow#5, %25:value/borrow#6, %13:value/borrow#7, %152:value/borrow#8, %329:value/borrow#9, %328:value/borrow#10][0m
[35m    %375 = drop %244[2m : ()[0m[2m ; ops [%244:value/borrow#0][0m[0m
    %334 = construct.array_literal { %256, %267, %278, %289, %300, %311, %322, %333 }[2m : Array of Array of Double[0m[2m ; ea stack#1[0m[2m ; ops [%256:element/consume#0, %267:element/consume#1, %278:element/consume#2, %289:element/consume#3, %300:element/consume#4, %311:element/consume#5, %322:element/consume#6, %333:element/consume#7][0m
    %335 = const.int 0
    %336 = const.int 1
    %337 = primitive_cast %152 from Int to Double[2m ; ops [%152:value/borrow#0][0m
    %338 = primitive_cast %336 from Int to Double[2m ; ops [%336:value/borrow#0][0m
    %339 = fadd %337, %338[2m ; ops [%337:value/borrow#0, %338:value/borrow#1][0m
    %340 = primitive_cast %140 from Int to Double[2m ; ops [%140:value/borrow#0][0m
    %341 = fmod %339, %340[2m ; ops [%339:value/borrow#0, %340:value/borrow#1][0m
    %342 = extract.field %146, 1[2m : Int[0m[2m ; ops [%146:container/borrow#0][0m
    %343 = iadd %342, %335[2m ; ops [%342:value/borrow#0, %335:value/borrow#1][0m
    %344 = primitive_cast %341 from Double to Int[2m ; ops [%341:value/borrow#0][0m
    %345 = extract.field %146, 2[2m : Ptr[0m[2m ; ops [%146:container/borrow#0][0m
    %346 = ptr_offset %345, %343[2m : Ptr[0m[2m ; ops [%345:value/borrow#0, %343:index/borrow#1][0m
    %347 = store %346, %344[2m : ()[0m[2m ; ops [%346:value/borrow#0, %344:element/consume#1][0m
    %348 = fn_ref $vec_dot_double
    %349 = call %348(%31, %234)[2m : Double[0m[2m ; ops [%348:callee/borrow#0, %31:value/borrow#0, %234:value/borrow#1][0m
    %350 = fadd %13, %349[2m ; ops [%13:value/borrow#0, %349:value/borrow#1][0m
    return %350[2m ; ops [%350:return/consume#0][0m
}
fn fdn8.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = const.int 4
    %30 = ptr_offset %2, %29[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %29:index/borrow#1][0m
    %31 = load %30[2m : Ptr[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fn_ref $ylc_read_inlet_node_lane_i32
    %33 = const.int 0
    %34 = call %32(%31, %3, %33)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %33:value/borrow#2][0m
    %35 = const.int 5
    %36 = ptr_offset %2, %35[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %35:index/borrow#1][0m
    %37 = load %36[2m : Ptr[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = fn_ref $ylc_read_inlet_node_lane_i32
    %39 = const.int 0
    %40 = call %38(%37, %3, %39)[2m : Double[0m[2m ; ops [%38:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %39:value/borrow#2][0m
    %41 = const.int 6
    %42 = ptr_offset %2, %41[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %41:index/borrow#1][0m
    %43 = load %42[2m : Ptr[0m[2m ; ops [%42:value/borrow#0][0m
    %44 = fn_ref $ylc_read_inlet_node_lane_i32
    %45 = const.int 0
    %46 = call %44(%43, %3, %45)[2m : Double[0m[2m ; ops [%44:callee/borrow#0, %43:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2][0m
    %47 = const.int 7
    %48 = ptr_offset %2, %47[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %47:index/borrow#1][0m
    %49 = load %48[2m : Ptr[0m[2m ; ops [%48:value/borrow#0][0m
    %50 = fn_ref $ylc_read_inlet_node_lane_i32
    %51 = const.int 0
    %52 = call %50(%49, %3, %51)[2m : Double[0m[2m ; ops [%50:callee/borrow#0, %49:value/borrow#0, %3:value/borrow#1, %51:value/borrow#2][0m
    %53 = const.int 8
    %54 = ptr_offset %2, %53[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %53:index/borrow#1][0m
    %55 = load %54[2m : Ptr[0m[2m ; ops [%54:value/borrow#0][0m
    %56 = fn_ref $ylc_read_inlet_node_lane_i32
    %57 = const.int 0
    %58 = call %56(%55, %3, %57)[2m : Double[0m[2m ; ops [%56:callee/borrow#0, %55:value/borrow#0, %3:value/borrow#1, %57:value/borrow#2][0m
    %59 = const.int 9
    %60 = ptr_offset %2, %59[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %59:index/borrow#1][0m
    %61 = load %60[2m : Ptr[0m[2m ; ops [%60:value/borrow#0][0m
    %62 = fn_ref $ylc_read_inlet_node_lane_i32
    %63 = const.int 0
    %64 = call %62(%61, %3, %63)[2m : Double[0m[2m ; ops [%62:callee/borrow#0, %61:value/borrow#0, %3:value/borrow#1, %63:value/borrow#2][0m
    %65 = fn_ref $fdn8.kernel
    %66 = call %65(%0, %1, %3, %4, %10, %16, %22, %28, %34, %40, %46, %52, %58, %64)[2m : Double[0m[2m ; ops [%65:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/consume#4, %16:value/consume#5, %22:value/consume#6, %28:value/consume#7, %34:value/consume#8, %40:value/consume#9, %46:value/consume#10, %52:value/consume#11, %58:value/borrow#12, %64:value/borrow#13][0m
    %67 = fn_ref $ylc_get_output_buf
    %68 = call %67(%0)[2m : Ptr[0m[2m ; ops [%67:callee/borrow#0, %0:value/borrow#0][0m
    %69 = ptr_offset %68, %3[2m : Ptr[0m[2m ; ops [%68:value/borrow#0, %3:index/borrow#1][0m
    %70 = store %69, %66[2m : ()[0m[2m ; ops [%69:value/borrow#0, %66:element/consume#1][0m
    %71 = const.void
    return %71[2m ; ops [%71:return/consume#0][0m
}
fn fdn8.array_map.callback.6.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn fdn8.array_map.callback.6.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn fdn8.array_map.callback.6.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 w[2m: Int @borrow[0m, %5 arg0[2m: (Array of Double, Double, ) @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = extract.field %5, 0[2m : Array of Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = extract.field %5, 1[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = fdiv %7, %3[2m ; ops [%7:value/borrow#0, %3:value/borrow#1][0m
    %9 = primitive_cast %4 from Int to Double[2m ; ops [%4:value/borrow#0][0m
    %10 = fsub %9, %8[2m ; ops [%9:value/borrow#0, %8:value/borrow#1][0m
    %11 = extract.field %6, 0[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %12 = extract.field %6, 1[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %13 = extract.field %6, 2[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    %14 = fn_ref $ylc_audio_tabread_samp_kernel
    %15 = primitive_cast %1 from Ptr of Char to Ptr of Double[2m ; ops [%1:value/borrow#0][0m
    %16 = call %14(%15, %3, %11, %12, %13, %10)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %15:value/borrow#0, %3:value/borrow#1, %11:value/borrow#2, %12:value/borrow#3, %13:value/borrow#4, %10:value/borrow#5][0m
    return %16[2m ; ops [%16:return/consume#0][0m
}
fn fdn8.array_mapi.callback.7.cons(%0 parent_state[2m: Ptr @borrow[0m, %1 state_offset[2m: Int @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn fdn8.array_mapi.callback.7.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 16
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 0
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = fn_ref $lpf1p.init
    %12 = call %11(%10)[2m : ()[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0][0m
    %13 = const.int 16
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = fn_ref $hpf1p.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn fdn8.array_mapi.callback.7.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 mmy[2m: Array of Double @borrow[0m, %5 rt60[2m: Double @borrow[0m, %6 b[2m: Array of Double @borrow[0m, %7 inp[2m: Double @borrow[0m, %8 w[2m: Int @borrow[0m, %9 i[2m: Int @borrow[0m, %10 arg1[2m: (Array of Double, Double, ) @borrow[0m)[2m -> Array of Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %11 = extract.field %10, 0[2m : Array of Double[0m[2m ; ops [%10:container/borrow#0][0m
    %12 = extract.field %10, 1[2m : Double[0m[2m ; ops [%10:container/borrow#0][0m
    %13 = extract.field %6, 1[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    %14 = iadd %13, %9[2m ; ops [%13:value/borrow#0, %9:value/borrow#1][0m
    %15 = extract.field %6, 2[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    %16 = ptr_offset %15, %14[2m : Ptr[0m[2m ; ops [%15:value/borrow#0, %14:index/borrow#1][0m
    %17 = load %16[2m : Double[0m[2m ; ops [%16:value/borrow#0][0m
    %18 = fmul %7, %17[2m ; ops [%7:value/borrow#0, %17:value/borrow#1][0m
    %19 = const.int 200
    %20 = const.int 15000
    %21 = fn_ref $pow
    %22 = const.double 10
    %23 = const.double -3
    %24 = fmul %23, %12[2m ; ops [%23:value/borrow#0, %12:value/borrow#1][0m
    %25 = fdiv %24, %5[2m ; ops [%24:value/borrow#0, %5:value/borrow#1][0m
    %26 = call %21(%22, %25)[2m : Double[0m[2m ; ops [%21:callee/borrow#0, %22:value/borrow#0, %25:value/borrow#1][0m
    %27 = extract.field %4, 1[2m : Int[0m[2m ; ops [%4:container/borrow#0][0m
    %28 = iadd %27, %9[2m ; ops [%27:value/borrow#0, %9:value/borrow#1][0m
    %29 = extract.field %4, 2[2m : Ptr[0m[2m ; ops [%4:container/borrow#0][0m
    %30 = ptr_offset %29, %28[2m : Ptr[0m[2m ; ops [%29:value/borrow#0, %28:index/borrow#1][0m
    %31 = load %30[2m : Double[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fmul %26, %31[2m ; ops [%26:value/borrow#0, %31:value/borrow#1][0m
    %33 = fn_ref $lpf1p.kernel
    %34 = const.int 0
    %35 = ptr_offset %1, %34[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %34:index/borrow#1][0m
    %36 = primitive_cast %35 from Ptr of Char to Ptr of Char[2m ; ops [%35:value/borrow#0][0m
    %37 = primitive_cast %20 from Int to Double[2m ; ops [%20:value/borrow#0][0m
    %38 = call %33(%0, %36, %2, %3, %37, %32)[2m : Double[0m[2m ; ops [%33:callee/borrow#0, %0:value/consume#0, %36:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %37:value/borrow#4, %32:value/borrow#5][0m
    %39 = fn_ref $hpf1p.kernel
    %40 = const.int 16
    %41 = ptr_offset %1, %40[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %40:index/borrow#1][0m
    %42 = primitive_cast %41 from Ptr of Char to Ptr of Char[2m ; ops [%41:value/borrow#0][0m
    %43 = primitive_cast %19 from Int to Double[2m ; ops [%19:value/borrow#0][0m
    %44 = call %39(%0, %42, %2, %3, %43, %38)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %42:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %43:value/borrow#4, %38:value/borrow#5][0m
    %45 = fadd %18, %44[2m ; ops [%18:value/borrow#0, %44:value/borrow#1][0m
    %46 = extract.field %11, 1[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %47 = iadd %46, %8[2m ; ops [%46:value/borrow#0, %8:value/borrow#1][0m
    %48 = extract.field %11, 2[2m : Ptr[0m[2m ; ops [%11:container/borrow#0][0m
    %49 = ptr_offset %48, %47[2m : Ptr[0m[2m ; ops [%48:value/borrow#0, %47:index/borrow#1][0m
    %50 = store %49, %45[2m : ()[0m[2m ; ops [%49:value/borrow#0, %45:element/consume#1][0m
[35m    %51 = dup %11[2m : ()[0m[2m ; ops [%11:value/borrow#0][0m[0m
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn room.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $room.frame
    %3 = const.string "room"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 3073224
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $room.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $room.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn room.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn8.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn room.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.015
    %6 = const.double 0.0189
    %7 = const.double 0.024
    %8 = const.double 0.0307
    %9 = const.double 0.0388
    %10 = const.double 0.0491
    %11 = const.double 0.0625
    %12 = const.double 0.0792
    %13 = const.double 0.5
    %14 = fn_ref $fdn8.kernel
    %15 = const.int 0
    %16 = ptr_offset %1, %15[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Char[2m ; ops [%16:value/borrow#0][0m
    %18 = call %14(%0, %17, %2, %3, %5, %6, %7, %8, %9, %10, %11, %12, %13, %4)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %0:value/consume#0, %17:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/consume#8, %10:value/consume#9, %11:value/consume#10, %12:value/consume#11, %13:value/borrow#12, %4:value/borrow#13][0m
    return %18[2m ; ops [%18:return/consume#0][0m
}
fn room.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $room.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn concert.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $concert.frame
    %3 = const.string "concert"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 3073224
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $concert.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $concert.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn concert.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn8.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn concert.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.0397
    %6 = const.double 0.05
    %7 = const.double 0.0638
    %8 = const.double 0.0814
    %9 = const.double 0.1027
    %10 = const.double 0.1301
    %11 = const.double 0.1656
    %12 = const.double 0.21
    %13 = const.double 2
    %14 = fn_ref $fdn8.kernel
    %15 = const.int 0
    %16 = ptr_offset %1, %15[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Char[2m ; ops [%16:value/borrow#0][0m
    %18 = call %14(%0, %17, %2, %3, %5, %6, %7, %8, %9, %10, %11, %12, %13, %4)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %0:value/consume#0, %17:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/consume#8, %10:value/consume#9, %11:value/consume#10, %12:value/consume#11, %13:value/borrow#12, %4:value/borrow#13][0m
    return %18[2m ; ops [%18:return/consume#0][0m
}
fn concert.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $concert.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn cathedral.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $cathedral.frame
    %3 = const.string "cathedral"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 3073224
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $cathedral.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $cathedral.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn cathedral.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn8.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn cathedral.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.0941
    %6 = const.double 0.1188
    %7 = const.double 0.1509
    %8 = const.double 0.1925
    %9 = const.double 0.2435
    %10 = const.double 0.3083
    %11 = const.double 0.3925
    %12 = const.double 0.4975
    %13 = const.double 5
    %14 = fn_ref $fdn8.kernel
    %15 = const.int 0
    %16 = ptr_offset %1, %15[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Char[2m ; ops [%16:value/borrow#0][0m
    %18 = call %14(%0, %17, %2, %3, %5, %6, %7, %8, %9, %10, %11, %12, %13, %4)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %0:value/consume#0, %17:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/consume#8, %10:value/consume#9, %11:value/consume#10, %12:value/consume#11, %13:value/borrow#12, %4:value/borrow#13][0m
    return %18[2m ; ops [%18:return/consume#0][0m
}
fn cathedral.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $cathedral.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn mega_cathedral.cons(%0 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $mega_cathedral.frame
    %3 = const.string "mega_cathedral"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 3073224
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $mega_cathedral.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $mega_cathedral.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn mega_cathedral.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 0
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = fn_ref $fdn8.init
    %8 = call %7(%6)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn mega_cathedral.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 inp[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.double 0.0941
    %6 = const.double 0.1188
    %7 = const.double 0.1509
    %8 = const.double 0.1925
    %9 = const.double 0.2435
    %10 = const.double 0.3083
    %11 = const.double 0.3925
    %12 = const.double 0.4975
    %13 = const.double 10
    %14 = fn_ref $fdn8.kernel
    %15 = const.int 0
    %16 = ptr_offset %1, %15[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Char[2m ; ops [%16:value/borrow#0][0m
    %18 = call %14(%0, %17, %2, %3, %5, %6, %7, %8, %9, %10, %11, %12, %13, %4)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %0:value/consume#0, %17:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %5:value/consume#4, %6:value/consume#5, %7:value/consume#6, %8:value/consume#7, %9:value/consume#8, %10:value/consume#9, %11:value/consume#10, %12:value/consume#11, %13:value/borrow#12, %4:value/borrow#13][0m
    return %18[2m ; ops [%18:return/consume#0][0m
}
fn mega_cathedral.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $mega_cathedral.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
fn dust.cons(%0 density[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $dust.frame
    %3 = const.string "dust"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 0
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $dust.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $dust.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/consume#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn dust.init(%0 state[2m: Ptr @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.void
    return %1[2m ; ops [%1:return/consume#0][0m
}
fn dust.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 density[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fmul %4, %3[2m ; ops [%4:value/borrow#0, %3:value/borrow#1][0m
    %6 = fn_ref $rand_double
    %7 = call %6()[2m : Double[0m[2m ; ops [%6:callee/borrow#0][0m
    %8 = flt %7, %5[2m ; ops [%7:value/borrow#0, %5:value/borrow#1][0m
    %9 = const.double 0
    %10 = const.double 1
    %11 = const.bool true
    %12 = primitive_cast %11 from Double to Bool[2m ; ops [%11:value/borrow#0][0m
    %13 = beq %8, %12[2m ; ops [%8:value/borrow#0, %12:value/borrow#1][0m
    %14 = primitive_cast %13 from Bool to Double[2m ; ops [%13:value/borrow#0][0m
    %15 = const.double 1
    %16 = fsub %15, %14[2m ; ops [%15:value/borrow#0, %14:value/borrow#1][0m
    %17 = fmul %14, %10[2m ; ops [%14:value/borrow#0, %10:value/borrow#1][0m
    %18 = fmul %16, %9[2m ; ops [%16:value/borrow#0, %9:value/borrow#1][0m
    %19 = fadd %17, %18[2m ; ops [%17:value/borrow#0, %18:value/borrow#1][0m
    return %19[2m ; ops [%19:return/consume#0][0m
}
fn dust.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @consume[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $dust.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/consume#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
extern fn int32_parse(%0 arg0[2m: String @borrow[0m)[2m -> Int @owned[0m;
fn DSP.DSP_common.parse_note(%0 s[2m: String @borrow[0m)[2m -> Option
  | Some of Int
  | None
 @owned[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %3 = ptr_offset %2, %1[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %1:index/borrow#1][0m
    %4 = load %3[2m : Char[0m[2m ; ops [%3:value/borrow#0][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %27 = phi [bb4: %7, bb6: %10, bb8: %13, bb10: %16, bb12: %19, bb14: %22, bb16: %25, bb18: %26][2m : Int[0m[2m ; ops [%7:value/borrow#0, %10:value/borrow#1, %13:value/borrow#2, %16:value/borrow#3, %19:value/borrow#4, %22:value/borrow#5, %25:value/borrow#6, %26:value/borrow#7][0m
    %28 = const.int 1
    %29 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %30 = ptr_offset %29, %28[2m : Ptr[0m[2m ; ops [%29:value/borrow#0, %28:index/borrow#1][0m
    %31 = load %30[2m : Char[0m[2m ; ops [%30:value/borrow#0][0m
    br bb21
  bb3 match.arm.0.test:
    %5 = const.char 'c'
    %6 = ceq %4, %5[2m ; ops [%4:value/borrow#0, %5:value/borrow#1][0m
    cond %6, bb4, bb5[2m ; ops [%6:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %7 = const.int 0
    br bb2
  bb5 match.arm.1.test:
    %8 = const.char 'd'
    %9 = ceq %4, %8[2m ; ops [%4:value/borrow#0, %8:value/borrow#1][0m
    cond %9, bb6, bb7[2m ; ops [%9:condition/borrow#0][0m
  bb6 match.arm.1.body:
    %10 = const.int 2
    br bb2
  bb7 match.arm.2.test:
    %11 = const.char 'e'
    %12 = ceq %4, %11[2m ; ops [%4:value/borrow#0, %11:value/borrow#1][0m
    cond %12, bb8, bb9[2m ; ops [%12:condition/borrow#0][0m
  bb8 match.arm.2.body:
    %13 = const.int 4
    br bb2
  bb9 match.arm.3.test:
    %14 = const.char 'f'
    %15 = ceq %4, %14[2m ; ops [%4:value/borrow#0, %14:value/borrow#1][0m
    cond %15, bb10, bb11[2m ; ops [%15:condition/borrow#0][0m
  bb10 match.arm.3.body:
    %16 = const.int 5
    br bb2
  bb11 match.arm.4.test:
    %17 = const.char 'g'
    %18 = ceq %4, %17[2m ; ops [%4:value/borrow#0, %17:value/borrow#1][0m
    cond %18, bb12, bb13[2m ; ops [%18:condition/borrow#0][0m
  bb12 match.arm.4.body:
    %19 = const.int 7
    br bb2
  bb13 match.arm.5.test:
    %20 = const.char 'a'
    %21 = ceq %4, %20[2m ; ops [%4:value/borrow#0, %20:value/borrow#1][0m
    cond %21, bb14, bb15[2m ; ops [%21:condition/borrow#0][0m
  bb14 match.arm.5.body:
    %22 = const.int 9
    br bb2
  bb15 match.arm.6.test:
    %23 = const.char 'b'
    %24 = ceq %4, %23[2m ; ops [%4:value/borrow#0, %23:value/borrow#1][0m
    cond %24, bb16, bb17[2m ; ops [%24:condition/borrow#0][0m
  bb16 match.arm.6.body:
    %25 = const.int 11
    br bb2
  bb17 match.arm.7.test:
    br bb18
  bb18 match.arm.7.body:
    %26 = const.int -1
    br bb2
  bb19 match.no_match:
    unreachable
  bb20 match.cont:
    %84 = phi [bb22: %49, bb24: %67, bb26: %83][2m : (Int, Int, )[0m[2m ; ops [%49:value/borrow#0, %67:value/borrow#1, %83:value/borrow#2][0m
    %85 = extract.field %84, 0[2m : Int[0m[2m ; ops [%84:container/borrow#0][0m
    %86 = extract.field %84, 1[2m : Int[0m[2m ; ops [%84:container/borrow#0][0m
    br bb29
  bb21 match.arm.0.test:
    %32 = const.char '#'
    %33 = ceq %31, %32[2m ; ops [%31:value/borrow#0, %32:value/borrow#1][0m
    cond %33, bb22, bb23[2m ; ops [%33:condition/borrow#0][0m
  bb22 match.arm.0.body:
    %34 = const.int 1
    %35 = fn_ref $int32_parse
    %36 = const.int 2
    %37 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %38 = const.int 0
    %39 = igt %37, %38[2m ; ops [%37:value/borrow#0, %38:value/borrow#1][0m
    %40 = primitive_cast %39 from Bool to Int[2m ; ops [%39:value/borrow#0][0m
    %41 = imul %36, %40[2m ; ops [%36:value/borrow#0, %40:value/borrow#1][0m
    %42 = isub %37, %41[2m ; ops [%37:value/borrow#0, %41:value/borrow#1][0m
    %43 = extract.field %0, 1[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %44 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %45 = ptr_offset %44, %41[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %41:index/borrow#1][0m
    %46 = iadd %43, %41[2m ; ops [%43:value/borrow#0, %41:value/borrow#1][0m
    %47 = construct.tuple { %42, %46, %45 }[2m : String[0m[2m ; ops [%42:field/borrow#0, %46:field/borrow#1, %45:field/borrow#2][0m
    %48 = call %35(%47)[2m : Int[0m[2m ; ops [%35:callee/borrow#0, %47:value/borrow#0][0m
    %49 = construct.tuple { %34, %48 }[2m : (Int, Int, )[0m[2m ; ops [%34:field/consume#0, %48:field/consume#1][0m
    br bb20
  bb23 match.arm.1.test:
    %50 = const.char '_'
    %51 = ceq %31, %50[2m ; ops [%31:value/borrow#0, %50:value/borrow#1][0m
    cond %51, bb24, bb25[2m ; ops [%51:condition/borrow#0][0m
  bb24 match.arm.1.body:
    %52 = const.int -1
    %53 = fn_ref $int32_parse
    %54 = const.int 2
    %55 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %56 = const.int 0
    %57 = igt %55, %56[2m ; ops [%55:value/borrow#0, %56:value/borrow#1][0m
    %58 = primitive_cast %57 from Bool to Int[2m ; ops [%57:value/borrow#0][0m
    %59 = imul %54, %58[2m ; ops [%54:value/borrow#0, %58:value/borrow#1][0m
    %60 = isub %55, %59[2m ; ops [%55:value/borrow#0, %59:value/borrow#1][0m
    %61 = extract.field %0, 1[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %62 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %63 = ptr_offset %62, %59[2m : Ptr[0m[2m ; ops [%62:value/borrow#0, %59:index/borrow#1][0m
    %64 = iadd %61, %59[2m ; ops [%61:value/borrow#0, %59:value/borrow#1][0m
    %65 = construct.tuple { %60, %64, %63 }[2m : String[0m[2m ; ops [%60:field/borrow#0, %64:field/borrow#1, %63:field/borrow#2][0m
    %66 = call %53(%65)[2m : Int[0m[2m ; ops [%53:callee/borrow#0, %65:value/borrow#0][0m
    %67 = construct.tuple { %52, %66 }[2m : (Int, Int, )[0m[2m ; ops [%52:field/consume#0, %66:field/consume#1][0m
    br bb20
  bb25 match.arm.2.test:
    br bb26
  bb26 match.arm.2.body:
    %68 = const.int 0
    %69 = fn_ref $int32_parse
    %70 = const.int 1
    %71 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %72 = const.int 0
    %73 = igt %71, %72[2m ; ops [%71:value/borrow#0, %72:value/borrow#1][0m
    %74 = primitive_cast %73 from Bool to Int[2m ; ops [%73:value/borrow#0][0m
    %75 = imul %70, %74[2m ; ops [%70:value/borrow#0, %74:value/borrow#1][0m
    %76 = isub %71, %75[2m ; ops [%71:value/borrow#0, %75:value/borrow#1][0m
    %77 = extract.field %0, 1[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %78 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %79 = ptr_offset %78, %75[2m : Ptr[0m[2m ; ops [%78:value/borrow#0, %75:index/borrow#1][0m
    %80 = iadd %77, %75[2m ; ops [%77:value/borrow#0, %75:value/borrow#1][0m
    %81 = construct.tuple { %76, %80, %79 }[2m : String[0m[2m ; ops [%76:field/borrow#0, %80:field/borrow#1, %79:field/borrow#2][0m
    %82 = call %69(%81)[2m : Int[0m[2m ; ops [%69:callee/borrow#0, %81:value/borrow#0][0m
    %83 = construct.tuple { %68, %82 }[2m : (Int, Int, )[0m[2m ; ops [%68:field/consume#0, %82:field/consume#1][0m
    br bb20
  bb27 match.no_match:
    unreachable
  bb28 match.cont:
    %97 = phi [bb30: %89, bb32: %96][2m : Option
  | Some of Int
  | None
[0m[2m ; ops [%89:value/borrow#0, %96:value/borrow#1][0m
    return %97[2m ; ops [%97:return/consume#0][0m
  bb29 match.arm.0.test:
    %87 = const.int -1
    %88 = ieq %27, %87[2m ; ops [%27:value/borrow#0, %87:value/borrow#1][0m
    cond %88, bb30, bb31[2m ; ops [%88:condition/borrow#0][0m
  bb30 match.arm.0.body:
    %89 = construct.variant None#1()[2m : Option
  | Some of Int
  | None
[0m
    br bb28
  bb31 match.arm.1.test:
    br bb32
  bb32 match.arm.1.body:
    %90 = const.int 1
    %91 = iadd %86, %90[2m ; ops [%86:value/borrow#0, %90:value/borrow#1][0m
    %92 = const.int 12
    %93 = imul %91, %92[2m ; ops [%91:value/borrow#0, %92:value/borrow#1][0m
    %94 = iadd %93, %27[2m ; ops [%93:value/borrow#0, %27:value/borrow#1][0m
    %95 = iadd %94, %85[2m ; ops [%94:value/borrow#0, %85:value/borrow#1][0m
    %96 = construct.variant Some#0(%95)[2m : Option
  | Some of Int
  | None
[0m[2m ; ops [%95:field/consume#0][0m
    br bb28
}
fn DSP.DSP_common.midi_freq(%0 n[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = const.double 440
    %2 = fn_ref $pow
    %3 = const.double 2
    %4 = const.double 69
    %5 = fsub %0, %4[2m ; ops [%0:value/borrow#0, %4:value/borrow#1][0m
    %6 = const.double 12
    %7 = fdiv %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    %8 = call %2(%3, %7)[2m : Double[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %7:value/borrow#1][0m
    %9 = fmul %1, %8[2m ; ops [%1:value/borrow#0, %8:value/borrow#1][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn DSP.DSP_common.midiratio(%0 n[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = fn_ref $pow
    %2 = const.double 2
    %3 = const.double 12
    %4 = fdiv %0, %3[2m ; ops [%0:value/borrow#0, %3:value/borrow#1][0m
    %5 = call %1(%2, %4)[2m : Double[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn xline.cons(%0 start[2m: Double @borrow[0m, %1 finish[2m: Double @borrow[0m, %2 duration[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %3 = fn_ref $ylc_create_audio_frame_node
    %4 = fn_ref $xline.frame
    %5 = const.string "xline"
    %6 = const.int 3
    %7 = const.int 1
    %8 = const.int 24
    %9 = call %3(%4, %6, %7, %8, %5)[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1, %7:value/borrow#2, %8:value/borrow#3, %5:value/borrow#4][0m
    %10 = fn_ref $ylc_audio_node_set_state_init
    %11 = fn_ref $xline.init
    %12 = call %10(%9, %11)[2m : ()[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %11:value/borrow#1][0m
    %13 = fn_ref $ylc_audio_node_inline_state
    %14 = call %13(%9)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %9:value/borrow#0][0m
    %15 = fn_ref $xline.init
    %16 = call %15(%14)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %14:value/borrow#0][0m
    %17 = fn_ref $node_connect_input
    %18 = fn_ref $const_sig
    %19 = call %18(%0)[2m : Ptr[0m[2m ; ops [%18:callee/borrow#0, %0:value/borrow#0][0m
    %20 = const.int 0
    %21 = call %17(%20, %9, %19)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %20:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %22 = fn_ref $const_sig
    %23 = call %22(%1)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %1:value/borrow#0][0m
    %24 = const.int 1
    %25 = call %17(%24, %9, %23)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %24:value/borrow#0, %9:value/borrow#1, %23:value/borrow#2][0m
    %26 = fn_ref $const_sig
    %27 = call %26(%2)[2m : Ptr[0m[2m ; ops [%26:callee/borrow#0, %2:value/borrow#0][0m
    %28 = const.int 2
    %29 = call %17(%28, %9, %27)[2m : ()[0m[2m ; ops [%17:callee/borrow#0, %28:value/borrow#0, %9:value/borrow#1, %27:value/borrow#2][0m
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn xline.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 8
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 0
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn xline.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 start[2m: Double @borrow[0m, %5 finish[2m: Double @borrow[0m, %6 duration[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %7 = const.int 0
    %8 = primitive_cast %7 from Int to Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.int 0
    %10 = ptr_offset %1, %9[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %9:index/borrow#1][0m
    %11 = primitive_cast %10 from Ptr of Char to Ptr of Double[2m ; ops [%10:value/borrow#0][0m
    %12 = fn_ref $ylc_audio_trig_once_kernel
    %13 = call %12(%11, %3, %8)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0, %3:value/borrow#1, %8:value/borrow#2][0m
    %14 = fn_ref $ylc_audio_decay_kernel
    %15 = const.int 8
    %16 = ptr_offset %1, %15[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %15:index/borrow#1][0m
    %17 = primitive_cast %16 from Ptr of Char to Ptr of Double[2m ; ops [%16:value/borrow#0][0m
    %18 = call %14(%17, %3, %6, %13)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %17:value/borrow#0, %3:value/borrow#1, %6:value/borrow#2, %13:value/borrow#3][0m
    %19 = fn_ref $ylc_audio_scale_kernel
    %20 = primitive_cast %1 from Ptr of Char to Ptr of Double[2m ; ops [%1:value/borrow#0][0m
    %21 = call %19(%20, %3, %5, %4, %18)[2m : Double[0m[2m ; ops [%19:callee/borrow#0, %20:value/borrow#0, %3:value/borrow#1, %5:value/borrow#2, %4:value/borrow#3, %18:value/borrow#4][0m
    return %21[2m ; ops [%21:return/consume#0][0m
}
fn xline.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = fn_ref $xline.kernel
    %24 = call %23(%0, %1, %3, %4, %10, %16, %22)[2m : Double[0m[2m ; ops [%23:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/borrow#4, %16:value/borrow#5, %22:value/borrow#6][0m
    %25 = fn_ref $ylc_get_output_buf
    %26 = call %25(%0)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %0:value/borrow#0][0m
    %27 = ptr_offset %26, %3[2m : Ptr[0m[2m ; ops [%26:value/borrow#0, %3:index/borrow#1][0m
    %28 = store %27, %24[2m : ()[0m[2m ; ops [%27:value/borrow#0, %24:element/consume#1][0m
    %29 = const.void
    return %29[2m ; ops [%29:return/consume#0][0m
}
extern fn ylc_audio_trig_once_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_decay_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_scale_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn init_audio()[2m -> () @none[0m;
extern fn play_pattern(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: T @borrow[0m)[2m -> Task @owned[0m;
extern fn cancel_task(%0 arg0[2m: Task @borrow[0m)[2m -> () @none[0m;
extern fn play_node(%0 arg0[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m;
extern fn play_into(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m;
fn DSP.~>(%0 n[2m: Ptr @borrow[0m, %1 t[2m: Ptr @borrow[0m)[2m -> Ptr @owned[0m {
  bb0 entry:
    %2 = fn_ref $play_into
    %3 = call %2(%1, %0)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %1:value/borrow#0, %0:value/borrow#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
}
extern fn set_input_scalar(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Int @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Ptr @owned[0m;
extern fn set_input_trig(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Int @borrow[0m)[2m -> Ptr @owned[0m;
extern fn record_start(%0 arg0[2m: String @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Int @owned[0m;
extern fn record_stop()[2m -> () @none[0m;
fn $Math_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = const.int 2147483647
    %1 = global_store @$global.Math.MAX_INT, %0[2m : Int[0m[2m ; ops [%0:value/consume#0][0m
    %2 = const.double 3.14159
    %3 = global_store @$global.Math.PI, %2[2m : Double[0m[2m ; ops [%2:value/consume#0][0m
    %4 = const.double 1e-12
    %5 = global_store @$global.Math.epsilon, %4[2m : Double[0m[2m ; ops [%4:value/consume#0][0m
    %6 = fn_ref $rand_int
    %7 = fn_ref $rand_double
    %8 = fn_ref $rand_double_range
    %9 = fn_ref $rand_double_range
    %10 = global_store @$global.Math.rrand, %9[2m : (Double -> (Double -> Double))[0m[2m ; ops [%9:value/consume#0][0m
    %11 = fn_ref $amp_db
    %12 = fn_ref $db_amp
    %13 = fn_ref $sin
    %14 = fn_ref $cos
    %15 = fn_ref $bipolar_scale
    %16 = fn_ref $unipolar_scale
    %17 = fn_ref $double_array_init
    %18 = fn_ref $double_array
    %19 = fn_ref $pow
    %20 = fn_ref $tanh
    %21 = fn_ref $tan
    %22 = fn_ref $atan
    %23 = fn_ref $atan2
    %24 = fn_ref $ilog2
    %25 = fn_ref $log
    %26 = fn_ref $u64pow
    %27 = fn_ref $sqrt
    %28 = fn_ref $floor
    %29 = fn_ref $round
    %30 = fn_ref $ceil
    %31 = fn_ref $Math.max
    %32 = fn_ref $Math.min
    %33 = fn_ref $Math.clamp
    %34 = fn_ref $Math.abs
    %35 = fn_ref $Math.neg
    %36 = fn_ref $Math.normalize
    %37 = fn_ref $Math.chance
    %38 = fn_ref $Math.chance2
    %39 = fn_ref $Math.shuffle
    %40 = fn_ref $Math.shuffle_ip
    %41 = fn_ref $Math.array_choose
    %42 = fn_ref $nonzero_randu_double
    %43 = fn_ref $log
    %44 = fn_ref $exp
    %45 = fn_ref $_randn_pair
    %46 = fn_ref $Math.randn
    %47 = fn_ref $Math.ilog10
    %48 = fn_ref $ipow
    %49 = const.void
    return %49[2m ; ops [%49:return/consume#0][0m
}
fn Math.max(%0 a[2m: `1607 @consume[0m, %1 b[2m: `1607 @consume[0m)[2m -> `1607 @owned[0m {
  bb0 entry:
    %2 = call @>(%0, %1)[2m : Bool[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %5 = phi [bb4: %0, bb6: %1][2m : `1607[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn Math.min(%0 a[2m: `1614 @consume[0m, %1 b[2m: `1614 @consume[0m)[2m -> `1614 @owned[0m {
  bb0 entry:
    %2 = call @<(%0, %1)[2m : Bool[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %5 = phi [bb4: %0, bb6: %1][2m : `1614[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn Math.clamp(%0 mn[2m: `1622 @consume[0m, %1 mx[2m: `1622 @consume[0m, %2 v[2m: `1622 @consume[0m)[2m -> `1622 @owned[0m {
  bb0 entry:
    %3 = fn_ref $Math.max
    %4 = fn_ref $Math.min
    %5 = call %4(%1, %2)[2m : `1622[0m[2m ; ops [%4:callee/borrow#0, %1:value/consume#0, %2:value/consume#1][0m
    %6 = call %3(%0, %5)[2m : `1622[0m[2m ; ops [%3:callee/borrow#0, %0:value/consume#0, %5:value/consume#1][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn Math.abs(%0 a[2m: Int @consume[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %1 = const.int 0
    %2 = igt %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %7 = phi [bb4: %0, bb6: %6][2m : Int[0m[2m ; ops [%0:value/borrow#0, %6:value/borrow#1][0m
    return %7[2m ; ops [%7:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = const.bool true
    %4 = beq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb4, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %5 = const.int -1
    %6 = imul %0, %5[2m ; ops [%0:value/borrow#0, %5:value/borrow#1][0m
    br bb2
}
fn Math.neg(%0 a[2m: `1636 @borrow[0m)[2m -> `1637 @owned[0m {
  bb0 entry:
    %1 = const.int -1
    %2 = call @*(%1, %0)[2m : `1637[0m[2m ; ops [_:callee/borrow#0, %1:value/borrow#0, %0:value/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn Math.normalize(%0 mn[2m: `1646 @borrow[0m, %1 mx[2m: `1648 @borrow[0m, %2 v[2m: `1645 @borrow[0m)[2m -> `1644 @owned[0m {
  bb0 entry:
    %3 = call @-(%2, %0)[2m : `1642[0m[2m ; ops [_:callee/borrow#0, %2:value/borrow#0, %0:value/borrow#1][0m
    %4 = call @-(%1, %0)[2m : `1643[0m[2m ; ops [_:callee/borrow#0, %1:value/borrow#0, %0:value/borrow#1][0m
    %5 = call @/(%3, %4)[2m : `1644[0m[2m ; ops [_:callee/borrow#0, %3:value/borrow#0, %4:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn Math.chance(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> `1658) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %2 = fn_ref $rand_double
    %3 = call %2()[2m : Double[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = flte %3, %0[2m ; ops [%3:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %10 = const.void
    return %10[2m ; ops [%10:return/consume#0][0m
  bb3 match.arm.0.test:
    %5 = const.bool true
    %6 = beq %4, %5[2m ; ops [%4:value/borrow#0, %5:value/borrow#1][0m
    cond %6, bb4, bb5[2m ; ops [%6:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %7 = call %1()[2m : `1658[0m[2m ; ops [%1:callee/borrow#0][0m
    %8 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %9 = const.void
    br bb2
}
fn Math.chance2(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> `1667) @borrow[0m, %2 g[2m: (() -> `1668) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %3 = fn_ref $rand_double
    %4 = call %3()[2m : Double[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = flte %4, %0[2m ; ops [%4:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
  bb3 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb4, bb5[2m ; ops [%7:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %8 = call %1()[2m : `1667[0m[2m ; ops [%1:callee/borrow#0][0m
    %9 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %10 = call %2()[2m : `1668[0m[2m ; ops [%2:callee/borrow#0][0m
    %11 = const.void
    br bb2
}
fn Math.shuffle(%0 n[2m: Int @borrow[0m)[2m -> Array of Int @owned[0m {
  bb0 entry:
    %1 = fn_ref $Math.shuffle.<anonymous>
    %2 = construct.array_fill %0, %1[2m : Array of Int[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%0:value/borrow#0, %1:function/borrow#1][0m
    %3 = const.int 0
    br bb1
  bb1 loop.cond:
    %4 = phi [bb0: %3, bb3: %26][2m : Int[0m[2m ; ops [%3:value/borrow#0, %26:value/borrow#1][0m
    %5 = ult %4, %0[2m ; ops [%4:value/borrow#0, %0:value/borrow#1][0m
    cond %5, bb2, bb4[2m ; ops [%5:condition/borrow#0][0m
  bb2 loop.body:
    %6 = const.int 1
    %7 = isub %0, %6[2m ; ops [%0:value/borrow#0, %6:value/borrow#1][0m
    %8 = isub %7, %4[2m ; ops [%7:value/borrow#0, %4:value/borrow#1][0m
    %9 = fn_ref $rand_int
    %10 = const.int 1
    %11 = iadd %8, %10[2m ; ops [%8:value/borrow#0, %10:value/borrow#1][0m
    %12 = call %9(%11)[2m : Int[0m[2m ; ops [%9:callee/borrow#0, %11:value/borrow#0][0m
    %13 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %14 = ptr_offset %13, %8[2m : Ptr[0m[2m ; ops [%13:value/borrow#0, %8:index/borrow#1][0m
    %15 = load %14[2m : Int[0m[2m ; ops [%14:value/borrow#0][0m
    %16 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %17 = ptr_offset %16, %12[2m : Ptr[0m[2m ; ops [%16:value/borrow#0, %12:index/borrow#1][0m
    %18 = load %17[2m : Int[0m[2m ; ops [%17:value/borrow#0][0m
    %19 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %20 = ptr_offset %19, %8[2m : Ptr[0m[2m ; ops [%19:value/borrow#0, %8:index/borrow#1][0m
[35m    %28 = dup %18[2m : ()[0m[2m ; ops [%18:value/borrow#0][0m[0m
    %21 = store %20, %18[2m : ()[0m[2m ; ops [%20:value/borrow#0, %18:element/consume#1][0m
    %22 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %23 = ptr_offset %22, %12[2m : Ptr[0m[2m ; ops [%22:value/borrow#0, %12:index/borrow#1][0m
[35m    %29 = dup %15[2m : ()[0m[2m ; ops [%15:value/borrow#0][0m[0m
    %24 = store %23, %15[2m : ()[0m[2m ; ops [%23:value/borrow#0, %15:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %25 = const.int 1
    %26 = iadd %4, %25[2m ; ops [%4:value/borrow#0, %25:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %27 = const.void
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn Math.shuffle.<anonymous>(%0 i[2m: Int @consume[0m)[2m -> Int @owned[0m {
  bb0 entry:
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn Math.shuffle_ip(%0 arr[2m: Array of `1700 @consume[0m)[2m -> Array of `1700 @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = const.int 0
    br bb1
  bb1 loop.cond:
    %3 = phi [bb0: %2, bb3: %25][2m : Int[0m[2m ; ops [%2:value/borrow#0, %25:value/borrow#1][0m
    %4 = ult %3, %1[2m ; ops [%3:value/borrow#0, %1:value/borrow#1][0m
    cond %4, bb2, bb4[2m ; ops [%4:condition/borrow#0][0m
  bb2 loop.body:
    %5 = const.int 1
    %6 = isub %1, %5[2m ; ops [%1:value/borrow#0, %5:value/borrow#1][0m
    %7 = isub %6, %3[2m ; ops [%6:value/borrow#0, %3:value/borrow#1][0m
    %8 = fn_ref $rand_int
    %9 = const.int 1
    %10 = iadd %7, %9[2m ; ops [%7:value/borrow#0, %9:value/borrow#1][0m
    %11 = call %8(%10)[2m : Int[0m[2m ; ops [%8:callee/borrow#0, %10:value/borrow#0][0m
    %12 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %13 = ptr_offset %12, %7[2m : Ptr[0m[2m ; ops [%12:value/borrow#0, %7:index/borrow#1][0m
    %14 = load %13[2m : `1700[0m[2m ; ops [%13:value/borrow#0][0m
    %15 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %16 = ptr_offset %15, %11[2m : Ptr[0m[2m ; ops [%15:value/borrow#0, %11:index/borrow#1][0m
    %17 = load %16[2m : `1700[0m[2m ; ops [%16:value/borrow#0][0m
    %18 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %19 = ptr_offset %18, %7[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %7:index/borrow#1][0m
[35m    %27 = dup %17[2m : ()[0m[2m ; ops [%17:value/borrow#0][0m[0m
    %20 = store %19, %17[2m : ()[0m[2m ; ops [%19:value/borrow#0, %17:element/consume#1][0m
    %21 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %22 = ptr_offset %21, %11[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %11:index/borrow#1][0m
[35m    %28 = dup %14[2m : ()[0m[2m ; ops [%14:value/borrow#0][0m[0m
    %23 = store %22, %14[2m : ()[0m[2m ; ops [%22:value/borrow#0, %14:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %24 = const.int 1
    %25 = iadd %3, %24[2m ; ops [%3:value/borrow#0, %24:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %26 = const.void
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn Math.array_choose(%0 arr[2m: Array of `1707 @borrow[0m)[2m -> `1707 @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = fn_ref $rand_int
    %3 = call %2(%1)[2m : Int[0m[2m ; ops [%2:callee/borrow#0, %1:value/borrow#0][0m
    %4 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : `1707[0m[2m ; ops [%5:value/borrow#0][0m
[35m    %7 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn Math.randn(%0 mu[2m: Double @borrow[0m, %1 sig[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fn_ref $_randn_pair
    %3 = call %2(%0, %1)[2m : (Double, Double, )[0m[2m ; ops [%2:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    %4 = extract.field %3, 0[2m : Double[0m[2m ; ops [%3:container/borrow#0][0m
    %5 = extract.field %3, 1[2m : Double[0m[2m ; ops [%3:container/borrow#0][0m
    return %4[2m ; ops [%4:return/consume#0][0m
}
fn Math.ilog10(%0 a[2m: Int @borrow[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %1 = fn_ref $Math.ilog10.aux
    %2 = const.int 0
    %3 = call %1 as $Math_ilog10_aux.Int.Int.Int(%2, %0)[2m : Int[0m[2m ; ops [%1:callee/borrow#0, %2:value/consume#0, %0:value/borrow#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
}
fn Math.ilog10.aux(%0 d[2m: `1725 @consume[0m, %1 x[2m: Int @borrow[0m)[2m -> `1725 @owned[0m {
  bb0 entry:
    %2 = const.int 10
    %3 = idiv %1, %2[2m ; ops [%1:value/borrow#0, %2:value/borrow#1][0m
    %4 = const.int 0
    %5 = igt %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb3, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb3 match.arm.0.body:
    %8 = fn_ref $Math.ilog10.aux
    %9 = const.int 1
    %10 = call @+(%0, %9)[2m : `1725[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %9:value/borrow#1][0m
    tail_call %8(%10, %3)[2m ; ops [%8:callee/borrow#0, %10:value/consume#0, %3:value/consume#1][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn Math_ilog10_aux.Int.Int.Int(%0 d[2m: Int @consume[0m, %1 x[2m: Int @borrow[0m)[2m -> Int @owned[0m {
  bb0 entry:
    %2 = const.int 10
    %3 = idiv %1, %2[2m ; ops [%1:value/borrow#0, %2:value/borrow#1][0m
    %4 = const.int 0
    %5 = igt %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %5, %6[2m ; ops [%5:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb3, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb3 match.arm.0.body:
    %8 = fn_ref $Math_ilog10_aux.Int.Int.Int
    %9 = const.int 1
    %10 = iadd %0, %9[2m ; ops [%0:value/borrow#0, %9:value/borrow#1][0m
    tail_call %8(%10, %3)[2m ; ops [%8:callee/borrow#0, %10:value/consume#0, %3:value/consume#1][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn $Arrays_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $Arrays.print_arr
    %1 = fn_ref $Arrays.fold
    %2 = fn_ref $Arrays.foldi__
    %3 = fn_ref $Arrays.foldi
    %4 = fn_ref $Arrays.map
    %5 = fn_ref $Arrays.map2
    %6 = fn_ref $Arrays.mapi
    %7 = fn_ref $Arrays.filter
    %8 = fn_ref $Arrays.last
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn Arrays.print_arr(%0 a[2m: Array of `1733 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %1 = const.string "[|"
    %2 = print %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m
    %3 = flush [2m : ()[0m
    %4 = const.int 0
    %5 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %6 = phi [bb0: %4, bb3: %17][2m : Int[0m[2m ; ops [%4:value/borrow#0, %17:value/borrow#1][0m
    %7 = ult %6, %5[2m ; ops [%6:value/borrow#0, %5:value/borrow#1][0m
    cond %7, bb2, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb2 loop.body:
    %8 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %9 = ptr_offset %8, %6[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %6:index/borrow#1][0m
    %10 = load %9[2m : `1733[0m[2m ; ops [%9:value/borrow#0][0m
    %11 = str %10[2m : String[0m[2m ; ea stack#0[0m[2m ; ops [%10:value/borrow#0][0m
    %12 = print %11[2m : ()[0m[2m ; ops [%11:value/borrow#0][0m
    %13 = const.string ", "
    %14 = print %13[2m : ()[0m[2m ; ops [%13:value/borrow#0][0m
    %15 = flush [2m : ()[0m
    br bb3
  bb3 loop.inc:
    %16 = const.int 1
    %17 = iadd %6, %16[2m ; ops [%6:value/borrow#0, %16:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %18 = const.void
    %19 = const.string "|]\n"
    %20 = print %19[2m : ()[0m[2m ; ops [%19:value/borrow#0][0m
    %21 = flush [2m : ()[0m
    return %21[2m ; ops [%21:return/consume#0][0m
}
fn Arrays.fold(%0 f[2m: (`1741 -> (`1740 -> `1741)) @consume[0m, %1 res[2m: `1741 @consume[0m, %2 a[2m: Array of `1740 @borrow[0m)[2m -> `1741 @owned[0m {
  bb0 entry:
    %3 = extract.field %2, 0[2m : Int[0m[2m ; ops [%2:container/borrow#0][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %4 = const.int 0
    %5 = ieq %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    cond %5, bb3, bb4[2m ; ops [%5:condition/borrow#0][0m
  bb3 match.arm.0.body:
    return %1[2m ; ops [%1:return/consume#0][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    %6 = fn_ref $Arrays.fold
    %7 = const.int 0
    %8 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %9 = ptr_offset %8, %7[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %7:index/borrow#1][0m
    %10 = load %9[2m : `1740[0m[2m ; ops [%9:value/borrow#0][0m
[35m    %22 = dup %10[2m : ()[0m[2m ; ops [%10:value/borrow#0][0m[0m
    %11 = call %0(%1, %10)[2m : `1741[0m[2m ; ops [%0:callee/borrow#0, %1:value/consume#0, %10:value/consume#1][0m
    %12 = extract.field %2, 0[2m : Int[0m[2m ; ops [%2:container/borrow#0][0m
    %13 = const.int 0
    %14 = igt %12, %13[2m ; ops [%12:value/borrow#0, %13:value/borrow#1][0m
    %15 = primitive_cast %14 from Bool to Int[2m ; ops [%14:value/borrow#0][0m
    %16 = isub %12, %15[2m ; ops [%12:value/borrow#0, %15:value/borrow#1][0m
    %17 = extract.field %2, 1[2m : Int[0m[2m ; ops [%2:container/borrow#0][0m
    %18 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %19 = ptr_offset %18, %15[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %15:index/borrow#1][0m
    %20 = iadd %17, %15[2m ; ops [%17:value/borrow#0, %15:value/borrow#1][0m
    %21 = construct.tuple { %16, %20, %19 }[2m : Array of `1740[0m[2m ; ops [%16:field/borrow#0, %20:field/borrow#1, %19:field/borrow#2][0m
[35m    %23 = dup %21[2m : ()[0m[2m ; ops [%21:value/borrow#0][0m[0m
    tail_call %6(%0, %11, %21)[2m ; ops [%6:callee/borrow#0, %0:value/consume#0, %11:value/consume#1, %21:value/consume#2][0m
}
fn Arrays.foldi__(%0 idx[2m: `1756 @consume[0m, %1 f[2m: (`1756 -> (`1755 -> (`1754 -> `1755))) @consume[0m, %2 res[2m: `1755 @consume[0m, %3 a[2m: Array of `1754 @borrow[0m)[2m -> `1755 @owned[0m {
  bb0 entry:
    %4 = extract.field %3, 0[2m : Int[0m[2m ; ops [%3:container/borrow#0][0m
    br bb2
  bb1 match.no_match:
    unreachable
  bb2 match.arm.0.test:
    %5 = const.int 0
    %6 = ieq %4, %5[2m ; ops [%4:value/borrow#0, %5:value/borrow#1][0m
    cond %6, bb3, bb4[2m ; ops [%6:condition/borrow#0][0m
  bb3 match.arm.0.body:
    return %2[2m ; ops [%2:return/consume#0][0m
  bb4 match.arm.1.test:
    br bb5
  bb5 match.arm.1.body:
    %7 = fn_ref $Arrays.foldi__
    %8 = const.int 1
    %9 = call @+(%0, %8)[2m : `1756[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %8:value/borrow#1][0m
    %10 = const.int 0
    %11 = extract.field %3, 2[2m : Ptr[0m[2m ; ops [%3:container/borrow#0][0m
    %12 = ptr_offset %11, %10[2m : Ptr[0m[2m ; ops [%11:value/borrow#0, %10:index/borrow#1][0m
    %13 = load %12[2m : `1754[0m[2m ; ops [%12:value/borrow#0][0m
[35m    %25 = dup %13[2m : ()[0m[2m ; ops [%13:value/borrow#0][0m[0m
    %14 = call %1(%0, %2, %13)[2m : `1755[0m[2m ; ops [%1:callee/borrow#0, %0:value/consume#0, %2:value/consume#1, %13:value/consume#2][0m
    %15 = extract.field %3, 0[2m : Int[0m[2m ; ops [%3:container/borrow#0][0m
    %16 = const.int 0
    %17 = igt %15, %16[2m ; ops [%15:value/borrow#0, %16:value/borrow#1][0m
    %18 = primitive_cast %17 from Bool to Int[2m ; ops [%17:value/borrow#0][0m
    %19 = isub %15, %18[2m ; ops [%15:value/borrow#0, %18:value/borrow#1][0m
    %20 = extract.field %3, 1[2m : Int[0m[2m ; ops [%3:container/borrow#0][0m
    %21 = extract.field %3, 2[2m : Ptr[0m[2m ; ops [%3:container/borrow#0][0m
    %22 = ptr_offset %21, %18[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %18:index/borrow#1][0m
    %23 = iadd %20, %18[2m ; ops [%20:value/borrow#0, %18:value/borrow#1][0m
    %24 = construct.tuple { %19, %23, %22 }[2m : Array of `1754[0m[2m ; ops [%19:field/borrow#0, %23:field/borrow#1, %22:field/borrow#2][0m
[35m    %26 = dup %24[2m : ()[0m[2m ; ops [%24:value/borrow#0][0m[0m
    tail_call %7(%9, %1, %14, %24)[2m ; ops [%7:callee/borrow#0, %9:value/consume#0, %1:value/consume#1, %14:value/consume#2, %24:value/consume#3][0m
}
fn Arrays.foldi(%0 f[2m: (Int -> (`1773 -> (`1774 -> `1773))) @consume[0m, %1 res[2m: `1773 @consume[0m, %2 a[2m: Array of `1774 @borrow[0m)[2m -> `1773 @owned[0m {
  bb0 entry:
    %3 = fn_ref $Arrays.foldi__
    %4 = const.int 0
    %5 = call %3(%4, %0, %1, %2)[2m : `1773[0m[2m ; ops [%3:callee/borrow#0, %4:value/consume#0, %0:value/consume#1, %1:value/consume#2, %2:value/borrow#3][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn Arrays.map(%0 f[2m: (`1782 -> `1784) @borrow[0m, %1 a[2m: Array of `1782 @borrow[0m)[2m -> Array of `1784 @owned[0m {
  bb0 entry:
    %2 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    %3 = construct.array_fill_zeroes %2[2m : Array of `1784[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = const.int 0
    %5 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %6 = phi [bb0: %4, bb3: %16][2m : Int[0m[2m ; ops [%4:value/borrow#0, %16:value/borrow#1][0m
    %7 = ult %6, %5[2m ; ops [%6:value/borrow#0, %5:value/borrow#1][0m
    cond %7, bb2, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb2 loop.body:
    %8 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %9 = ptr_offset %8, %6[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %6:index/borrow#1][0m
    %10 = load %9[2m : `1782[0m[2m ; ops [%9:value/borrow#0][0m
[35m    %18 = dup %10[2m : ()[0m[2m ; ops [%10:value/borrow#0][0m[0m
    %11 = call %0(%10)[2m : `1784[0m[2m ; ops [%0:callee/borrow#0, %10:value/consume#0][0m
    %12 = extract.field %3, 2[2m : Ptr[0m[2m ; ops [%3:container/borrow#0][0m
    %13 = ptr_offset %12, %6[2m : Ptr[0m[2m ; ops [%12:value/borrow#0, %6:index/borrow#1][0m
    %14 = store %13, %11[2m : ()[0m[2m ; ops [%13:value/borrow#0, %11:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %15 = const.int 1
    %16 = iadd %6, %15[2m ; ops [%6:value/borrow#0, %15:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %17 = const.void
    return %3[2m ; ops [%3:return/consume#0][0m
}
fn Arrays.map2(%0 f[2m: (`1795 -> (`1793 -> `1796)) @borrow[0m, %1 xs[2m: Array of `1795 @borrow[0m, %2 ys[2m: Array of `1793 @borrow[0m)[2m -> Array of `1796 @owned[0m {
  bb0 entry:
    %3 = const.int 0
    %4 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : `1795[0m[2m ; ops [%5:value/borrow#0][0m
    %7 = const.int 0
    %8 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %9 = ptr_offset %8, %7[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %7:index/borrow#1][0m
    %10 = load %9[2m : `1793[0m[2m ; ops [%9:value/borrow#0][0m
[35m    %31 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
[35m    %32 = dup %10[2m : ()[0m[2m ; ops [%10:value/borrow#0][0m[0m
    %11 = call %0(%6, %10)[2m : `1796[0m[2m ; ops [%0:callee/borrow#0, %6:value/consume#0, %10:value/consume#1][0m
    %12 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    %13 = construct.array_fill_const %12, %11[2m : Array of `1796[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%12:value/borrow#0, %11:element/consume#1][0m
    %14 = const.int 1
    %15 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %16 = phi [bb0: %14, bb3: %29][2m : Int[0m[2m ; ops [%14:value/borrow#0, %29:value/borrow#1][0m
    %17 = ult %16, %15[2m ; ops [%16:value/borrow#0, %15:value/borrow#1][0m
    cond %17, bb2, bb4[2m ; ops [%17:condition/borrow#0][0m
  bb2 loop.body:
    %18 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %19 = ptr_offset %18, %16[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %16:index/borrow#1][0m
    %20 = load %19[2m : `1795[0m[2m ; ops [%19:value/borrow#0][0m
    %21 = extract.field %2, 2[2m : Ptr[0m[2m ; ops [%2:container/borrow#0][0m
    %22 = ptr_offset %21, %16[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %16:index/borrow#1][0m
    %23 = load %22[2m : `1793[0m[2m ; ops [%22:value/borrow#0][0m
[35m    %33 = dup %20[2m : ()[0m[2m ; ops [%20:value/borrow#0][0m[0m
[35m    %34 = dup %23[2m : ()[0m[2m ; ops [%23:value/borrow#0][0m[0m
    %24 = call %0(%20, %23)[2m : `1796[0m[2m ; ops [%0:callee/borrow#0, %20:value/consume#0, %23:value/consume#1][0m
    %25 = extract.field %13, 2[2m : Ptr[0m[2m ; ops [%13:container/borrow#0][0m
    %26 = ptr_offset %25, %16[2m : Ptr[0m[2m ; ops [%25:value/borrow#0, %16:index/borrow#1][0m
    %27 = store %26, %24[2m : ()[0m[2m ; ops [%26:value/borrow#0, %24:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %28 = const.int 1
    %29 = iadd %16, %28[2m ; ops [%16:value/borrow#0, %28:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %30 = const.void
    return %13[2m ; ops [%13:return/consume#0][0m
}
fn Arrays.mapi(%0 f[2m: (Int -> (`1807 -> `1809)) @borrow[0m, %1 a[2m: Array of `1807 @borrow[0m)[2m -> Array of `1809 @owned[0m {
  bb0 entry:
    %2 = const.int 0
    %3 = const.int 0
    %4 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : `1807[0m[2m ; ops [%5:value/borrow#0][0m
[35m    %24 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
    %7 = call %0(%2, %6)[2m : `1809[0m[2m ; ops [%0:callee/borrow#0, %2:value/consume#0, %6:value/consume#1][0m
    %8 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    %9 = construct.array_fill_const %8, %7[2m : Array of `1809[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%8:value/borrow#0, %7:element/consume#1][0m
    %10 = const.int 1
    %11 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %12 = phi [bb0: %10, bb3: %22][2m : Int[0m[2m ; ops [%10:value/borrow#0, %22:value/borrow#1][0m
    %13 = ult %12, %11[2m ; ops [%12:value/borrow#0, %11:value/borrow#1][0m
    cond %13, bb2, bb4[2m ; ops [%13:condition/borrow#0][0m
  bb2 loop.body:
    %14 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %15 = ptr_offset %14, %12[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %12:index/borrow#1][0m
    %16 = load %15[2m : `1807[0m[2m ; ops [%15:value/borrow#0][0m
[35m    %25 = dup %16[2m : ()[0m[2m ; ops [%16:value/borrow#0][0m[0m
    %17 = call %0(%12, %16)[2m : `1809[0m[2m ; ops [%0:callee/borrow#0, %12:value/consume#0, %16:value/consume#1][0m
    %18 = extract.field %9, 2[2m : Ptr[0m[2m ; ops [%9:container/borrow#0][0m
    %19 = ptr_offset %18, %12[2m : Ptr[0m[2m ; ops [%18:value/borrow#0, %12:index/borrow#1][0m
    %20 = store %19, %17[2m : ()[0m[2m ; ops [%19:value/borrow#0, %17:element/consume#1][0m
    br bb3
  bb3 loop.inc:
    %21 = const.int 1
    %22 = iadd %12, %21[2m ; ops [%12:value/borrow#0, %21:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %23 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
}
fn Arrays.filter(%0 f[2m: (`1843 -> Bool) @borrow[0m, %1 a[2m: Array of `1843 @borrow[0m)[2m -> Array of `1843 @owned[0m {
  bb0 entry:
    %2 = const.int 0
    %3 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %4 = ptr_offset %3, %2[2m : Ptr[0m[2m ; ops [%3:value/borrow#0, %2:index/borrow#1][0m
    %5 = load %4[2m : `1843[0m[2m ; ops [%4:value/borrow#0][0m
    %6 = construct.list_empty [2m : List
  | []
  | :: of `1843, List
[0m
[35m    %64 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %7 = construct.list_cons %5, %6[2m : List
  | []
  | :: of `1843, List
[0m[2m ; ea heap#0[0m[2m ; ops [%5:element/consume#0, %6:container/consume#1][0m
    %8 = construct.array_literal { %7 }[2m : Array of List
  | []
  | :: of `1843, List
[0m[2m ; ea heap#1 mutable[0m[2m ; ops [%7:element/consume#0][0m
    %9 = const.int 0
    %10 = construct.array_literal { %9 }[2m : Array of Int[0m[2m ; ea stack#2 mutable[0m[2m ; ops [%9:element/consume#0][0m
    %11 = const.int 0
    %12 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %13 = phi [bb0: %11, bb3: %45][2m : Int[0m[2m ; ops [%11:value/borrow#0, %45:value/borrow#1][0m
    %14 = ult %13, %12[2m ; ops [%13:value/borrow#0, %12:value/borrow#1][0m
    cond %14, bb2, bb4[2m ; ops [%14:condition/borrow#0][0m
  bb2 loop.body:
    %15 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %16 = ptr_offset %15, %13[2m : Ptr[0m[2m ; ops [%15:value/borrow#0, %13:index/borrow#1][0m
    %17 = load %16[2m : `1843[0m[2m ; ops [%16:value/borrow#0][0m
[35m    %65 = dup %17[2m : ()[0m[2m ; ops [%17:value/borrow#0][0m[0m
    %18 = call %0(%17)[2m : Bool[0m[2m ; ops [%0:callee/borrow#0, %17:value/consume#0][0m
    br bb7
  bb3 loop.inc:
    %44 = const.int 1
    %45 = iadd %13, %44[2m ; ops [%13:value/borrow#0, %44:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %46 = const.void
    %47 = const.int 0
    %48 = extract.field %10, 2[2m : Ptr[0m[2m ; ops [%10:container/borrow#0][0m
    %49 = ptr_offset %48, %47[2m : Ptr[0m[2m ; ops [%48:value/borrow#0, %47:index/borrow#1][0m
    %50 = load %49[2m : Int[0m[2m ; ops [%49:value/borrow#0][0m
[35m    %66 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %51 = construct.array_fill_const %50, %5[2m : Array of `1843[0m[2m ; ea heap#3[0m[2m ; ops [%50:value/borrow#0, %5:element/consume#1][0m
    %52 = fn_ref $Arrays.filter.list_to_arr
    %53 = const.int 0
    %54 = extract.field %8, 2[2m : Ptr[0m[2m ; ops [%8:container/borrow#0][0m
    %55 = ptr_offset %54, %53[2m : Ptr[0m[2m ; ops [%54:value/borrow#0, %53:index/borrow#1][0m
    %56 = load %55[2m : List
  | []
  | :: of `1843, List
[0m[2m ; ops [%55:value/borrow#0][0m
    %57 = const.int 0
    %58 = extract.field %10, 2[2m : Ptr[0m[2m ; ops [%10:container/borrow#0][0m
    %59 = ptr_offset %58, %57[2m : Ptr[0m[2m ; ops [%58:value/borrow#0, %57:index/borrow#1][0m
    %60 = load %59[2m : Int[0m[2m ; ops [%59:value/borrow#0][0m
    %61 = const.int 1
    %62 = isub %60, %61[2m ; ops [%60:value/borrow#0, %61:value/borrow#1][0m
[35m    %67 = dup %56[2m : ()[0m[2m ; ops [%56:value/borrow#0][0m[0m
    %63 = call %52(%56, %51, %62)[2m : Array of `1843[0m[2m ; ops [%52:callee/borrow#0, %56:value/consume#0, %51:value/consume#1, %62:value/borrow#2][0m
[35m    %68 = drop %8[2m : ()[0m[2m ; ops [%8:value/borrow#0][0m[0m
    return %63[2m ; ops [%63:return/consume#0][0m
  bb5 match.no_match:
    br bb6
  bb6 match.cont:
    %43 = const.void
    br bb3
  bb7 match.arm.0.test:
    %19 = const.bool true
    %20 = beq %18, %19[2m ; ops [%18:value/borrow#0, %19:value/borrow#1][0m
    cond %20, bb8, bb5[2m ; ops [%20:condition/borrow#0][0m
  bb8 match.arm.0.body:
    %21 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %22 = ptr_offset %21, %13[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %13:index/borrow#1][0m
    %23 = load %22[2m : `1843[0m[2m ; ops [%22:value/borrow#0][0m
    %24 = const.int 0
    %25 = extract.field %8, 2[2m : Ptr[0m[2m ; ops [%8:container/borrow#0][0m
    %26 = ptr_offset %25, %24[2m : Ptr[0m[2m ; ops [%25:value/borrow#0, %24:index/borrow#1][0m
    %27 = load %26[2m : List
  | []
  | :: of `1843, List
[0m[2m ; ops [%26:value/borrow#0][0m
[35m    %69 = dup %23[2m : ()[0m[2m ; ops [%23:value/borrow#0][0m[0m
[35m    %70 = dup %27[2m : ()[0m[2m ; ops [%27:value/borrow#0][0m[0m
[35m    %73 = drop-reuse %71[2m : Ptr[0m[2m ; ops [%71:value/borrow#0][0m[0m
    %28 = construct.list_cons @ru %73, %23, %27[2m : List
  | []
  | :: of `1843, List
[0m[2m ; ea heap#4[0m[2m ; ops [%23:element/consume#0, %27:container/consume#1, %73:value/borrow#2][0m
    %29 = const.int 0
    %30 = extract.field %8, 2[2m : Ptr[0m[2m ; ops [%8:container/borrow#0][0m
    %31 = ptr_offset %30, %29[2m : Ptr[0m[2m ; ops [%30:value/borrow#0, %29:index/borrow#1][0m
    %71 = load_owned %31[2m : List
  | []
  | :: of `1843, List
[0m[2m ; ops [%31:value/borrow#0][0m
    %32 = store %31, %28[2m : ()[0m[2m ; ops [%31:value/borrow#0, %28:element/consume#1][0m
    %33 = const.int 1
    %34 = const.int 0
    %35 = extract.field %10, 2[2m : Ptr[0m[2m ; ops [%10:container/borrow#0][0m
    %36 = ptr_offset %35, %34[2m : Ptr[0m[2m ; ops [%35:value/borrow#0, %34:index/borrow#1][0m
    %37 = load %36[2m : Int[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = iadd %33, %37[2m ; ops [%33:value/borrow#0, %37:value/borrow#1][0m
    %39 = const.int 0
    %40 = extract.field %10, 2[2m : Ptr[0m[2m ; ops [%10:container/borrow#0][0m
    %41 = ptr_offset %40, %39[2m : Ptr[0m[2m ; ops [%40:value/borrow#0, %39:index/borrow#1][0m
    %42 = store %41, %38[2m : ()[0m[2m ; ops [%41:value/borrow#0, %38:element/consume#1][0m
    br bb6
}
fn Arrays.filter.list_to_arr(%0 l[2m: List
  | []
  | :: of `1836, List
 @consume[0m, %1 res[2m: Array of `1836 @consume[0m, %2 i[2m: Int @borrow[0m)[2m -> Array of `1836 @owned[0m {
  bb0 entry:
[35m    %5 = drop %0[2m : ()[0m[2m ; ops [%0:value/borrow#0][0m[0m
[35m    %6 = drop %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m[0m
    br bb2
  bb1 match.no_match:
    <no terminator>
  bb2 match.arm.0.test:
    %3 = const.int 0
    %4 = ieq %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb3, bb4[2m ; ops [%4:condition/borrow#0][0m
  bb3 match.arm.0.body:
    unreachable
  bb4 match.arm.1.test:
    <no terminator>
}
fn Arrays.last(%0 x[2m: Array of `1852 @borrow[0m)[2m -> `1852 @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = const.int 1
    %3 = isub %1, %2[2m ; ops [%1:value/borrow#0, %2:value/borrow#1][0m
    %4 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : `1852[0m[2m ; ops [%5:value/borrow#0][0m
[35m    %7 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn $SF_module_init()[2m -> () @none[0m {
  bb0 entry:
    %0 = fn_ref $sf_open_opt
    %1 = fn_ref $sf_close
    %2 = fn_ref $u64_to_int
    %3 = fn_ref $sf_read_double
    %4 = fn_ref $SF.load_sf
    %5 = fn_ref $SF.load_sf_mono
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
}
extern fn sf_open_opt(%0 arg0[2m: String @borrow[0m)[2m -> Option
  | Some of (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )
  | None
 @owned[0m;
extern fn sf_close(%0 arg0[2m: Ptr @borrow[0m)[2m -> Int @owned[0m;
extern fn u64_to_int(%0 arg0[2m: Uint64 @borrow[0m)[2m -> Int @owned[0m;
extern fn sf_read_double(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Ptr @borrow[0m, %2 arg2[2m: Uint64 @borrow[0m)[2m -> Uint64 @owned[0m;
fn SF.load_sf(%0 path[2m: String @borrow[0m)[2m -> (data: Array of Double, channels: Int, samplerate: Int, ) @owned[0m {
  bb0 entry:
    %1 = fn_ref $sf_open_opt
    %2 = call %1(%0)[2m : Option
  | Some of (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )
  | None
[0m[2m ; ops [%1:callee/borrow#0, %0:value/borrow#0][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %29 = phi [bb4: %22, bb10: %28][2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%22:value/borrow#0, %28:value/borrow#1][0m
    return %29[2m ; ops [%29:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb6, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %11 = primitive_cast %10 from Int to Uint64[2m ; ops [%10:value/borrow#0][0m
    %12 = umul %8, %11[2m ; ops [%8:value/borrow#0, %11:value/borrow#1][0m
    %13 = const.double 0
    %14 = fn_ref $u64_to_int
    %15 = call %14(%12)[2m : Int[0m[2m ; ops [%14:callee/borrow#0, %12:value/borrow#0][0m
    %16 = construct.array_fill_const %15, %13[2m : Array of Double[0m[2m ; ea heap#0[0m[2m ; ops [%15:value/borrow#0, %13:element/consume#1][0m
    %17 = fn_ref $sf_read_double
    %18 = cstr %16[2m : Ptr[0m[2m ; ops [%16:value/borrow#0][0m
    %19 = call %17(%7, %18, %12)[2m : Uint64[0m[2m ; ops [%17:callee/borrow#0, %7:value/borrow#0, %18:value/borrow#1, %12:value/borrow#2][0m
    %20 = fn_ref $sf_close
    %21 = call %20(%7)[2m : Int[0m[2m ; ops [%20:callee/borrow#0, %7:value/borrow#0][0m
    %22 = construct.tuple { data: %16, channels: %10, samplerate: %9 }[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%16:field/consume#0, %10:field/consume#1, %9:field/consume#2][0m
    br bb2
  bb5 match.arm.1.test:
    %23 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %24 = tag_eq %23, None#1[2m ; ops [%23:tag/borrow#0][0m
    cond %24, bb10, bb1[2m ; ops [%24:condition/borrow#0][0m
  bb6 match.payload.bb3:
    %5 = extract.variant_payload %2, Some#0[2m : Some of (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = extract.field %6, 0[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    br bb7
  bb7 match.tuple.6.1:
    %8 = extract.field %6, 1[2m : Uint64[0m[2m ; ops [%6:container/borrow#0][0m
    br bb8
  bb8 match.tuple.7.2:
    %9 = extract.field %6, 2[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    br bb9
  bb9 match.tuple.8.3:
    %10 = extract.field %6, 3[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    br bb4
  bb10 match.arm.1.body:
    %25 = construct.array_literal {  }[2m : Array of Double[0m[2m ; ea heap#1[0m
    %26 = const.int 0
    %27 = const.int 0
    %28 = construct.tuple { data: %25, channels: %26, samplerate: %27 }[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%25:field/consume#0, %26:field/consume#1, %27:field/consume#2][0m
    br bb2
}
fn SF.load_sf_mono(%0 path[2m: String @borrow[0m)[2m -> (data: Array of Double, channels: Int, samplerate: Int, ) @owned[0m {
  bb0 entry:
    %1 = fn_ref $sf_open_opt
    %2 = call %1(%0)[2m : Option
  | Some of (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )
  | None
[0m[2m ; ops [%1:callee/borrow#0, %0:value/borrow#0][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %60 = phi [bb13: %53, bb14: %59][2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%53:value/borrow#0, %59:value/borrow#1][0m
    return %60[2m ; ops [%60:return/consume#0][0m
  bb3 match.arm.0.test:
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb6, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %11 = primitive_cast %10 from Int to Uint64[2m ; ops [%10:value/borrow#0][0m
    %12 = umul %8, %11[2m ; ops [%8:value/borrow#0, %11:value/borrow#1][0m
    %13 = const.double 0
    %14 = fn_ref $u64_to_int
    %15 = call %14(%12)[2m : Int[0m[2m ; ops [%14:callee/borrow#0, %12:value/borrow#0][0m
    %16 = construct.array_fill_const %15, %13[2m : Array of Double[0m[2m ; ea stack#0[0m[2m ; ops [%15:value/borrow#0, %13:element/consume#1][0m
    %17 = fn_ref $sf_read_double
    %18 = cstr %16[2m : Ptr[0m[2m ; ops [%16:value/borrow#0][0m
    %19 = call %17(%7, %18, %12)[2m : Uint64[0m[2m ; ops [%17:callee/borrow#0, %7:value/borrow#0, %18:value/borrow#1, %12:value/borrow#2][0m
    %20 = fn_ref $u64_to_int
    %21 = call %20(%8)[2m : Int[0m[2m ; ops [%20:callee/borrow#0, %8:value/borrow#0][0m
    %22 = const.double 0
    %23 = construct.array_fill_const %21, %22[2m : Array of Double[0m[2m ; ea heap#1 mutable[0m[2m ; ops [%21:value/borrow#0, %22:element/consume#1][0m
    %24 = const.int 0
    br bb10
  bb5 match.arm.1.test:
    %54 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %55 = tag_eq %54, None#1[2m ; ops [%54:tag/borrow#0][0m
    cond %55, bb14, bb1[2m ; ops [%55:condition/borrow#0][0m
  bb6 match.payload.bb3:
    %5 = extract.variant_payload %2, Some#0[2m : Some of (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : (fd: Ptr, frames: Uint64, samplerate: Int, channels: Int, )[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = extract.field %6, 0[2m : Ptr[0m[2m ; ops [%6:container/borrow#0][0m
    br bb7
  bb7 match.tuple.6.1:
    %8 = extract.field %6, 1[2m : Uint64[0m[2m ; ops [%6:container/borrow#0][0m
    br bb8
  bb8 match.tuple.7.2:
    %9 = extract.field %6, 2[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    br bb9
  bb9 match.tuple.8.3:
    %10 = extract.field %6, 3[2m : Int[0m[2m ; ops [%6:container/borrow#0][0m
    br bb4
  bb10 loop.cond:
    %25 = phi [bb4: %24, bb12: %48][2m : Int[0m[2m ; ops [%24:value/borrow#0, %48:value/borrow#1][0m
    %26 = ult %25, %21[2m ; ops [%25:value/borrow#0, %21:value/borrow#1][0m
    cond %26, bb11, bb13[2m ; ops [%26:condition/borrow#0][0m
  bb11 loop.body:
    %27 = const.double 0.5
    %28 = const.int 2
    %29 = imul %25, %28[2m ; ops [%25:value/borrow#0, %28:value/borrow#1][0m
    %30 = const.int 0
    %31 = iadd %29, %30[2m ; ops [%29:value/borrow#0, %30:value/borrow#1][0m
    %32 = extract.field %16, 2[2m : Ptr[0m[2m ; ops [%16:container/borrow#0][0m
    %33 = ptr_offset %32, %31[2m : Ptr[0m[2m ; ops [%32:value/borrow#0, %31:index/borrow#1][0m
    %34 = load %33[2m : Double[0m[2m ; ops [%33:value/borrow#0][0m
    %35 = const.int 2
    %36 = imul %25, %35[2m ; ops [%25:value/borrow#0, %35:value/borrow#1][0m
    %37 = const.int 1
    %38 = iadd %36, %37[2m ; ops [%36:value/borrow#0, %37:value/borrow#1][0m
    %39 = extract.field %16, 2[2m : Ptr[0m[2m ; ops [%16:container/borrow#0][0m
    %40 = ptr_offset %39, %38[2m : Ptr[0m[2m ; ops [%39:value/borrow#0, %38:index/borrow#1][0m
    %41 = load %40[2m : Double[0m[2m ; ops [%40:value/borrow#0][0m
    %42 = fadd %34, %41[2m ; ops [%34:value/borrow#0, %41:value/borrow#1][0m
    %43 = fmul %27, %42[2m ; ops [%27:value/borrow#0, %42:value/borrow#1][0m
    %44 = extract.field %23, 2[2m : Ptr[0m[2m ; ops [%23:container/borrow#0][0m
    %45 = ptr_offset %44, %25[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %25:index/borrow#1][0m
    %46 = store %45, %43[2m : ()[0m[2m ; ops [%45:value/borrow#0, %43:element/consume#1][0m
    br bb12
  bb12 loop.inc:
    %47 = const.int 1
    %48 = iadd %25, %47[2m ; ops [%25:value/borrow#0, %47:value/borrow#1][0m
    br bb10
  bb13 loop.after:
    %49 = const.void
    %50 = fn_ref $sf_close
    %51 = call %50(%7)[2m : Int[0m[2m ; ops [%50:callee/borrow#0, %7:value/borrow#0][0m
    %52 = const.int 1
    %53 = construct.tuple { data: %23, channels: %52, samplerate: %9 }[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%23:field/consume#0, %52:field/consume#1, %9:field/consume#2][0m
    br bb2
  bb14 match.arm.1.body:
    %56 = construct.array_literal {  }[2m : Array of Double[0m[2m ; ea heap#2[0m
    %57 = const.int 0
    %58 = const.int 0
    %59 = construct.tuple { data: %56, channels: %57, samplerate: %58 }[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%56:field/consume#0, %57:field/consume#1, %58:field/consume#2][0m
    br bb2
}
fn Arrays_map.Fn_String_Char_SF_Array_Double_Int_Int.Array_String_Char.Array_SF_Array_Double_Int_Int(%0 f[2m: (String -> (data: Array of Double, channels: Int, samplerate: Int, )) @borrow[0m, %1 a[2m: Array of String @borrow[0m)[2m -> Array of (data: Array of Double, channels: Int, samplerate: Int, ) @owned[0m {
  bb0 entry:
    %2 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    %3 = construct.array_fill_zeroes %2[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ea heap#0 mutable[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = const.int 0
    %5 = extract.field %1, 0[2m : Int[0m[2m ; ops [%1:container/borrow#0][0m
    br bb1
  bb1 loop.cond:
    %6 = phi [bb0: %4, bb3: %16][2m : Int[0m[2m ; ops [%4:value/borrow#0, %16:value/borrow#1][0m
    %7 = ult %6, %5[2m ; ops [%6:value/borrow#0, %5:value/borrow#1][0m
    cond %7, bb2, bb4[2m ; ops [%7:condition/borrow#0][0m
  bb2 loop.body:
    %8 = extract.field %1, 2[2m : Ptr[0m[2m ; ops [%1:container/borrow#0][0m
    %9 = ptr_offset %8, %6[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %6:index/borrow#1][0m
    %10 = load %9[2m : String[0m[2m ; ops [%9:value/borrow#0][0m
[35m    %18 = dup %10[2m : ()[0m[2m ; ops [%10:value/borrow#0][0m[0m
    %11 = call %0(%10)[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%0:callee/borrow#0, %10:value/consume#0][0m
    %12 = extract.field %3, 2[2m : Ptr[0m[2m ; ops [%3:container/borrow#0][0m
    %13 = ptr_offset %12, %6[2m : Ptr[0m[2m ; ops [%12:value/borrow#0, %6:index/borrow#1][0m
    %19 = load_owned %13[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%13:value/borrow#0][0m
    %14 = store %13, %11[2m : ()[0m[2m ; ops [%13:value/borrow#0, %11:element/consume#1][0m
[35m    %20 = drop %19[2m : ()[0m[2m ; ops [%19:value/borrow#0][0m[0m
    br bb3
  bb3 loop.inc:
    %15 = const.int 1
    %16 = iadd %6, %15[2m ; ops [%6:value/borrow#0, %15:value/borrow#1][0m
    br bb1
  bb4 loop.after:
    %17 = const.void
    return %3[2m ; ops [%3:return/consume#0][0m
}
fn kd.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $kd.frame
    %2 = const.string "kd"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 80
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $kd.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $kd.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn kd.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 56
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 48
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 24
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 8
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 0
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.void
    return %26[2m ; ops [%26:return/consume#0][0m
}
fn kd.kernel(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = const.double 1
    %5 = global_load @$global.bufs[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m
[35m    %57 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %6 = const.int 1
    %7 = extract.field %5, 1[2m : Int[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = iadd %7, %6[2m ; ops [%7:value/borrow#0, %6:value/borrow#1][0m
    %9 = extract.field %5, 2[2m : Ptr[0m[2m ; ops [%5:container/borrow#0][0m
    %10 = ptr_offset %9, %8[2m : Ptr[0m[2m ; ops [%9:value/borrow#0, %8:index/borrow#1][0m
    %11 = load %10[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%10:value/borrow#0][0m
    %12 = const.int 0
    %13 = primitive_cast %12 from Int to Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 0
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = fn_ref $ylc_audio_trig_once_kernel
    %18 = call %17(%16, %3, %13)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %16:value/borrow#0, %3:value/borrow#1, %13:value/borrow#2][0m
    %19 = fn_ref $db_amp
    %20 = const.double 12
    %21 = call %19(%20)[2m : Double[0m[2m ; ops [%19:callee/borrow#0, %20:value/borrow#0][0m
    %22 = const.double 0
    %23 = const.double 0
    %24 = const.double 1
    %25 = const.double 0.01
    %26 = const.double 0.1
    %27 = fn_ref $ylc_audio_rect_kernel
    %28 = const.int 8
    %29 = ptr_offset %1, %28[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %28:index/borrow#1][0m
    %30 = primitive_cast %29 from Ptr of Char to Ptr of Double[2m ; ops [%29:value/borrow#0][0m
    %31 = call %27(%30, %3, %26, %18)[2m : Double[0m[2m ; ops [%27:callee/borrow#0, %30:value/borrow#0, %3:value/borrow#1, %26:value/borrow#2, %18:value/borrow#3][0m
    %32 = fn_ref $ylc_audio_adsr_kernel
    %33 = const.int 24
    %34 = ptr_offset %1, %33[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %33:index/borrow#1][0m
    %35 = primitive_cast %34 from Ptr of Char to Ptr of Double[2m ; ops [%34:value/borrow#0][0m
    %36 = call %32(%35, %3, %22, %23, %24, %25, %31)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %35:value/borrow#0, %3:value/borrow#1, %22:value/borrow#2, %23:value/borrow#3, %24:value/borrow#4, %25:value/borrow#5, %31:value/borrow#6][0m
    %37 = fn_ref $ylc_audio_kill_on_end_kernel
    %38 = const.int 48
    %39 = ptr_offset %1, %38[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %38:index/borrow#1][0m
    %40 = primitive_cast %39 from Ptr of Char to Ptr of Double[2m ; ops [%39:value/borrow#0][0m
    %41 = call %37(%40, %3, %0, %36)[2m : Double[0m[2m ; ops [%37:callee/borrow#0, %40:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %36:value/borrow#3][0m
    %42 = extract.field %11, 0 ; data[2m : Array of Double[0m[2m ; ops [%11:container/borrow#0][0m
    %43 = extract.field %11, 2 ; samplerate[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %44 = primitive_cast %43 from Int to Double[2m ; ops [%43:value/borrow#0][0m
    %45 = fmul %44, %3[2m ; ops [%44:value/borrow#0, %3:value/borrow#1][0m
    %46 = const.double 0
    %47 = fn_ref $ylc_audio_bufplay_kernel
    %48 = extract.field %42, 0[2m : Int[0m[2m ; ops [%42:container/borrow#0][0m
    %49 = extract.field %42, 1[2m : Int[0m[2m ; ops [%42:container/borrow#0][0m
    %50 = extract.field %42, 2[2m : Ptr[0m[2m ; ops [%42:container/borrow#0][0m
    %51 = const.int 56
    %52 = ptr_offset %1, %51[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %51:index/borrow#1][0m
    %53 = primitive_cast %52 from Ptr of Char to Ptr of Double[2m ; ops [%52:value/borrow#0][0m
    %54 = call %47(%53, %3, %48, %49, %50, %45, %46, %18)[2m : Double[0m[2m ; ops [%47:callee/borrow#0, %53:value/borrow#0, %3:value/borrow#1, %48:value/borrow#2, %49:value/borrow#3, %50:value/borrow#4, %45:value/borrow#5, %46:value/borrow#6, %18:value/borrow#7][0m
[35m    %58 = drop %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %55 = fmul %41, %54[2m ; ops [%41:value/borrow#0, %54:value/borrow#1][0m
    %56 = fmul %21, %55[2m ; ops [%21:value/borrow#0, %55:value/borrow#1][0m
    return %56[2m ; ops [%56:return/consume#0][0m
}
fn kd.frame(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $kd.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
extern fn ylc_audio_rect_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_adsr_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_kill_on_end_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Ptr @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_bufplay_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Int @borrow[0m, %4 arg4[2m: Ptr @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m, %7 arg7[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn snr.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $snr.frame
    %2 = const.string "snr"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 80
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $snr.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $snr.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn snr.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 56
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 48
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 24
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 8
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 0
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.void
    return %26[2m ; ops [%26:return/consume#0][0m
}
fn snr.kernel(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = const.double 1
    %5 = global_load @$global.bufs[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m
[35m    %53 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %6 = const.int 2
    %7 = extract.field %5, 1[2m : Int[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = iadd %7, %6[2m ; ops [%7:value/borrow#0, %6:value/borrow#1][0m
    %9 = extract.field %5, 2[2m : Ptr[0m[2m ; ops [%5:container/borrow#0][0m
    %10 = ptr_offset %9, %8[2m : Ptr[0m[2m ; ops [%9:value/borrow#0, %8:index/borrow#1][0m
    %11 = load %10[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%10:value/borrow#0][0m
    %12 = const.int 0
    %13 = primitive_cast %12 from Int to Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 0
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = fn_ref $ylc_audio_trig_once_kernel
    %18 = call %17(%16, %3, %13)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %16:value/borrow#0, %3:value/borrow#1, %13:value/borrow#2][0m
    %19 = const.double 0
    %20 = const.double 0
    %21 = const.double 1
    %22 = const.double 0.01
    %23 = const.double 0.1
    %24 = fn_ref $ylc_audio_rect_kernel
    %25 = const.int 8
    %26 = ptr_offset %1, %25[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %25:index/borrow#1][0m
    %27 = primitive_cast %26 from Ptr of Char to Ptr of Double[2m ; ops [%26:value/borrow#0][0m
    %28 = call %24(%27, %3, %23, %18)[2m : Double[0m[2m ; ops [%24:callee/borrow#0, %27:value/borrow#0, %3:value/borrow#1, %23:value/borrow#2, %18:value/borrow#3][0m
    %29 = fn_ref $ylc_audio_adsr_kernel
    %30 = const.int 24
    %31 = ptr_offset %1, %30[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %30:index/borrow#1][0m
    %32 = primitive_cast %31 from Ptr of Char to Ptr of Double[2m ; ops [%31:value/borrow#0][0m
    %33 = call %29(%32, %3, %19, %20, %21, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %32:value/borrow#0, %3:value/borrow#1, %19:value/borrow#2, %20:value/borrow#3, %21:value/borrow#4, %22:value/borrow#5, %28:value/borrow#6][0m
    %34 = fn_ref $ylc_audio_kill_on_end_kernel
    %35 = const.int 48
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Double[2m ; ops [%36:value/borrow#0][0m
    %38 = call %34(%37, %3, %0, %33)[2m : Double[0m[2m ; ops [%34:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %33:value/borrow#3][0m
    %39 = extract.field %11, 0 ; data[2m : Array of Double[0m[2m ; ops [%11:container/borrow#0][0m
    %40 = extract.field %11, 2 ; samplerate[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %41 = primitive_cast %40 from Int to Double[2m ; ops [%40:value/borrow#0][0m
    %42 = fmul %41, %3[2m ; ops [%41:value/borrow#0, %3:value/borrow#1][0m
    %43 = const.double 0
    %44 = fn_ref $ylc_audio_bufplay_kernel
    %45 = extract.field %39, 0[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %46 = extract.field %39, 1[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %47 = extract.field %39, 2[2m : Ptr[0m[2m ; ops [%39:container/borrow#0][0m
    %48 = const.int 56
    %49 = ptr_offset %1, %48[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %48:index/borrow#1][0m
    %50 = primitive_cast %49 from Ptr of Char to Ptr of Double[2m ; ops [%49:value/borrow#0][0m
    %51 = call %44(%50, %3, %45, %46, %47, %42, %43, %18)[2m : Double[0m[2m ; ops [%44:callee/borrow#0, %50:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2, %46:value/borrow#3, %47:value/borrow#4, %42:value/borrow#5, %43:value/borrow#6, %18:value/borrow#7][0m
[35m    %54 = drop %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %52 = fmul %38, %51[2m ; ops [%38:value/borrow#0, %51:value/borrow#1][0m
    return %52[2m ; ops [%52:return/consume#0][0m
}
fn snr.frame(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $snr.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn hh.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $hh.frame
    %2 = const.string "hh"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 80
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $hh.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $hh.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn hh.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 56
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 48
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 24
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 8
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 0
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.void
    return %26[2m ; ops [%26:return/consume#0][0m
}
fn hh.kernel(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = const.double 1
    %5 = global_load @$global.bufs[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m
[35m    %53 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %6 = const.int 0
    %7 = extract.field %5, 1[2m : Int[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = iadd %7, %6[2m ; ops [%7:value/borrow#0, %6:value/borrow#1][0m
    %9 = extract.field %5, 2[2m : Ptr[0m[2m ; ops [%5:container/borrow#0][0m
    %10 = ptr_offset %9, %8[2m : Ptr[0m[2m ; ops [%9:value/borrow#0, %8:index/borrow#1][0m
    %11 = load %10[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%10:value/borrow#0][0m
    %12 = const.int 0
    %13 = primitive_cast %12 from Int to Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 0
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = fn_ref $ylc_audio_trig_once_kernel
    %18 = call %17(%16, %3, %13)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %16:value/borrow#0, %3:value/borrow#1, %13:value/borrow#2][0m
    %19 = const.double 0
    %20 = const.double 0
    %21 = const.double 1
    %22 = const.double 0.01
    %23 = const.double 0.05
    %24 = fn_ref $ylc_audio_rect_kernel
    %25 = const.int 8
    %26 = ptr_offset %1, %25[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %25:index/borrow#1][0m
    %27 = primitive_cast %26 from Ptr of Char to Ptr of Double[2m ; ops [%26:value/borrow#0][0m
    %28 = call %24(%27, %3, %23, %18)[2m : Double[0m[2m ; ops [%24:callee/borrow#0, %27:value/borrow#0, %3:value/borrow#1, %23:value/borrow#2, %18:value/borrow#3][0m
    %29 = fn_ref $ylc_audio_adsr_kernel
    %30 = const.int 24
    %31 = ptr_offset %1, %30[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %30:index/borrow#1][0m
    %32 = primitive_cast %31 from Ptr of Char to Ptr of Double[2m ; ops [%31:value/borrow#0][0m
    %33 = call %29(%32, %3, %19, %20, %21, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %32:value/borrow#0, %3:value/borrow#1, %19:value/borrow#2, %20:value/borrow#3, %21:value/borrow#4, %22:value/borrow#5, %28:value/borrow#6][0m
    %34 = fn_ref $ylc_audio_kill_on_end_kernel
    %35 = const.int 48
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Double[2m ; ops [%36:value/borrow#0][0m
    %38 = call %34(%37, %3, %0, %33)[2m : Double[0m[2m ; ops [%34:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %33:value/borrow#3][0m
    %39 = extract.field %11, 0 ; data[2m : Array of Double[0m[2m ; ops [%11:container/borrow#0][0m
    %40 = extract.field %11, 2 ; samplerate[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %41 = primitive_cast %40 from Int to Double[2m ; ops [%40:value/borrow#0][0m
    %42 = fmul %41, %3[2m ; ops [%41:value/borrow#0, %3:value/borrow#1][0m
    %43 = const.double 0
    %44 = fn_ref $ylc_audio_bufplay_kernel
    %45 = extract.field %39, 0[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %46 = extract.field %39, 1[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %47 = extract.field %39, 2[2m : Ptr[0m[2m ; ops [%39:container/borrow#0][0m
    %48 = const.int 56
    %49 = ptr_offset %1, %48[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %48:index/borrow#1][0m
    %50 = primitive_cast %49 from Ptr of Char to Ptr of Double[2m ; ops [%49:value/borrow#0][0m
    %51 = call %44(%50, %3, %45, %46, %47, %42, %43, %18)[2m : Double[0m[2m ; ops [%44:callee/borrow#0, %50:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2, %46:value/borrow#3, %47:value/borrow#4, %42:value/borrow#5, %43:value/borrow#6, %18:value/borrow#7][0m
[35m    %54 = drop %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %52 = fmul %38, %51[2m ; ops [%38:value/borrow#0, %51:value/borrow#1][0m
    return %52[2m ; ops [%52:return/consume#0][0m
}
fn hh.frame(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $hh.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn hh2.cons()[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %0 = fn_ref $ylc_create_audio_frame_node
    %1 = fn_ref $hh2.frame
    %2 = const.string "hh2"
    %3 = const.int 0
    %4 = const.int 1
    %5 = const.int 80
    %6 = call %0(%1, %3, %4, %5, %2)[2m : Ptr[0m[2m ; ops [%0:callee/borrow#0, %1:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2, %5:value/borrow#3, %2:value/borrow#4][0m
    %7 = fn_ref $ylc_audio_node_set_state_init
    %8 = fn_ref $hh2.init
    %9 = call %7(%6, %8)[2m : ()[0m[2m ; ops [%7:callee/borrow#0, %6:value/borrow#0, %8:value/borrow#1][0m
    %10 = fn_ref $ylc_audio_node_inline_state
    %11 = call %10(%6)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %6:value/borrow#0][0m
    %12 = fn_ref $hh2.init
    %13 = call %12(%11)[2m : ()[0m[2m ; ops [%12:callee/borrow#0, %11:value/borrow#0][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn hh2.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 56
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 48
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 24
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.double 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 8
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = primitive_cast %17 from Ptr of Char to Ptr of Double[2m ; ops [%17:value/borrow#0][0m
    %19 = const.double 0
    %20 = store %18, %19[2m : ()[0m[2m ; ops [%18:value/borrow#0, %19:element/consume#1][0m
    %21 = const.int 0
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.void
    return %26[2m ; ops [%26:return/consume#0][0m
}
fn hh2.kernel(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %4 = const.double 1.2
    %5 = global_load @$global.bufs[2m : Array of (data: Array of Double, channels: Int, samplerate: Int, )[0m
[35m    %54 = dup %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %6 = const.int 0
    %7 = extract.field %5, 1[2m : Int[0m[2m ; ops [%5:container/borrow#0][0m
    %8 = iadd %7, %6[2m ; ops [%7:value/borrow#0, %6:value/borrow#1][0m
    %9 = extract.field %5, 2[2m : Ptr[0m[2m ; ops [%5:container/borrow#0][0m
    %10 = ptr_offset %9, %8[2m : Ptr[0m[2m ; ops [%9:value/borrow#0, %8:index/borrow#1][0m
    %11 = load %10[2m : (data: Array of Double, channels: Int, samplerate: Int, )[0m[2m ; ops [%10:value/borrow#0][0m
    %12 = const.int 0
    %13 = primitive_cast %12 from Int to Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 0
    %15 = ptr_offset %1, %14[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = fn_ref $ylc_audio_trig_once_kernel
    %18 = call %17(%16, %3, %13)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %16:value/borrow#0, %3:value/borrow#1, %13:value/borrow#2][0m
    %19 = const.double 0
    %20 = const.double 0
    %21 = const.double 1
    %22 = const.double 0.01
    %23 = const.double 0.05
    %24 = fn_ref $ylc_audio_rect_kernel
    %25 = const.int 8
    %26 = ptr_offset %1, %25[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %25:index/borrow#1][0m
    %27 = primitive_cast %26 from Ptr of Char to Ptr of Double[2m ; ops [%26:value/borrow#0][0m
    %28 = call %24(%27, %3, %23, %18)[2m : Double[0m[2m ; ops [%24:callee/borrow#0, %27:value/borrow#0, %3:value/borrow#1, %23:value/borrow#2, %18:value/borrow#3][0m
    %29 = fn_ref $ylc_audio_adsr_kernel
    %30 = const.int 24
    %31 = ptr_offset %1, %30[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %30:index/borrow#1][0m
    %32 = primitive_cast %31 from Ptr of Char to Ptr of Double[2m ; ops [%31:value/borrow#0][0m
    %33 = call %29(%32, %3, %19, %20, %21, %22, %28)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %32:value/borrow#0, %3:value/borrow#1, %19:value/borrow#2, %20:value/borrow#3, %21:value/borrow#4, %22:value/borrow#5, %28:value/borrow#6][0m
    %34 = fn_ref $ylc_audio_kill_on_end_kernel
    %35 = const.int 48
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Double[2m ; ops [%36:value/borrow#0][0m
    %38 = call %34(%37, %3, %0, %33)[2m : Double[0m[2m ; ops [%34:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %33:value/borrow#3][0m
    %39 = extract.field %11, 0 ; data[2m : Array of Double[0m[2m ; ops [%11:container/borrow#0][0m
    %40 = extract.field %11, 2 ; samplerate[2m : Int[0m[2m ; ops [%11:container/borrow#0][0m
    %41 = primitive_cast %40 from Int to Double[2m ; ops [%40:value/borrow#0][0m
    %42 = fmul %4, %41[2m ; ops [%4:value/borrow#0, %41:value/borrow#1][0m
    %43 = fmul %42, %3[2m ; ops [%42:value/borrow#0, %3:value/borrow#1][0m
    %44 = const.double 0
    %45 = fn_ref $ylc_audio_bufplay_kernel
    %46 = extract.field %39, 0[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %47 = extract.field %39, 1[2m : Int[0m[2m ; ops [%39:container/borrow#0][0m
    %48 = extract.field %39, 2[2m : Ptr[0m[2m ; ops [%39:container/borrow#0][0m
    %49 = const.int 56
    %50 = ptr_offset %1, %49[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %49:index/borrow#1][0m
    %51 = primitive_cast %50 from Ptr of Char to Ptr of Double[2m ; ops [%50:value/borrow#0][0m
    %52 = call %45(%51, %3, %46, %47, %48, %43, %44, %18)[2m : Double[0m[2m ; ops [%45:callee/borrow#0, %51:value/borrow#0, %3:value/borrow#1, %46:value/borrow#2, %47:value/borrow#3, %48:value/borrow#4, %43:value/borrow#5, %44:value/borrow#6, %18:value/borrow#7][0m
[35m    %55 = drop %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m[0m
    %53 = fmul %38, %52[2m ; ops [%38:value/borrow#0, %52:value/borrow#1][0m
    return %53[2m ; ops [%53:return/consume#0][0m
}
fn hh2.frame(%0 node[2m: Ptr @borrow[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @consume[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = fn_ref $hh2.kernel
    %6 = call %5(%0, %1, %3, %4)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3][0m
    %7 = fn_ref $ylc_get_output_buf
    %8 = call %7(%0)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %0:value/borrow#0][0m
    %9 = ptr_offset %8, %3[2m : Ptr[0m[2m ; ops [%8:value/borrow#0, %3:index/borrow#1][0m
    %10 = store %9, %6[2m : ()[0m[2m ; ops [%9:value/borrow#0, %6:element/consume#1][0m
    %11 = const.void
    return %11[2m ; ops [%11:return/consume#0][0m
}
fn cymbal.cons(%0 freq[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = fn_ref $ylc_create_audio_frame_node
    %2 = fn_ref $cymbal.frame
    %3 = const.string "cymbal"
    %4 = const.int 1
    %5 = const.int 1
    %6 = const.int 160
    %7 = call %1(%2, %4, %5, %6, %3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %4:value/borrow#1, %5:value/borrow#2, %6:value/borrow#3, %3:value/borrow#4][0m
    %8 = fn_ref $ylc_audio_node_set_state_init
    %9 = fn_ref $cymbal.init
    %10 = call %8(%7, %9)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1][0m
    %11 = fn_ref $ylc_audio_node_inline_state
    %12 = call %11(%7)[2m : Ptr[0m[2m ; ops [%11:callee/borrow#0, %7:value/borrow#0][0m
    %13 = fn_ref $cymbal.init
    %14 = call %13(%12)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0][0m
    %15 = fn_ref $node_connect_input
    %16 = fn_ref $const_sig
    %17 = call %16(%0)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %0:value/borrow#0][0m
    %18 = const.int 0
    %19 = call %15(%18, %7, %17)[2m : ()[0m[2m ; ops [%15:callee/borrow#0, %18:value/borrow#0, %7:value/borrow#1, %17:value/borrow#2][0m
    return %7[2m ; ops [%7:return/consume#0][0m
}
fn cymbal.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 144
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 112
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = const.char '\x00'
    %9 = store %7, %8[2m : ()[0m[2m ; ops [%7:value/borrow#0, %8:element/consume#1][0m
    %10 = const.int 56
    %11 = ptr_offset %0, %10[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %10:index/borrow#1][0m
    %12 = const.char '\x00'
    %13 = store %11, %12[2m : ()[0m[2m ; ops [%11:value/borrow#0, %12:element/consume#1][0m
    %14 = const.int 48
    %15 = ptr_offset %0, %14[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %14:index/borrow#1][0m
    %16 = primitive_cast %15 from Ptr of Char to Ptr of Double[2m ; ops [%15:value/borrow#0][0m
    %17 = const.double 0
    %18 = store %16, %17[2m : ()[0m[2m ; ops [%16:value/borrow#0, %17:element/consume#1][0m
    %19 = const.int 24
    %20 = ptr_offset %0, %19[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %19:index/borrow#1][0m
    %21 = primitive_cast %20 from Ptr of Char to Ptr of Double[2m ; ops [%20:value/borrow#0][0m
    %22 = const.double 0
    %23 = store %21, %22[2m : ()[0m[2m ; ops [%21:value/borrow#0, %22:element/consume#1][0m
    %24 = const.int 8
    %25 = ptr_offset %0, %24[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Double[2m ; ops [%25:value/borrow#0][0m
    %27 = const.double 0
    %28 = store %26, %27[2m : ()[0m[2m ; ops [%26:value/borrow#0, %27:element/consume#1][0m
    %29 = const.int 0
    %30 = ptr_offset %0, %29[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Double[2m ; ops [%30:value/borrow#0][0m
    %32 = const.double 0
    %33 = store %31, %32[2m : ()[0m[2m ; ops [%31:value/borrow#0, %32:element/consume#1][0m
    %34 = const.int 56
    %35 = ptr_offset %0, %34[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %34:index/borrow#1][0m
    %36 = fn_ref $pink.init
    %37 = call %36(%35)[2m : ()[0m[2m ; ops [%36:callee/borrow#0, %35:value/borrow#0][0m
    %38 = const.int 112
    %39 = ptr_offset %0, %38[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %38:index/borrow#1][0m
    %40 = fn_ref $bpf.init
    %41 = call %40(%39)[2m : ()[0m[2m ; ops [%40:callee/borrow#0, %39:value/borrow#0][0m
    %42 = const.void
    return %42[2m ; ops [%42:return/consume#0][0m
}
fn cymbal.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = primitive_cast %5 from Int to Double[2m ; ops [%5:value/borrow#0][0m
    %7 = const.int 0
    %8 = ptr_offset %1, %7[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %7:index/borrow#1][0m
    %9 = primitive_cast %8 from Ptr of Char to Ptr of Double[2m ; ops [%8:value/borrow#0][0m
    %10 = fn_ref $ylc_audio_trig_once_kernel
    %11 = call %10(%9, %3, %6)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %9:value/borrow#0, %3:value/borrow#1, %6:value/borrow#2][0m
    %12 = fn_ref $db_amp
    %13 = const.double -15
    %14 = call %12(%13)[2m : Double[0m[2m ; ops [%12:callee/borrow#0, %13:value/borrow#0][0m
    %15 = const.double 0
    %16 = const.double 0.1
    %17 = const.double 0.2
    %18 = const.double 1
    %19 = const.double 0.5
    %20 = fn_ref $ylc_audio_rect_kernel
    %21 = const.int 8
    %22 = ptr_offset %1, %21[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = call %20(%23, %3, %19, %11)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %3:value/borrow#1, %19:value/borrow#2, %11:value/borrow#3][0m
    %25 = fn_ref $ylc_audio_adsr_kernel
    %26 = const.int 24
    %27 = ptr_offset %1, %26[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = call %25(%28, %3, %15, %16, %17, %18, %24)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %28:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2, %16:value/borrow#3, %17:value/borrow#4, %18:value/borrow#5, %24:value/borrow#6][0m
    %30 = fn_ref $ylc_audio_kill_on_end_kernel
    %31 = const.int 48
    %32 = ptr_offset %1, %31[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %31:index/borrow#1][0m
    %33 = primitive_cast %32 from Ptr of Char to Ptr of Double[2m ; ops [%32:value/borrow#0][0m
    %34 = call %30(%33, %3, %0, %29)[2m : Double[0m[2m ; ops [%30:callee/borrow#0, %33:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %29:value/borrow#3][0m
    %35 = fn_ref $db_amp
    %36 = const.double 40
    %37 = call %35(%36)[2m : Double[0m[2m ; ops [%35:callee/borrow#0, %36:value/borrow#0][0m
    %38 = const.double 16
    %39 = fn_ref $pink.kernel
    %40 = const.int 56
    %41 = ptr_offset %1, %40[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %40:index/borrow#1][0m
    %42 = primitive_cast %41 from Ptr of Char to Ptr of Char[2m ; ops [%41:value/borrow#0][0m
    %43 = call %39(%0, %42, %2, %3)[2m : Double[0m[2m ; ops [%39:callee/borrow#0, %0:value/consume#0, %42:value/borrow#1, %2:value/consume#2, %3:value/consume#3][0m
    %44 = fn_ref $bpf.kernel
    %45 = const.int 112
    %46 = ptr_offset %1, %45[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %45:index/borrow#1][0m
    %47 = primitive_cast %46 from Ptr of Char to Ptr of Char[2m ; ops [%46:value/borrow#0][0m
    %48 = call %44(%0, %47, %2, %3, %4, %38, %43)[2m : Double[0m[2m ; ops [%44:callee/borrow#0, %0:value/consume#0, %47:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %4:value/borrow#4, %38:value/borrow#5, %43:value/consume#6][0m
    %49 = fmul %37, %48[2m ; ops [%37:value/borrow#0, %48:value/borrow#1][0m
    %50 = const.double 1.01
    %51 = const.double 1.2
    %52 = fn_ref $ylc_audio_pm_osc_kernel
    %53 = const.int 144
    %54 = ptr_offset %1, %53[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %53:index/borrow#1][0m
    %55 = primitive_cast %54 from Ptr of Char to Ptr of Double[2m ; ops [%54:value/borrow#0][0m
    %56 = call %52(%55, %3, %51, %50, %4)[2m : Double[0m[2m ; ops [%52:callee/borrow#0, %55:value/borrow#0, %3:value/borrow#1, %51:value/borrow#2, %50:value/borrow#3, %4:value/borrow#4][0m
    %57 = fadd %49, %56[2m ; ops [%49:value/borrow#0, %56:value/borrow#1][0m
    %58 = fmul %34, %57[2m ; ops [%34:value/borrow#0, %57:value/borrow#1][0m
    %59 = fmul %14, %58[2m ; ops [%14:value/borrow#0, %58:value/borrow#1][0m
    return %59[2m ; ops [%59:return/consume#0][0m
}
fn cymbal.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = fn_ref $cymbal.kernel
    %12 = call %11(%0, %1, %3, %4, %10)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4][0m
    %13 = fn_ref $ylc_get_output_buf
    %14 = call %13(%0)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %0:value/borrow#0][0m
    %15 = ptr_offset %14, %3[2m : Ptr[0m[2m ; ops [%14:value/borrow#0, %3:index/borrow#1][0m
    %16 = store %15, %12[2m : ()[0m[2m ; ops [%15:value/borrow#0, %12:element/consume#1][0m
    %17 = const.void
    return %17[2m ; ops [%17:return/consume#0][0m
}
extern fn ylc_audio_pm_osc_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn mels.cons(%0 freq[2m: Double @borrow[0m, %1 dur[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %2 = fn_ref $ylc_create_audio_frame_node
    %3 = fn_ref $mels.frame
    %4 = const.string "mels"
    %5 = const.int 2
    %6 = const.int 2
    %7 = const.int 232
    %8 = call %2(%3, %5, %6, %7, %4)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %3:value/borrow#0, %5:value/borrow#1, %6:value/borrow#2, %7:value/borrow#3, %4:value/borrow#4][0m
    %9 = fn_ref $ylc_audio_node_set_state_init
    %10 = fn_ref $mels.init
    %11 = call %9(%8, %10)[2m : ()[0m[2m ; ops [%9:callee/borrow#0, %8:value/borrow#0, %10:value/borrow#1][0m
    %12 = fn_ref $ylc_audio_node_inline_state
    %13 = call %12(%8)[2m : Ptr[0m[2m ; ops [%12:callee/borrow#0, %8:value/borrow#0][0m
    %14 = fn_ref $mels.init
    %15 = call %14(%13)[2m : ()[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0][0m
    %16 = fn_ref $node_connect_input
    %17 = fn_ref $const_sig
    %18 = call %17(%0)[2m : Ptr[0m[2m ; ops [%17:callee/borrow#0, %0:value/borrow#0][0m
    %19 = const.int 0
    %20 = call %16(%19, %8, %18)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %19:value/borrow#0, %8:value/borrow#1, %18:value/borrow#2][0m
    %21 = fn_ref $const_sig
    %22 = call %21(%1)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %1:value/borrow#0][0m
    %23 = const.int 1
    %24 = call %16(%23, %8, %22)[2m : ()[0m[2m ; ops [%16:callee/borrow#0, %23:value/borrow#0, %8:value/borrow#1, %22:value/borrow#2][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn mels.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 200
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = const.char '\x00'
    %4 = store %2, %3[2m : ()[0m[2m ; ops [%2:value/borrow#0, %3:element/consume#1][0m
    %5 = const.int 168
    %6 = ptr_offset %0, %5[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %5:index/borrow#1][0m
    %7 = const.char '\x00'
    %8 = store %6, %7[2m : ()[0m[2m ; ops [%6:value/borrow#0, %7:element/consume#1][0m
    %9 = const.int 160
    %10 = ptr_offset %0, %9[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %9:index/borrow#1][0m
    %11 = const.char '\x00'
    %12 = store %10, %11[2m : ()[0m[2m ; ops [%10:value/borrow#0, %11:element/consume#1][0m
    %13 = const.int 152
    %14 = ptr_offset %0, %13[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %13:index/borrow#1][0m
    %15 = const.char '\x00'
    %16 = store %14, %15[2m : ()[0m[2m ; ops [%14:value/borrow#0, %15:element/consume#1][0m
    %17 = const.int 144
    %18 = ptr_offset %0, %17[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %17:index/borrow#1][0m
    %19 = primitive_cast %18 from Ptr of Char to Ptr of Double[2m ; ops [%18:value/borrow#0][0m
    %20 = const.double 0
    %21 = store %19, %20[2m : ()[0m[2m ; ops [%19:value/borrow#0, %20:element/consume#1][0m
    %22 = const.int 136
    %23 = ptr_offset %0, %22[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %22:index/borrow#1][0m
    %24 = primitive_cast %23 from Ptr of Char to Ptr of Double[2m ; ops [%23:value/borrow#0][0m
    %25 = const.double 0
    %26 = store %24, %25[2m : ()[0m[2m ; ops [%24:value/borrow#0, %25:element/consume#1][0m
    %27 = const.int 96
    %28 = ptr_offset %0, %27[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %27:index/borrow#1][0m
    %29 = primitive_cast %28 from Ptr of Char to Ptr of Double[2m ; ops [%28:value/borrow#0][0m
    %30 = const.double 0
    %31 = store %29, %30[2m : ()[0m[2m ; ops [%29:value/borrow#0, %30:element/consume#1][0m
    %32 = const.int 56
    %33 = ptr_offset %0, %32[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %32:index/borrow#1][0m
    %34 = primitive_cast %33 from Ptr of Char to Ptr of Double[2m ; ops [%33:value/borrow#0][0m
    %35 = const.double 0
    %36 = store %34, %35[2m : ()[0m[2m ; ops [%34:value/borrow#0, %35:element/consume#1][0m
    %37 = const.int 48
    %38 = ptr_offset %0, %37[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %37:index/borrow#1][0m
    %39 = primitive_cast %38 from Ptr of Char to Ptr of Double[2m ; ops [%38:value/borrow#0][0m
    %40 = const.double 0
    %41 = store %39, %40[2m : ()[0m[2m ; ops [%39:value/borrow#0, %40:element/consume#1][0m
    %42 = const.int 24
    %43 = ptr_offset %0, %42[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %42:index/borrow#1][0m
    %44 = primitive_cast %43 from Ptr of Char to Ptr of Double[2m ; ops [%43:value/borrow#0][0m
    %45 = const.double 0
    %46 = store %44, %45[2m : ()[0m[2m ; ops [%44:value/borrow#0, %45:element/consume#1][0m
    %47 = const.int 8
    %48 = ptr_offset %0, %47[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %47:index/borrow#1][0m
    %49 = primitive_cast %48 from Ptr of Char to Ptr of Double[2m ; ops [%48:value/borrow#0][0m
    %50 = const.double 0
    %51 = store %49, %50[2m : ()[0m[2m ; ops [%49:value/borrow#0, %50:element/consume#1][0m
    %52 = const.int 0
    %53 = ptr_offset %0, %52[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %52:index/borrow#1][0m
    %54 = primitive_cast %53 from Ptr of Char to Ptr of Double[2m ; ops [%53:value/borrow#0][0m
    %55 = const.double 0
    %56 = store %54, %55[2m : ()[0m[2m ; ops [%54:value/borrow#0, %55:element/consume#1][0m
    %57 = const.int 152
    %58 = ptr_offset %0, %57[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %57:index/borrow#1][0m
    %59 = fn_ref $sum.init
    %60 = call %59(%58)[2m : ()[0m[2m ; ops [%59:callee/borrow#0, %58:value/consume#0][0m
    %61 = const.int 160
    %62 = ptr_offset %0, %61[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %61:index/borrow#1][0m
    %63 = call %59(%62)[2m : ()[0m[2m ; ops [%59:callee/borrow#0, %62:value/consume#0][0m
    %64 = const.int 168
    %65 = ptr_offset %0, %64[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %64:index/borrow#1][0m
    %66 = fn_ref $lpf.init
    %67 = call %66(%65)[2m : ()[0m[2m ; ops [%66:callee/borrow#0, %65:value/borrow#0][0m
    %68 = const.int 200
    %69 = ptr_offset %0, %68[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %68:index/borrow#1][0m
    %70 = call %66(%69)[2m : ()[0m[2m ; ops [%66:callee/borrow#0, %69:value/borrow#0][0m
    %71 = const.void
    return %71[2m ; ops [%71:return/consume#0][0m
}
fn mels.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 freq[2m: Double @borrow[0m, %5 dur[2m: Double @borrow[0m)[2m -> (Double, Double, ) @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = const.int 0
    %7 = primitive_cast %6 from Int to Double[2m ; ops [%6:value/borrow#0][0m
    %8 = const.int 0
    %9 = ptr_offset %1, %8[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %8:index/borrow#1][0m
    %10 = primitive_cast %9 from Ptr of Char to Ptr of Double[2m ; ops [%9:value/borrow#0][0m
    %11 = fn_ref $ylc_audio_trig_once_kernel
    %12 = call %11(%10, %3, %7)[2m : Double[0m[2m ; ops [%11:callee/borrow#0, %10:value/borrow#0, %3:value/borrow#1, %7:value/borrow#2][0m
    %13 = fn_ref $db_amp
    %14 = const.double -16
    %15 = call %13(%14)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %14:value/borrow#0][0m
    %16 = const.double 0.01
    %17 = const.double 0.1
    %18 = const.double 0.5
    %19 = const.double 0.5
    %20 = fn_ref $ylc_audio_rect_kernel
    %21 = const.int 8
    %22 = ptr_offset %1, %21[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = call %20(%23, %3, %5, %12)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %3:value/borrow#1, %5:value/borrow#2, %12:value/borrow#3][0m
    %25 = fn_ref $ylc_audio_adsr_kernel
    %26 = const.int 24
    %27 = ptr_offset %1, %26[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = call %25(%28, %3, %16, %17, %18, %19, %24)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %28:value/borrow#0, %3:value/borrow#1, %16:value/borrow#2, %17:value/borrow#3, %18:value/borrow#4, %19:value/borrow#5, %24:value/borrow#6][0m
    %30 = fn_ref $ylc_audio_kill_on_end_kernel
    %31 = const.int 48
    %32 = ptr_offset %1, %31[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %31:index/borrow#1][0m
    %33 = primitive_cast %32 from Ptr of Char to Ptr of Double[2m ; ops [%32:value/borrow#0][0m
    %34 = call %30(%33, %3, %0, %29)[2m : Double[0m[2m ; ops [%30:callee/borrow#0, %33:value/borrow#0, %3:value/borrow#1, %0:value/borrow#2, %29:value/borrow#3][0m
    %35 = const.double 5000
    %36 = const.double 1
    %37 = const.double 0.5
    %38 = const.double 0.99
    %39 = const.double 1.01
    %40 = fn_ref $ylc_audio_lfnoise_kernel
    %41 = const.int 56
    %42 = ptr_offset %1, %41[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %41:index/borrow#1][0m
    %43 = primitive_cast %42 from Ptr of Char to Ptr of Double[2m ; ops [%42:value/borrow#0][0m
    %44 = call %40(%43, %3, %37, %38, %39)[2m : Double[0m[2m ; ops [%40:callee/borrow#0, %43:value/borrow#0, %3:value/borrow#1, %37:value/borrow#2, %38:value/borrow#3, %39:value/borrow#4][0m
    %45 = const.double 0.5
    %46 = const.double 0.99
    %47 = const.double 1.01
    %48 = fn_ref $ylc_audio_lfnoise_kernel
    %49 = const.int 96
    %50 = ptr_offset %1, %49[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %49:index/borrow#1][0m
    %51 = primitive_cast %50 from Ptr of Char to Ptr of Double[2m ; ops [%50:value/borrow#0][0m
    %52 = call %48(%51, %3, %45, %46, %47)[2m : Double[0m[2m ; ops [%48:callee/borrow#0, %51:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2, %46:value/borrow#3, %47:value/borrow#4][0m
    %53 = construct.tuple { %44, %52 }[2m : (Double, Double, )[0m[2m ; ops [%44:field/consume#0, %52:field/consume#1][0m
    %54 = fmul %4, %44[2m ; ops [%4:value/borrow#0, %44:value/borrow#1][0m
    %55 = fmul %4, %52[2m ; ops [%4:value/borrow#0, %52:value/borrow#1][0m
    %56 = construct.tuple { %54, %55 }[2m : (Double, Double, )[0m[2m ; ops [%54:field/consume#0, %55:field/consume#1][0m
    %57 = fn_ref $ylc_audio_saw_osc_kernel
    %58 = const.int 136
    %59 = ptr_offset %1, %58[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %58:index/borrow#1][0m
    %60 = primitive_cast %59 from Ptr of Char to Ptr of Double[2m ; ops [%59:value/borrow#0][0m
    %61 = call %57(%60, %3, %54)[2m : Double[0m[2m ; ops [%57:callee/borrow#0, %60:value/borrow#0, %3:value/borrow#1, %54:value/borrow#2][0m
    %62 = const.int 144
    %63 = ptr_offset %1, %62[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %62:index/borrow#1][0m
    %64 = primitive_cast %63 from Ptr of Char to Ptr of Double[2m ; ops [%63:value/borrow#0][0m
    %65 = call %57(%64, %3, %55)[2m : Double[0m[2m ; ops [%57:callee/borrow#0, %64:value/borrow#0, %3:value/borrow#1, %55:value/borrow#2][0m
    %66 = construct.tuple { %61, %65 }[2m : (Double, Double, )[0m[2m ; ops [%61:field/consume#0, %65:field/consume#1][0m
    %67 = fn_ref $sum.kernel
    %68 = const.int 152
    %69 = ptr_offset %1, %68[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %68:index/borrow#1][0m
    %70 = primitive_cast %69 from Ptr of Char to Ptr of Char[2m ; ops [%69:value/borrow#0][0m
    %71 = call %67(%0, %70, %2, %3, %61)[2m : Double[0m[2m ; ops [%67:callee/borrow#0, %0:value/consume#0, %70:value/consume#1, %2:value/consume#2, %3:value/consume#3, %61:value/consume#4][0m
    %72 = const.int 160
    %73 = ptr_offset %1, %72[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %72:index/borrow#1][0m
    %74 = primitive_cast %73 from Ptr of Char to Ptr of Char[2m ; ops [%73:value/borrow#0][0m
    %75 = call %67(%0, %74, %2, %3, %65)[2m : Double[0m[2m ; ops [%67:callee/borrow#0, %0:value/consume#0, %74:value/consume#1, %2:value/consume#2, %3:value/consume#3, %65:value/consume#4][0m
    %76 = construct.tuple { %71, %75 }[2m : (Double, Double, )[0m[2m ; ops [%71:field/consume#0, %75:field/consume#1][0m
    %77 = fn_ref $lpf.kernel
    %78 = const.int 168
    %79 = ptr_offset %1, %78[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %78:index/borrow#1][0m
    %80 = primitive_cast %79 from Ptr of Char to Ptr of Char[2m ; ops [%79:value/borrow#0][0m
    %81 = call %77(%0, %80, %2, %3, %35, %36, %71)[2m : Double[0m[2m ; ops [%77:callee/borrow#0, %0:value/consume#0, %80:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %35:value/borrow#4, %36:value/borrow#5, %71:value/consume#6][0m
    %82 = const.int 200
    %83 = ptr_offset %1, %82[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %82:index/borrow#1][0m
    %84 = primitive_cast %83 from Ptr of Char to Ptr of Char[2m ; ops [%83:value/borrow#0][0m
    %85 = call %77(%0, %84, %2, %3, %35, %36, %75)[2m : Double[0m[2m ; ops [%77:callee/borrow#0, %0:value/consume#0, %84:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %35:value/borrow#4, %36:value/borrow#5, %75:value/consume#6][0m
    %86 = construct.tuple { %81, %85 }[2m : (Double, Double, )[0m[2m ; ops [%81:field/consume#0, %85:field/consume#1][0m
    %87 = fmul %34, %81[2m ; ops [%34:value/borrow#0, %81:value/borrow#1][0m
    %88 = fmul %34, %85[2m ; ops [%34:value/borrow#0, %85:value/borrow#1][0m
    %89 = construct.tuple { %87, %88 }[2m : (Double, Double, )[0m[2m ; ops [%87:field/consume#0, %88:field/consume#1][0m
    %90 = fmul %15, %87[2m ; ops [%15:value/borrow#0, %87:value/borrow#1][0m
    %91 = fmul %15, %88[2m ; ops [%15:value/borrow#0, %88:value/borrow#1][0m
    %92 = construct.tuple { %90, %91 }[2m : (Double, Double, )[0m[2m ; ops [%90:field/consume#0, %91:field/consume#1][0m
    return %92[2m ; ops [%92:return/consume#0][0m
}
fn mels.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = fn_ref $mels.kernel
    %18 = call %17(%0, %1, %3, %4, %10, %16)[2m : (Double, Double, )[0m[2m ; ops [%17:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/borrow#5][0m
    %19 = fn_ref $ylc_get_output_buf
    %20 = call %19(%0)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %0:value/borrow#0][0m
    %21 = const.int 2
    %22 = imul %3, %21[2m ; ops [%3:value/borrow#0, %21:value/borrow#1][0m
    %23 = extract.field %18, 0[2m : Double[0m[2m ; ops [%18:container/borrow#0][0m
    %24 = ptr_offset %20, %22[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %22:index/borrow#1][0m
    %25 = store %24, %23[2m : ()[0m[2m ; ops [%24:value/borrow#0, %23:element/consume#1][0m
    %26 = extract.field %18, 1[2m : Double[0m[2m ; ops [%18:container/borrow#0][0m
    %27 = const.int 1
    %28 = iadd %22, %27[2m ; ops [%22:value/borrow#0, %27:value/borrow#1][0m
    %29 = ptr_offset %20, %28[2m : Ptr[0m[2m ; ops [%20:value/borrow#0, %28:index/borrow#1][0m
    %30 = store %29, %26[2m : ()[0m[2m ; ops [%29:value/borrow#0, %26:element/consume#1][0m
    %31 = const.void
    return %31[2m ; ops [%31:return/consume#0][0m
}
extern fn ylc_audio_lfnoise_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_saw_osc_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn note(%0 d[2m: `1992 @borrow[0m, %1 n[2m: Double @borrow[0m)[2m -> `1994 @owned[0m {
  bb0 entry:
    %2 = fn_ref $DSP.~>
    %3 = fn_ref $DSP.DSP_common.midi_freq
    %4 = call %3(%1)[2m : Double[0m[2m ; ops [%3:callee/borrow#0, %1:value/borrow#0][0m
    %5 = fn_ref $cymbal.cons
    %6 = call %5(%4)[2m : Ptr[0m[2m ; ops [%5:callee/borrow#0, %4:value/borrow#0][0m
    %7 = global_load @$global.r[2m : Ptr[0m
    %8 = call %2(%6, %7)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %6:value/borrow#0, %7:value/borrow#1][0m
    %9 = global_load @$global.beat[2m : Double[0m
    %10 = call @*(%0, %9)[2m : `1994[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %9:value/borrow#1][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn mel(%0 d[2m: Double @borrow[0m, %1 n[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fn_ref $DSP.~>
    %3 = fn_ref $DSP.DSP_common.midi_freq
    %4 = call %3(%1)[2m : Double[0m[2m ; ops [%3:callee/borrow#0, %1:value/borrow#0][0m
    %5 = global_load @$global.beat[2m : Double[0m
    %6 = fmul %0, %5[2m ; ops [%0:value/borrow#0, %5:value/borrow#1][0m
    %7 = fn_ref $mels.cons
    %8 = call %7(%4, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %4:value/borrow#0, %6:value/borrow#1][0m
    %9 = global_load @$global.r[2m : Ptr[0m
    %10 = call %2(%8, %9)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %8:value/borrow#0, %9:value/borrow#1][0m
    %11 = global_load @$global.beat[2m : Double[0m
    %12 = fmul %0, %11[2m ; ops [%0:value/borrow#0, %11:value/borrow#1][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn m(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $mel
    %2 = const.double 2
    %3 = const.double 59
    %4 = call %1(%2, %3)[2m : Double[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %3:value/borrow#1][0m
    yield %4, bb1[2m ; ops [%4:value/borrow#0][0m
  bb1 yield.resume:
    %5 = fn_ref $mel
    %6 = const.double 0.75
    %7 = const.double 59
    %8 = call %5(%6, %7)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %6:value/borrow#0, %7:value/borrow#1][0m
    yield %8, bb2[2m ; ops [%8:value/borrow#0][0m
  bb2 yield.resume:
    %9 = fn_ref $mel
    %10 = const.double 0.75
    %11 = const.double 59
    %12 = call %9(%10, %11)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %10:value/borrow#0, %11:value/borrow#1][0m
    yield %12, bb3[2m ; ops [%12:value/borrow#0][0m
  bb3 yield.resume:
    %13 = fn_ref $mel
    %14 = const.double 0.5
    %15 = const.double 57
    %16 = call %13(%14, %15)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %14:value/borrow#0, %15:value/borrow#1][0m
    yield %16, bb4[2m ; ops [%16:value/borrow#0][0m
  bb4 yield.resume:
    %17 = fn_ref $mel
    %18 = const.double 2
    %19 = const.double 61
    %20 = call %17(%18, %19)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %18:value/borrow#0, %19:value/borrow#1][0m
    yield %20, bb5[2m ; ops [%20:value/borrow#0][0m
  bb5 yield.resume:
    %21 = fn_ref $mel
    %22 = const.double 0.75
    %23 = const.double 61
    %24 = call %21(%22, %23)[2m : Double[0m[2m ; ops [%21:callee/borrow#0, %22:value/borrow#0, %23:value/borrow#1][0m
    yield %24, bb6[2m ; ops [%24:value/borrow#0][0m
  bb6 yield.resume:
    %25 = fn_ref $mel
    %26 = const.double 1
    %27 = const.double 61
    %28 = call %25(%26, %27)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %26:value/borrow#0, %27:value/borrow#1][0m
    yield %28, bb7[2m ; ops [%28:value/borrow#0][0m
  bb7 yield.resume:
    %29 = fn_ref $mel
    %30 = const.double 0.25
    %31 = const.double 62
    %32 = call %29(%30, %31)[2m : Double[0m[2m ; ops [%29:callee/borrow#0, %30:value/borrow#0, %31:value/borrow#1][0m
    yield %32, bb8[2m ; ops [%32:value/borrow#0][0m
  bb8 yield.resume:
    coro.done
}
fn m2(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $mel
    %2 = const.double 1
    %3 = const.double 73
    %4 = call %1(%2, %3)[2m : Double[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %3:value/borrow#1][0m
    yield %4, bb1[2m ; ops [%4:value/borrow#0][0m
  bb1 yield.resume:
    %5 = fn_ref $mel
    %6 = const.double 2
    %7 = const.double 71
    %8 = call %5(%6, %7)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %6:value/borrow#0, %7:value/borrow#1][0m
    yield %8, bb2[2m ; ops [%8:value/borrow#0][0m
  bb2 yield.resume:
    %9 = fn_ref $mel
    %10 = const.double 0.25
    %11 = const.double 71
    %12 = call %9(%10, %11)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %10:value/borrow#0, %11:value/borrow#1][0m
    yield %12, bb3[2m ; ops [%12:value/borrow#0][0m
  bb3 yield.resume:
    %13 = fn_ref $mel
    %14 = const.double 0.3
    %15 = const.double 73
    %16 = call %13(%14, %15)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %14:value/borrow#0, %15:value/borrow#1][0m
    yield %16, bb4[2m ; ops [%16:value/borrow#0][0m
  bb4 yield.resume:
    %17 = fn_ref $mel
    %18 = const.double 0.3
    %19 = const.double 71
    %20 = call %17(%18, %19)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %18:value/borrow#0, %19:value/borrow#1][0m
    yield %20, bb5[2m ; ops [%20:value/borrow#0][0m
  bb5 yield.resume:
    %21 = fn_ref $mel
    %22 = const.double 2
    %23 = const.double 71
    %24 = call %21(%22, %23)[2m : Double[0m[2m ; ops [%21:callee/borrow#0, %22:value/borrow#0, %23:value/borrow#1][0m
    yield %24, bb6[2m ; ops [%24:value/borrow#0][0m
  bb6 yield.resume:
    %25 = const.double 2.15
    %26 = global_load @$global.beat[2m : Double[0m
    %27 = fmul %25, %26[2m ; ops [%25:value/borrow#0, %26:value/borrow#1][0m
    yield %27, bb7[2m ; ops [%27:value/borrow#0][0m
  bb7 yield.resume:
    coro.done
}
fn m3(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $mel
    %2 = const.double 1
    %3 = const.double 62
    %4 = call %1(%2, %3)[2m : Double[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %3:value/borrow#1][0m
    yield %4, bb1[2m ; ops [%4:value/borrow#0][0m
  bb1 yield.resume:
    %5 = fn_ref $mel
    %6 = const.double 0.75
    %7 = const.double 61
    %8 = call %5(%6, %7)[2m : Double[0m[2m ; ops [%5:callee/borrow#0, %6:value/borrow#0, %7:value/borrow#1][0m
    yield %8, bb2[2m ; ops [%8:value/borrow#0][0m
  bb2 yield.resume:
    %9 = fn_ref $mel
    %10 = const.double 1.25
    %11 = const.double 61
    %12 = call %9(%10, %11)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %10:value/borrow#0, %11:value/borrow#1][0m
    yield %12, bb3[2m ; ops [%12:value/borrow#0][0m
  bb3 yield.resume:
    %13 = fn_ref $mel
    %14 = const.double 5
    %15 = const.double 88
    %16 = call %13(%14, %15)[2m : Double[0m[2m ; ops [%13:callee/borrow#0, %14:value/borrow#0, %15:value/borrow#1][0m
    yield %16, bb4[2m ; ops [%16:value/borrow#0][0m
  bb4 yield.resume:
    coro.done
}
fn c(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $note
    %2 = const.double 0.5
    %3 = const.int 0
    %4 = global_load @$global.notes[2m : Array of Double[0m
[35m    %138 = dup %4[2m : ()[0m[2m ; ops [%4:value/borrow#0][0m[0m
    %5 = extract.field %4, 2[2m : Ptr[0m[2m ; ops [%4:container/borrow#0][0m
    %6 = ptr_offset %5, %3[2m : Ptr[0m[2m ; ops [%5:value/borrow#0, %3:index/borrow#1][0m
    %7 = load %6[2m : Double[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = call %1 as $note.Double.Double.Double(%2, %7)[2m : Double[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %7:value/borrow#1][0m
[35m    %139 = drop %4[2m : ()[0m[2m ; ops [%4:value/borrow#0][0m[0m
    yield %8, bb1[2m ; ops [%8:value/borrow#0][0m
  bb1 yield.resume:
    %9 = fn_ref $note
    %10 = const.double 0.5
    %11 = const.int 1
    %12 = global_load @$global.notes[2m : Array of Double[0m
[35m    %140 = dup %12[2m : ()[0m[2m ; ops [%12:value/borrow#0][0m[0m
    %13 = extract.field %12, 2[2m : Ptr[0m[2m ; ops [%12:container/borrow#0][0m
    %14 = ptr_offset %13, %11[2m : Ptr[0m[2m ; ops [%13:value/borrow#0, %11:index/borrow#1][0m
    %15 = load %14[2m : Double[0m[2m ; ops [%14:value/borrow#0][0m
    %16 = call %9 as $note.Double.Double.Double(%10, %15)[2m : Double[0m[2m ; ops [%9:callee/borrow#0, %10:value/borrow#0, %15:value/borrow#1][0m
[35m    %141 = drop %12[2m : ()[0m[2m ; ops [%12:value/borrow#0][0m[0m
    yield %16, bb2[2m ; ops [%16:value/borrow#0][0m
  bb2 yield.resume:
    %17 = fn_ref $note
    %18 = const.double 0.5
    %19 = const.int 2
    %20 = global_load @$global.notes[2m : Array of Double[0m
[35m    %142 = dup %20[2m : ()[0m[2m ; ops [%20:value/borrow#0][0m[0m
    %21 = extract.field %20, 2[2m : Ptr[0m[2m ; ops [%20:container/borrow#0][0m
    %22 = ptr_offset %21, %19[2m : Ptr[0m[2m ; ops [%21:value/borrow#0, %19:index/borrow#1][0m
    %23 = load %22[2m : Double[0m[2m ; ops [%22:value/borrow#0][0m
    %24 = call %17 as $note.Double.Double.Double(%18, %23)[2m : Double[0m[2m ; ops [%17:callee/borrow#0, %18:value/borrow#0, %23:value/borrow#1][0m
[35m    %143 = drop %20[2m : ()[0m[2m ; ops [%20:value/borrow#0][0m[0m
    yield %24, bb3[2m ; ops [%24:value/borrow#0][0m
  bb3 yield.resume:
    %25 = const.double 0.25
    %26 = fn_ref $rand_double
    %27 = call %26()[2m : Double[0m[2m ; ops [%26:callee/borrow#0][0m
    %28 = const.double 0.3
    %29 = flte %27, %28[2m ; ops [%27:value/borrow#0, %28:value/borrow#1][0m
    %30 = primitive_cast %29 from Bool to Double[2m ; ops [%29:value/borrow#0][0m
    %31 = fmul %25, %30[2m ; ops [%25:value/borrow#0, %30:value/borrow#1][0m
    %32 = fn_ref $note
    %33 = const.double 0.5
    %34 = fsub %33, %31[2m ; ops [%33:value/borrow#0, %31:value/borrow#1][0m
    %35 = const.int 3
    %36 = global_load @$global.notes[2m : Array of Double[0m
[35m    %144 = dup %36[2m : ()[0m[2m ; ops [%36:value/borrow#0][0m[0m
    %37 = extract.field %36, 2[2m : Ptr[0m[2m ; ops [%36:container/borrow#0][0m
    %38 = ptr_offset %37, %35[2m : Ptr[0m[2m ; ops [%37:value/borrow#0, %35:index/borrow#1][0m
    %39 = load %38[2m : Double[0m[2m ; ops [%38:value/borrow#0][0m
    %40 = call %32 as $note.Double.Double.Double(%34, %39)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %34:value/borrow#0, %39:value/borrow#1][0m
[35m    %145 = drop %36[2m : ()[0m[2m ; ops [%36:value/borrow#0][0m[0m
    yield %40, bb4[2m ; ops [%40:value/borrow#0][0m
  bb4 yield.resume:
    %41 = fn_ref $note
    %42 = const.double 0.5
    %43 = fadd %42, %31[2m ; ops [%42:value/borrow#0, %31:value/borrow#1][0m
    %44 = const.int 4
    %45 = global_load @$global.notes[2m : Array of Double[0m
[35m    %146 = dup %45[2m : ()[0m[2m ; ops [%45:value/borrow#0][0m[0m
    %46 = extract.field %45, 2[2m : Ptr[0m[2m ; ops [%45:container/borrow#0][0m
    %47 = ptr_offset %46, %44[2m : Ptr[0m[2m ; ops [%46:value/borrow#0, %44:index/borrow#1][0m
    %48 = load %47[2m : Double[0m[2m ; ops [%47:value/borrow#0][0m
    %49 = call %41 as $note.Double.Double.Double(%43, %48)[2m : Double[0m[2m ; ops [%41:callee/borrow#0, %43:value/borrow#0, %48:value/borrow#1][0m
[35m    %147 = drop %45[2m : ()[0m[2m ; ops [%45:value/borrow#0][0m[0m
    yield %49, bb5[2m ; ops [%49:value/borrow#0][0m
  bb5 yield.resume:
    %50 = fn_ref $note
    %51 = const.double 0.5
    %52 = const.int 5
    %53 = global_load @$global.notes[2m : Array of Double[0m
[35m    %148 = dup %53[2m : ()[0m[2m ; ops [%53:value/borrow#0][0m[0m
    %54 = extract.field %53, 2[2m : Ptr[0m[2m ; ops [%53:container/borrow#0][0m
    %55 = ptr_offset %54, %52[2m : Ptr[0m[2m ; ops [%54:value/borrow#0, %52:index/borrow#1][0m
    %56 = load %55[2m : Double[0m[2m ; ops [%55:value/borrow#0][0m
    %57 = call %50 as $note.Double.Double.Double(%51, %56)[2m : Double[0m[2m ; ops [%50:callee/borrow#0, %51:value/borrow#0, %56:value/borrow#1][0m
[35m    %149 = drop %53[2m : ()[0m[2m ; ops [%53:value/borrow#0][0m[0m
    yield %57, bb6[2m ; ops [%57:value/borrow#0][0m
  bb6 yield.resume:
    %58 = fn_ref $note
    %59 = const.double 0.5
    %60 = const.int 6
    %61 = global_load @$global.notes[2m : Array of Double[0m
[35m    %150 = dup %61[2m : ()[0m[2m ; ops [%61:value/borrow#0][0m[0m
    %62 = extract.field %61, 2[2m : Ptr[0m[2m ; ops [%61:container/borrow#0][0m
    %63 = ptr_offset %62, %60[2m : Ptr[0m[2m ; ops [%62:value/borrow#0, %60:index/borrow#1][0m
    %64 = load %63[2m : Double[0m[2m ; ops [%63:value/borrow#0][0m
    %65 = call %58 as $note.Double.Double.Double(%59, %64)[2m : Double[0m[2m ; ops [%58:callee/borrow#0, %59:value/borrow#0, %64:value/borrow#1][0m
[35m    %151 = drop %61[2m : ()[0m[2m ; ops [%61:value/borrow#0][0m[0m
    yield %65, bb7[2m ; ops [%65:value/borrow#0][0m
  bb7 yield.resume:
    %66 = fn_ref $note
    %67 = const.double 0.5
    %68 = const.int 7
    %69 = global_load @$global.notes[2m : Array of Double[0m
[35m    %152 = dup %69[2m : ()[0m[2m ; ops [%69:value/borrow#0][0m[0m
    %70 = extract.field %69, 2[2m : Ptr[0m[2m ; ops [%69:container/borrow#0][0m
    %71 = ptr_offset %70, %68[2m : Ptr[0m[2m ; ops [%70:value/borrow#0, %68:index/borrow#1][0m
    %72 = load %71[2m : Double[0m[2m ; ops [%71:value/borrow#0][0m
    %73 = call %66 as $note.Double.Double.Double(%67, %72)[2m : Double[0m[2m ; ops [%66:callee/borrow#0, %67:value/borrow#0, %72:value/borrow#1][0m
[35m    %153 = drop %69[2m : ()[0m[2m ; ops [%69:value/borrow#0][0m[0m
    yield %73, bb8[2m ; ops [%73:value/borrow#0][0m
  bb8 yield.resume:
    %74 = fn_ref $note
    %75 = const.double 0.5
    %76 = const.int 8
    %77 = global_load @$global.notes[2m : Array of Double[0m
[35m    %154 = dup %77[2m : ()[0m[2m ; ops [%77:value/borrow#0][0m[0m
    %78 = extract.field %77, 2[2m : Ptr[0m[2m ; ops [%77:container/borrow#0][0m
    %79 = ptr_offset %78, %76[2m : Ptr[0m[2m ; ops [%78:value/borrow#0, %76:index/borrow#1][0m
    %80 = load %79[2m : Double[0m[2m ; ops [%79:value/borrow#0][0m
    %81 = call %74 as $note.Double.Double.Double(%75, %80)[2m : Double[0m[2m ; ops [%74:callee/borrow#0, %75:value/borrow#0, %80:value/borrow#1][0m
[35m    %155 = drop %77[2m : ()[0m[2m ; ops [%77:value/borrow#0][0m[0m
    yield %81, bb9[2m ; ops [%81:value/borrow#0][0m
  bb9 yield.resume:
    %82 = fn_ref $note
    %83 = const.double 0.5
    %84 = const.int 9
    %85 = global_load @$global.notes[2m : Array of Double[0m
[35m    %156 = dup %85[2m : ()[0m[2m ; ops [%85:value/borrow#0][0m[0m
    %86 = extract.field %85, 2[2m : Ptr[0m[2m ; ops [%85:container/borrow#0][0m
    %87 = ptr_offset %86, %84[2m : Ptr[0m[2m ; ops [%86:value/borrow#0, %84:index/borrow#1][0m
    %88 = load %87[2m : Double[0m[2m ; ops [%87:value/borrow#0][0m
    %89 = call %82 as $note.Double.Double.Double(%83, %88)[2m : Double[0m[2m ; ops [%82:callee/borrow#0, %83:value/borrow#0, %88:value/borrow#1][0m
[35m    %157 = drop %85[2m : ()[0m[2m ; ops [%85:value/borrow#0][0m[0m
    yield %89, bb10[2m ; ops [%89:value/borrow#0][0m
  bb10 yield.resume:
    %90 = fn_ref $note
    %91 = const.double 0.5
    %92 = const.int 10
    %93 = global_load @$global.notes[2m : Array of Double[0m
[35m    %158 = dup %93[2m : ()[0m[2m ; ops [%93:value/borrow#0][0m[0m
    %94 = extract.field %93, 2[2m : Ptr[0m[2m ; ops [%93:container/borrow#0][0m
    %95 = ptr_offset %94, %92[2m : Ptr[0m[2m ; ops [%94:value/borrow#0, %92:index/borrow#1][0m
    %96 = load %95[2m : Double[0m[2m ; ops [%95:value/borrow#0][0m
    %97 = call %90 as $note.Double.Double.Double(%91, %96)[2m : Double[0m[2m ; ops [%90:callee/borrow#0, %91:value/borrow#0, %96:value/borrow#1][0m
[35m    %159 = drop %93[2m : ()[0m[2m ; ops [%93:value/borrow#0][0m[0m
    yield %97, bb11[2m ; ops [%97:value/borrow#0][0m
  bb11 yield.resume:
    %98 = fn_ref $note
    %99 = const.double 0.5
    %100 = const.int 11
    %101 = global_load @$global.notes[2m : Array of Double[0m
[35m    %160 = dup %101[2m : ()[0m[2m ; ops [%101:value/borrow#0][0m[0m
    %102 = extract.field %101, 2[2m : Ptr[0m[2m ; ops [%101:container/borrow#0][0m
    %103 = ptr_offset %102, %100[2m : Ptr[0m[2m ; ops [%102:value/borrow#0, %100:index/borrow#1][0m
    %104 = load %103[2m : Double[0m[2m ; ops [%103:value/borrow#0][0m
    %105 = call %98 as $note.Double.Double.Double(%99, %104)[2m : Double[0m[2m ; ops [%98:callee/borrow#0, %99:value/borrow#0, %104:value/borrow#1][0m
[35m    %161 = drop %101[2m : ()[0m[2m ; ops [%101:value/borrow#0][0m[0m
    yield %105, bb12[2m ; ops [%105:value/borrow#0][0m
  bb12 yield.resume:
    %106 = fn_ref $note
    %107 = const.double 0.5
    %108 = const.int 12
    %109 = global_load @$global.notes[2m : Array of Double[0m
[35m    %162 = dup %109[2m : ()[0m[2m ; ops [%109:value/borrow#0][0m[0m
    %110 = extract.field %109, 2[2m : Ptr[0m[2m ; ops [%109:container/borrow#0][0m
    %111 = ptr_offset %110, %108[2m : Ptr[0m[2m ; ops [%110:value/borrow#0, %108:index/borrow#1][0m
    %112 = load %111[2m : Double[0m[2m ; ops [%111:value/borrow#0][0m
    %113 = call %106 as $note.Double.Double.Double(%107, %112)[2m : Double[0m[2m ; ops [%106:callee/borrow#0, %107:value/borrow#0, %112:value/borrow#1][0m
[35m    %163 = drop %109[2m : ()[0m[2m ; ops [%109:value/borrow#0][0m[0m
    yield %113, bb13[2m ; ops [%113:value/borrow#0][0m
  bb13 yield.resume:
    %114 = fn_ref $note
    %115 = const.double 0.5
    %116 = const.int 13
    %117 = global_load @$global.notes[2m : Array of Double[0m
[35m    %164 = dup %117[2m : ()[0m[2m ; ops [%117:value/borrow#0][0m[0m
    %118 = extract.field %117, 2[2m : Ptr[0m[2m ; ops [%117:container/borrow#0][0m
    %119 = ptr_offset %118, %116[2m : Ptr[0m[2m ; ops [%118:value/borrow#0, %116:index/borrow#1][0m
    %120 = load %119[2m : Double[0m[2m ; ops [%119:value/borrow#0][0m
    %121 = call %114 as $note.Double.Double.Double(%115, %120)[2m : Double[0m[2m ; ops [%114:callee/borrow#0, %115:value/borrow#0, %120:value/borrow#1][0m
[35m    %165 = drop %117[2m : ()[0m[2m ; ops [%117:value/borrow#0][0m[0m
    yield %121, bb14[2m ; ops [%121:value/borrow#0][0m
  bb14 yield.resume:
    %122 = fn_ref $note
    %123 = const.double 0.5
    %124 = const.int 14
    %125 = global_load @$global.notes[2m : Array of Double[0m
[35m    %166 = dup %125[2m : ()[0m[2m ; ops [%125:value/borrow#0][0m[0m
    %126 = extract.field %125, 2[2m : Ptr[0m[2m ; ops [%125:container/borrow#0][0m
    %127 = ptr_offset %126, %124[2m : Ptr[0m[2m ; ops [%126:value/borrow#0, %124:index/borrow#1][0m
    %128 = load %127[2m : Double[0m[2m ; ops [%127:value/borrow#0][0m
    %129 = call %122 as $note.Double.Double.Double(%123, %128)[2m : Double[0m[2m ; ops [%122:callee/borrow#0, %123:value/borrow#0, %128:value/borrow#1][0m
[35m    %167 = drop %125[2m : ()[0m[2m ; ops [%125:value/borrow#0][0m[0m
    yield %129, bb15[2m ; ops [%129:value/borrow#0][0m
  bb15 yield.resume:
    %130 = fn_ref $note
    %131 = const.double 0.5
    %132 = const.int 15
    %133 = global_load @$global.notes[2m : Array of Double[0m
[35m    %168 = dup %133[2m : ()[0m[2m ; ops [%133:value/borrow#0][0m[0m
    %134 = extract.field %133, 2[2m : Ptr[0m[2m ; ops [%133:container/borrow#0][0m
    %135 = ptr_offset %134, %132[2m : Ptr[0m[2m ; ops [%134:value/borrow#0, %132:index/borrow#1][0m
    %136 = load %135[2m : Double[0m[2m ; ops [%135:value/borrow#0][0m
    %137 = call %130 as $note.Double.Double.Double(%131, %136)[2m : Double[0m[2m ; ops [%130:callee/borrow#0, %131:value/borrow#0, %136:value/borrow#1][0m
[35m    %169 = drop %133[2m : ()[0m[2m ; ops [%133:value/borrow#0][0m[0m
    yield %137, bb16[2m ; ops [%137:value/borrow#0][0m
  bb16 yield.resume:
    coro.done
}
fn note.Double.Double.Double(%0 d[2m: Double @borrow[0m, %1 n[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fn_ref $DSP.~>
    %3 = fn_ref $DSP.DSP_common.midi_freq
    %4 = call %3(%1)[2m : Double[0m[2m ; ops [%3:callee/borrow#0, %1:value/borrow#0][0m
    %5 = fn_ref $cymbal.cons
    %6 = call %5(%4)[2m : Ptr[0m[2m ; ops [%5:callee/borrow#0, %4:value/borrow#0][0m
    %7 = global_load @$global.r[2m : Ptr[0m
    %8 = call %2(%6, %7)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %6:value/borrow#0, %7:value/borrow#1][0m
    %9 = global_load @$global.beat[2m : Double[0m
    %10 = fmul %0, %9[2m ; ops [%0:value/borrow#0, %9:value/borrow#1][0m
    return %10[2m ; ops [%10:return/consume#0][0m
}
fn hhc(%0 _[2m: () @borrow[0m)[2m -> Coroutine of `2078 with From <Array of `2078>,From <List
  | []
  | :: of `2078, List
>, @owned[0m {
  bb0 entry:
    coro.restart bb0
}
fn stutter.cons(%0 trig_sig[2m: Double @borrow[0m, %1 active[2m: Double @borrow[0m, %2 beat_length[2m: Double @borrow[0m, %3 subdiv[2m: Double @borrow[0m, %4 rate[2m: Double @borrow[0m, %5 inp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = fn_ref $ylc_create_audio_frame_node
    %7 = fn_ref $stutter.frame
    %8 = const.string "stutter"
    %9 = const.int 6
    %10 = const.int 1
    %11 = const.int 1536032
    %12 = call %6(%7, %9, %10, %11, %8)[2m : Ptr[0m[2m ; ops [%6:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1, %10:value/borrow#2, %11:value/borrow#3, %8:value/borrow#4][0m
    %13 = fn_ref $ylc_audio_node_set_state_init
    %14 = fn_ref $stutter.init
    %15 = call %13(%12, %14)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0, %14:value/borrow#1][0m
    %16 = fn_ref $ylc_audio_node_inline_state
    %17 = call %16(%12)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %12:value/borrow#0][0m
    %18 = fn_ref $stutter.init
    %19 = call %18(%17)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %17:value/borrow#0][0m
    %20 = fn_ref $node_connect_input
    %21 = fn_ref $const_sig
    %22 = call %21(%0)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %0:value/borrow#0][0m
    %23 = const.int 0
    %24 = call %20(%23, %12, %22)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %12:value/borrow#1, %22:value/borrow#2][0m
    %25 = fn_ref $const_sig
    %26 = call %25(%1)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %1:value/borrow#0][0m
    %27 = const.int 1
    %28 = call %20(%27, %12, %26)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %27:value/borrow#0, %12:value/borrow#1, %26:value/borrow#2][0m
    %29 = fn_ref $const_sig
    %30 = call %29(%2)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %2:value/borrow#0][0m
    %31 = const.int 2
    %32 = call %20(%31, %12, %30)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %31:value/borrow#0, %12:value/borrow#1, %30:value/borrow#2][0m
    %33 = fn_ref $const_sig
    %34 = call %33(%3)[2m : Ptr[0m[2m ; ops [%33:callee/borrow#0, %3:value/borrow#0][0m
    %35 = const.int 3
    %36 = call %20(%35, %12, %34)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %35:value/borrow#0, %12:value/borrow#1, %34:value/borrow#2][0m
    %37 = fn_ref $const_sig
    %38 = call %37(%4)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %4:value/borrow#0][0m
    %39 = const.int 4
    %40 = call %20(%39, %12, %38)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %39:value/borrow#0, %12:value/borrow#1, %38:value/borrow#2][0m
    %41 = fn_ref $const_sig
    %42 = call %41(%5)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %5:value/borrow#0][0m
    %43 = const.int 5
    %44 = call %20(%43, %12, %42)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %43:value/borrow#0, %12:value/borrow#1, %42:value/borrow#2][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn stutter.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 1536016
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 1536008
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 1536000
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Int[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 0
    %15 = store %13, %14[2m : ()[0m[2m ; ops [%13:value/borrow#0, %14:element/consume#1][0m
    %16 = const.int 0
    %17 = ptr_offset %0, %16[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %16:index/borrow#1][0m
    %18 = fn_ref $ylc_audio_memzero
    %19 = const.int 1536000
    %20 = call %18(%17, %19)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %17:value/borrow#0, %19:value/borrow#1][0m
    %21 = const.int 1536024
    %22 = ptr_offset %0, %21[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %21:index/borrow#1][0m
    %23 = primitive_cast %22 from Ptr of Char to Ptr of Double[2m ; ops [%22:value/borrow#0][0m
    %24 = const.double 0
    %25 = store %23, %24[2m : ()[0m[2m ; ops [%23:value/borrow#0, %24:element/consume#1][0m
    %26 = const.int 1536016
    %27 = ptr_offset %0, %26[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %26:index/borrow#1][0m
    %28 = primitive_cast %27 from Ptr of Char to Ptr of Double[2m ; ops [%27:value/borrow#0][0m
    %29 = const.double 0
    %30 = store %28, %29[2m : ()[0m[2m ; ops [%28:value/borrow#0, %29:element/consume#1][0m
    %31 = const.int 1536008
    %32 = ptr_offset %0, %31[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %31:index/borrow#1][0m
    %33 = primitive_cast %32 from Ptr of Char to Ptr of Double[2m ; ops [%32:value/borrow#0][0m
    %34 = const.double 0
    %35 = store %33, %34[2m : ()[0m[2m ; ops [%33:value/borrow#0, %34:element/consume#1][0m
    %36 = const.int 1536000
    %37 = ptr_offset %0, %36[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %36:index/borrow#1][0m
    %38 = primitive_cast %37 from Ptr of Char to Ptr of Int[2m ; ops [%37:value/borrow#0][0m
    %39 = const.int 0
    %40 = store %38, %39[2m : ()[0m[2m ; ops [%38:value/borrow#0, %39:element/consume#1][0m
    %41 = const.void
    return %41[2m ; ops [%41:return/consume#0][0m
}
fn stutter.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @borrow[0m, %4 trig_sig[2m: Double @consume[0m, %5 active[2m: Double @borrow[0m, %6 beat_length[2m: Double @borrow[0m, %7 subdiv[2m: Double @borrow[0m, %8 rate[2m: Double @borrow[0m, %9 inp[2m: Double @consume[0m)[2m -> Double @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = const.double 4
    %11 = const.int 0
    %12 = ptr_offset %1, %11[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %11:index/borrow#1][0m
    %13 = primitive_cast %12 from Ptr of Char to Ptr of Double[2m ; ops [%12:value/borrow#0][0m
    %14 = const.int 192000
    %15 = const.int 0
    %16 = construct.tuple { %14, %15, %13 }[2m : Array of Double[0m[2m ; ops [%14:field/borrow#0, %15:field/borrow#1, %13:field/borrow#2][0m
    %17 = extract.field %16, 0[2m : Int[0m[2m ; ops [%16:container/borrow#0][0m
    %18 = fmul %6, %7[2m ; ops [%6:value/borrow#0, %7:value/borrow#1][0m
    %19 = fn_ref $Math.max
    %20 = const.double 8
    %21 = fdiv %18, %3[2m ; ops [%18:value/borrow#0, %3:value/borrow#1][0m
    %22 = call %19 as $Math_max.Double.Double.Double(%20, %21)[2m : Double[0m[2m ; ops [%19:callee/borrow#0, %20:value/consume#0, %21:value/consume#1][0m
    %23 = const.int 1536000
    %24 = ptr_offset %1, %23[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %23:index/borrow#1][0m
    %25 = primitive_cast %24 from Ptr of Char to Ptr of Int[2m ; ops [%24:value/borrow#0][0m
    %26 = const.int 1
    %27 = const.int 0
    %28 = construct.tuple { %26, %27, %25 }[2m : Array of Int[0m[2m ; ops [%26:field/borrow#0, %27:field/borrow#1, %25:field/borrow#2][0m
    %29 = const.int 1536008
    %30 = ptr_offset %1, %29[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Double[2m ; ops [%30:value/borrow#0][0m
    %32 = const.int 1
    %33 = const.int 0
    %34 = construct.tuple { %32, %33, %31 }[2m : Array of Double[0m[2m ; ops [%32:field/borrow#0, %33:field/borrow#1, %31:field/borrow#2][0m
    %35 = const.int 1536016
    %36 = ptr_offset %1, %35[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %35:index/borrow#1][0m
    %37 = primitive_cast %36 from Ptr of Char to Ptr of Double[2m ; ops [%36:value/borrow#0][0m
    %38 = const.int 2
    %39 = const.int 0
    %40 = construct.tuple { %38, %39, %37 }[2m : Array of Double[0m[2m ; ops [%38:field/borrow#0, %39:field/borrow#1, %37:field/borrow#2][0m
    %41 = const.int 0
    %42 = extract.field %28, 1[2m : Int[0m[2m ; ops [%28:container/borrow#0][0m
    %43 = iadd %42, %41[2m ; ops [%42:value/borrow#0, %41:value/borrow#1][0m
    %44 = extract.field %28, 2[2m : Ptr[0m[2m ; ops [%28:container/borrow#0][0m
    %45 = ptr_offset %44, %43[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %43:index/borrow#1][0m
    %46 = load %45[2m : Int[0m[2m ; ops [%45:value/borrow#0][0m
    %47 = const.int 0
    %48 = extract.field %34, 1[2m : Int[0m[2m ; ops [%34:container/borrow#0][0m
    %49 = iadd %48, %47[2m ; ops [%48:value/borrow#0, %47:value/borrow#1][0m
    %50 = extract.field %34, 2[2m : Ptr[0m[2m ; ops [%34:container/borrow#0][0m
    %51 = ptr_offset %50, %49[2m : Ptr[0m[2m ; ops [%50:value/borrow#0, %49:index/borrow#1][0m
    %52 = load %51[2m : Double[0m[2m ; ops [%51:value/borrow#0][0m
    %53 = const.int 0
    %54 = extract.field %40, 1[2m : Int[0m[2m ; ops [%40:container/borrow#0][0m
    %55 = iadd %54, %53[2m ; ops [%54:value/borrow#0, %53:value/borrow#1][0m
    %56 = extract.field %40, 2[2m : Ptr[0m[2m ; ops [%40:container/borrow#0][0m
    %57 = ptr_offset %56, %55[2m : Ptr[0m[2m ; ops [%56:value/borrow#0, %55:index/borrow#1][0m
    %58 = load %57[2m : Double[0m[2m ; ops [%57:value/borrow#0][0m
    %59 = extract.field %16, 1[2m : Int[0m[2m ; ops [%16:container/borrow#0][0m
    %60 = iadd %59, %46[2m ; ops [%59:value/borrow#0, %46:value/borrow#1][0m
    %61 = extract.field %16, 2[2m : Ptr[0m[2m ; ops [%16:container/borrow#0][0m
    %62 = ptr_offset %61, %60[2m : Ptr[0m[2m ; ops [%61:value/borrow#0, %60:index/borrow#1][0m
    %63 = store %62, %9[2m : ()[0m[2m ; ops [%62:value/borrow#0, %9:element/consume#1][0m
    %64 = fn_ref $$builtin.op_eq
    %65 = const.double 1
    %66 = call %64 as $builtin_op_eq.Double.Double.Bool(%4, %65)[2m : Bool[0m[2m ; ops [%64:callee/borrow#0, %4:value/borrow#0, %65:value/borrow#1][0m
    %67 = primitive_cast %66 from Bool to Double[2m ; ops [%66:value/borrow#0][0m
    %68 = primitive_cast %46 from Int to Double[2m ; ops [%46:value/borrow#0][0m
    %69 = fmul %67, %68[2m ; ops [%67:value/borrow#0, %68:value/borrow#1][0m
    %70 = const.double 1
    %71 = fsub %70, %67[2m ; ops [%70:value/borrow#0, %67:value/borrow#1][0m
    %72 = fmul %71, %52[2m ; ops [%71:value/borrow#0, %52:value/borrow#1][0m
    %73 = fadd %69, %72[2m ; ops [%69:value/borrow#0, %72:value/borrow#1][0m
    %74 = const.double 1
    %75 = fsub %22, %74[2m ; ops [%22:value/borrow#0, %74:value/borrow#1][0m
    %76 = fmul %67, %75[2m ; ops [%67:value/borrow#0, %75:value/borrow#1][0m
    %77 = const.double 1
    %78 = fsub %77, %67[2m ; ops [%77:value/borrow#0, %67:value/borrow#1][0m
    %79 = fmul %78, %58[2m ; ops [%78:value/borrow#0, %58:value/borrow#1][0m
    %80 = fadd %76, %79[2m ; ops [%76:value/borrow#0, %79:value/borrow#1][0m
    %81 = fsub %73, %22[2m ; ops [%73:value/borrow#0, %22:value/borrow#1][0m
    %82 = const.double 1
    %83 = fadd %81, %82[2m ; ops [%81:value/borrow#0, %82:value/borrow#1][0m
    %84 = fadd %83, %80[2m ; ops [%83:value/borrow#0, %80:value/borrow#1][0m
    %85 = extract.field %16, 0[2m : Int[0m[2m ; ops [%16:container/borrow#0][0m
    %86 = extract.field %16, 1[2m : Int[0m[2m ; ops [%16:container/borrow#0][0m
    %87 = extract.field %16, 2[2m : Ptr[0m[2m ; ops [%16:container/borrow#0][0m
    %88 = fn_ref $ylc_audio_tabread_samp_kernel
    %89 = primitive_cast %1 from Ptr of Char to Ptr of Double[2m ; ops [%1:value/borrow#0][0m
    %90 = call %88(%89, %3, %85, %86, %87, %84)[2m : Double[0m[2m ; ops [%88:callee/borrow#0, %89:value/borrow#0, %3:value/borrow#1, %85:value/borrow#2, %86:value/borrow#3, %87:value/borrow#4, %84:value/borrow#5][0m
    %91 = fn_ref $$builtin.op_eq
    %92 = const.double 1
    %93 = call %91 as $builtin_op_eq.Double.Double.Bool(%5, %92)[2m : Bool[0m[2m ; ops [%91:callee/borrow#0, %5:value/borrow#0, %92:value/borrow#1][0m
    %94 = primitive_cast %93 from Bool to Double[2m ; ops [%93:value/borrow#0][0m
    %95 = fmul %94, %90[2m ; ops [%94:value/borrow#0, %90:value/borrow#1][0m
    %96 = const.double 1
    %97 = fsub %96, %94[2m ; ops [%96:value/borrow#0, %94:value/borrow#1][0m
    %98 = fmul %97, %9[2m ; ops [%97:value/borrow#0, %9:value/borrow#1][0m
    %99 = fadd %95, %98[2m ; ops [%95:value/borrow#0, %98:value/borrow#1][0m
    %100 = fadd %80, %8[2m ; ops [%80:value/borrow#0, %8:value/borrow#1][0m
    %101 = fn_ref $floor
    %102 = fdiv %100, %22[2m ; ops [%100:value/borrow#0, %22:value/borrow#1][0m
    %103 = call %101(%102)[2m : Double[0m[2m ; ops [%101:callee/borrow#0, %102:value/borrow#0][0m
    %104 = fmul %22, %103[2m ; ops [%22:value/borrow#0, %103:value/borrow#1][0m
    %105 = fsub %100, %104[2m ; ops [%100:value/borrow#0, %104:value/borrow#1][0m
    %106 = const.int 0
    %107 = extract.field %34, 1[2m : Int[0m[2m ; ops [%34:container/borrow#0][0m
    %108 = iadd %107, %106[2m ; ops [%107:value/borrow#0, %106:value/borrow#1][0m
    %109 = extract.field %34, 2[2m : Ptr[0m[2m ; ops [%34:container/borrow#0][0m
    %110 = ptr_offset %109, %108[2m : Ptr[0m[2m ; ops [%109:value/borrow#0, %108:index/borrow#1][0m
    %111 = store %110, %73[2m : ()[0m[2m ; ops [%110:value/borrow#0, %73:element/consume#1][0m
    %112 = const.double 0.5
    %113 = fgte %5, %112[2m ; ops [%5:value/borrow#0, %112:value/borrow#1][0m
    %114 = primitive_cast %113 from Bool to Double[2m ; ops [%113:value/borrow#0][0m
    %115 = const.int 0
    %116 = fmul %114, %105[2m ; ops [%114:value/borrow#0, %105:value/borrow#1][0m
    %117 = const.double 1
    %118 = fsub %117, %114[2m ; ops [%117:value/borrow#0, %114:value/borrow#1][0m
    %119 = fmul %118, %58[2m ; ops [%118:value/borrow#0, %58:value/borrow#1][0m
    %120 = fadd %116, %119[2m ; ops [%116:value/borrow#0, %119:value/borrow#1][0m
    %121 = extract.field %40, 1[2m : Int[0m[2m ; ops [%40:container/borrow#0][0m
    %122 = iadd %121, %115[2m ; ops [%121:value/borrow#0, %115:value/borrow#1][0m
    %123 = extract.field %40, 2[2m : Ptr[0m[2m ; ops [%40:container/borrow#0][0m
    %124 = ptr_offset %123, %122[2m : Ptr[0m[2m ; ops [%123:value/borrow#0, %122:index/borrow#1][0m
    %125 = store %124, %120[2m : ()[0m[2m ; ops [%124:value/borrow#0, %120:element/consume#1][0m
    %126 = const.int 1
    %127 = extract.field %40, 1[2m : Int[0m[2m ; ops [%40:container/borrow#0][0m
    %128 = iadd %127, %126[2m ; ops [%127:value/borrow#0, %126:value/borrow#1][0m
    %129 = extract.field %40, 2[2m : Ptr[0m[2m ; ops [%40:container/borrow#0][0m
    %130 = ptr_offset %129, %128[2m : Ptr[0m[2m ; ops [%129:value/borrow#0, %128:index/borrow#1][0m
    %131 = store %130, %4[2m : ()[0m[2m ; ops [%130:value/borrow#0, %4:element/consume#1][0m
    %132 = const.int 0
    %133 = const.int 1
    %134 = primitive_cast %46 from Int to Double[2m ; ops [%46:value/borrow#0][0m
    %135 = primitive_cast %133 from Int to Double[2m ; ops [%133:value/borrow#0][0m
    %136 = fadd %134, %135[2m ; ops [%134:value/borrow#0, %135:value/borrow#1][0m
    %137 = primitive_cast %17 from Int to Double[2m ; ops [%17:value/borrow#0][0m
    %138 = fmod %136, %137[2m ; ops [%136:value/borrow#0, %137:value/borrow#1][0m
    %139 = extract.field %28, 1[2m : Int[0m[2m ; ops [%28:container/borrow#0][0m
    %140 = iadd %139, %132[2m ; ops [%139:value/borrow#0, %132:value/borrow#1][0m
    %141 = primitive_cast %138 from Double to Int[2m ; ops [%138:value/borrow#0][0m
    %142 = extract.field %28, 2[2m : Ptr[0m[2m ; ops [%28:container/borrow#0][0m
    %143 = ptr_offset %142, %140[2m : Ptr[0m[2m ; ops [%142:value/borrow#0, %140:index/borrow#1][0m
    %144 = store %143, %141[2m : ()[0m[2m ; ops [%143:value/borrow#0, %141:element/consume#1][0m
    return %99[2m ; ops [%99:return/consume#0][0m
}
fn stutter.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = const.int 4
    %30 = ptr_offset %2, %29[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %29:index/borrow#1][0m
    %31 = load %30[2m : Ptr[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fn_ref $ylc_read_inlet_node_lane_i32
    %33 = const.int 0
    %34 = call %32(%31, %3, %33)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %33:value/borrow#2][0m
    %35 = const.int 5
    %36 = ptr_offset %2, %35[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %35:index/borrow#1][0m
    %37 = load %36[2m : Ptr[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = fn_ref $ylc_read_inlet_node_lane_i32
    %39 = const.int 0
    %40 = call %38(%37, %3, %39)[2m : Double[0m[2m ; ops [%38:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %39:value/borrow#2][0m
    %41 = fn_ref $stutter.kernel
    %42 = call %41(%0, %1, %3, %4, %10, %16, %22, %28, %34, %40)[2m : Double[0m[2m ; ops [%41:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/borrow#3, %10:value/consume#4, %16:value/borrow#5, %22:value/borrow#6, %28:value/borrow#7, %34:value/borrow#8, %40:value/consume#9][0m
    %43 = fn_ref $ylc_get_output_buf
    %44 = call %43(%0)[2m : Ptr[0m[2m ; ops [%43:callee/borrow#0, %0:value/borrow#0][0m
    %45 = ptr_offset %44, %3[2m : Ptr[0m[2m ; ops [%44:value/borrow#0, %3:index/borrow#1][0m
    %46 = store %45, %42[2m : ()[0m[2m ; ops [%45:value/borrow#0, %42:element/consume#1][0m
    %47 = const.void
    return %47[2m ; ops [%47:return/consume#0][0m
}
fn Math_max.Double.Double.Double(%0 a[2m: Double @consume[0m, %1 b[2m: Double @consume[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %2 = fgt %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %3 = phi [bb4: %0, bb6: %1][2m : Double[0m[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %3[2m ; ops [%3:return/consume#0][0m
  bb3 match.arm.0.test:
    %4 = const.bool true
    %5 = beq %2, %4[2m ; ops [%2:value/borrow#0, %4:value/borrow#1][0m
    cond %5, bb4, bb5[2m ; ops [%5:condition/borrow#0][0m
  bb4 match.arm.0.body:
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    br bb2
}
fn $builtin.op_eq(%0 arg0[2m: `42 @borrow[0m, %1 arg1[2m: `43 @borrow[0m)[2m -> Bool @owned[0m {
  bb0 entry:
    %2 = call @==(%0, %1)[2m : Bool[0m[2m ; ops [_:callee/borrow#0, %0:value/borrow#0, %1:value/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn builtin_op_eq.Double.Double.Bool(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Bool @owned[0m {
  bb0 entry:
    %2 = feq %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    return %2[2m ; ops [%2:return/consume#0][0m
}
fn fx.cons(%0 sel[2m: Double @borrow[0m, %1 trigger[2m: Double @borrow[0m, %2 subdiv[2m: Double @borrow[0m, %3 beat_length[2m: Double @borrow[0m, %4 rate[2m: Double @borrow[0m, %5 inpp[2m: Double @borrow[0m)[2m -> Ptr @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %6 = fn_ref $ylc_create_audio_frame_node
    %7 = fn_ref $fx.frame
    %8 = const.string "fx"
    %9 = const.int 6
    %10 = const.int 2
    %11 = const.int 3267608
    %12 = call %6(%7, %9, %10, %11, %8)[2m : Ptr[0m[2m ; ops [%6:callee/borrow#0, %7:value/borrow#0, %9:value/borrow#1, %10:value/borrow#2, %11:value/borrow#3, %8:value/borrow#4][0m
    %13 = fn_ref $ylc_audio_node_set_state_init
    %14 = fn_ref $fx.init
    %15 = call %13(%12, %14)[2m : ()[0m[2m ; ops [%13:callee/borrow#0, %12:value/borrow#0, %14:value/borrow#1][0m
    %16 = fn_ref $ylc_audio_node_inline_state
    %17 = call %16(%12)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %12:value/borrow#0][0m
    %18 = fn_ref $fx.init
    %19 = call %18(%17)[2m : ()[0m[2m ; ops [%18:callee/borrow#0, %17:value/borrow#0][0m
    %20 = fn_ref $node_connect_input
    %21 = fn_ref $const_sig
    %22 = call %21(%0)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %0:value/borrow#0][0m
    %23 = const.int 0
    %24 = call %20(%23, %12, %22)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %23:value/borrow#0, %12:value/borrow#1, %22:value/borrow#2][0m
    %25 = fn_ref $const_sig
    %26 = call %25(%1)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %1:value/borrow#0][0m
    %27 = const.int 1
    %28 = call %20(%27, %12, %26)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %27:value/borrow#0, %12:value/borrow#1, %26:value/borrow#2][0m
    %29 = fn_ref $const_sig
    %30 = call %29(%2)[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0, %2:value/borrow#0][0m
    %31 = const.int 2
    %32 = call %20(%31, %12, %30)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %31:value/borrow#0, %12:value/borrow#1, %30:value/borrow#2][0m
    %33 = fn_ref $const_sig
    %34 = call %33(%3)[2m : Ptr[0m[2m ; ops [%33:callee/borrow#0, %3:value/borrow#0][0m
    %35 = const.int 3
    %36 = call %20(%35, %12, %34)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %35:value/borrow#0, %12:value/borrow#1, %34:value/borrow#2][0m
    %37 = fn_ref $const_sig
    %38 = call %37(%4)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %4:value/borrow#0][0m
    %39 = const.int 4
    %40 = call %20(%39, %12, %38)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %39:value/borrow#0, %12:value/borrow#1, %38:value/borrow#2][0m
    %41 = fn_ref $const_sig
    %42 = call %41(%5)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %5:value/borrow#0][0m
    %43 = const.int 5
    %44 = call %20(%43, %12, %42)[2m : ()[0m[2m ; ops [%20:callee/borrow#0, %43:value/borrow#0, %12:value/borrow#1, %42:value/borrow#2][0m
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn fx.init(%0 state[2m: Ptr @borrow[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %1 = const.int 3267584
    %2 = ptr_offset %0, %1[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %1:index/borrow#1][0m
    %3 = primitive_cast %2 from Ptr of Char to Ptr of Double[2m ; ops [%2:value/borrow#0][0m
    %4 = const.double 0
    %5 = store %3, %4[2m : ()[0m[2m ; ops [%3:value/borrow#0, %4:element/consume#1][0m
    %6 = const.int 3267560
    %7 = ptr_offset %0, %6[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %6:index/borrow#1][0m
    %8 = primitive_cast %7 from Ptr of Char to Ptr of Double[2m ; ops [%7:value/borrow#0][0m
    %9 = const.double 0
    %10 = store %8, %9[2m : ()[0m[2m ; ops [%8:value/borrow#0, %9:element/consume#1][0m
    %11 = const.int 1731528
    %12 = ptr_offset %0, %11[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %11:index/borrow#1][0m
    %13 = const.char '\x00'
    %14 = store %12, %13[2m : ()[0m[2m ; ops [%12:value/borrow#0, %13:element/consume#1][0m
    %15 = const.int 195496
    %16 = ptr_offset %0, %15[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %15:index/borrow#1][0m
    %17 = const.char '\x00'
    %18 = store %16, %17[2m : ()[0m[2m ; ops [%16:value/borrow#0, %17:element/consume#1][0m
    %19 = const.int 195488
    %20 = ptr_offset %0, %19[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %19:index/borrow#1][0m
    %21 = primitive_cast %20 from Ptr of Char to Ptr of Double[2m ; ops [%20:value/borrow#0][0m
    %22 = const.double 0
    %23 = store %21, %22[2m : ()[0m[2m ; ops [%21:value/borrow#0, %22:element/consume#1][0m
    %24 = const.int 195480
    %25 = ptr_offset %0, %24[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %24:index/borrow#1][0m
    %26 = primitive_cast %25 from Ptr of Char to Ptr of Double[2m ; ops [%25:value/borrow#0][0m
    %27 = const.double 0
    %28 = store %26, %27[2m : ()[0m[2m ; ops [%26:value/borrow#0, %27:element/consume#1][0m
    %29 = const.int 195448
    %30 = ptr_offset %0, %29[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %29:index/borrow#1][0m
    %31 = primitive_cast %30 from Ptr of Char to Ptr of Double[2m ; ops [%30:value/borrow#0][0m
    %32 = const.double 0
    %33 = store %31, %32[2m : ()[0m[2m ; ops [%31:value/borrow#0, %32:element/consume#1][0m
    %34 = const.int 195416
    %35 = ptr_offset %0, %34[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %34:index/borrow#1][0m
    %36 = primitive_cast %35 from Ptr of Char to Ptr of Double[2m ; ops [%35:value/borrow#0][0m
    %37 = const.double 0
    %38 = store %36, %37[2m : ()[0m[2m ; ops [%36:value/borrow#0, %37:element/consume#1][0m
    %39 = const.int 195384
    %40 = ptr_offset %0, %39[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %39:index/borrow#1][0m
    %41 = primitive_cast %40 from Ptr of Char to Ptr of Double[2m ; ops [%40:value/borrow#0][0m
    %42 = const.double 0
    %43 = store %41, %42[2m : ()[0m[2m ; ops [%41:value/borrow#0, %42:element/consume#1][0m
    %44 = const.int 195352
    %45 = ptr_offset %0, %44[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %44:index/borrow#1][0m
    %46 = primitive_cast %45 from Ptr of Char to Ptr of Double[2m ; ops [%45:value/borrow#0][0m
    %47 = const.double 0
    %48 = store %46, %47[2m : ()[0m[2m ; ops [%46:value/borrow#0, %47:element/consume#1][0m
    %49 = const.int 195320
    %50 = ptr_offset %0, %49[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %49:index/borrow#1][0m
    %51 = const.char '\x00'
    %52 = store %50, %51[2m : ()[0m[2m ; ops [%50:value/borrow#0, %51:element/consume#1][0m
    %53 = const.int 195288
    %54 = ptr_offset %0, %53[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %53:index/borrow#1][0m
    %55 = const.char '\x00'
    %56 = store %54, %55[2m : ()[0m[2m ; ops [%54:value/borrow#0, %55:element/consume#1][0m
    %57 = const.int 193240
    %58 = ptr_offset %0, %57[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %57:index/borrow#1][0m
    %59 = primitive_cast %58 from Ptr of Char to Ptr of Double[2m ; ops [%58:value/borrow#0][0m
    %60 = const.double 0
    %61 = store %59, %60[2m : ()[0m[2m ; ops [%59:value/borrow#0, %60:element/consume#1][0m
    %62 = const.int 191192
    %63 = ptr_offset %0, %62[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %62:index/borrow#1][0m
    %64 = primitive_cast %63 from Ptr of Char to Ptr of Double[2m ; ops [%63:value/borrow#0][0m
    %65 = const.double 0
    %66 = store %64, %65[2m : ()[0m[2m ; ops [%64:value/borrow#0, %65:element/consume#1][0m
    %67 = const.int 152768
    %68 = ptr_offset %0, %67[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %67:index/borrow#1][0m
    %69 = fn_ref $ylc_audio_memzero
    %70 = const.int 38424
    %71 = call %69(%68, %70)[2m : ()[0m[2m ; ops [%69:callee/borrow#0, %68:value/borrow#0, %70:value/borrow#1][0m
    %72 = const.int 114344
    %73 = ptr_offset %0, %72[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %72:index/borrow#1][0m
    %74 = fn_ref $ylc_audio_memzero
    %75 = const.int 38424
    %76 = call %74(%73, %75)[2m : ()[0m[2m ; ops [%74:callee/borrow#0, %73:value/borrow#0, %75:value/borrow#1][0m
    %77 = const.int 114304
    %78 = ptr_offset %0, %77[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %77:index/borrow#1][0m
    %79 = primitive_cast %78 from Ptr of Char to Ptr of Double[2m ; ops [%78:value/borrow#0][0m
    %80 = const.double 0
    %81 = store %79, %80[2m : ()[0m[2m ; ops [%79:value/borrow#0, %80:element/consume#1][0m
    %82 = const.int 97752
    %83 = ptr_offset %0, %82[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %82:index/borrow#1][0m
    %84 = fn_ref $ylc_audio_memzero
    %85 = const.int 16552
    %86 = call %84(%83, %85)[2m : ()[0m[2m ; ops [%84:callee/borrow#0, %83:value/borrow#0, %85:value/borrow#1][0m
    %87 = const.int 81200
    %88 = ptr_offset %0, %87[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %87:index/borrow#1][0m
    %89 = fn_ref $ylc_audio_memzero
    %90 = const.int 16552
    %91 = call %89(%88, %90)[2m : ()[0m[2m ; ops [%89:callee/borrow#0, %88:value/borrow#0, %90:value/borrow#1][0m
    %92 = const.int 81160
    %93 = ptr_offset %0, %92[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %92:index/borrow#1][0m
    %94 = primitive_cast %93 from Ptr of Char to Ptr of Double[2m ; ops [%93:value/borrow#0][0m
    %95 = const.double 0
    %96 = store %94, %95[2m : ()[0m[2m ; ops [%94:value/borrow#0, %95:element/consume#1][0m
    %97 = const.int 81136
    %98 = ptr_offset %0, %97[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %97:index/borrow#1][0m
    %99 = fn_ref $ylc_audio_memzero
    %100 = const.int 24
    %101 = call %99(%98, %100)[2m : ()[0m[2m ; ops [%99:callee/borrow#0, %98:value/borrow#0, %100:value/borrow#1][0m
    %102 = const.int 81072
    %103 = ptr_offset %0, %102[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %102:index/borrow#1][0m
    %104 = primitive_cast %103 from Ptr of Char to Ptr of Double[2m ; ops [%103:value/borrow#0][0m
    %105 = const.double 0
    %106 = store %104, %105[2m : ()[0m[2m ; ops [%104:value/borrow#0, %105:element/consume#1][0m
    %107 = const.int 81048
    %108 = ptr_offset %0, %107[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %107:index/borrow#1][0m
    %109 = primitive_cast %108 from Ptr of Char to Ptr of Double[2m ; ops [%108:value/borrow#0][0m
    %110 = const.double 0
    %111 = store %109, %110[2m : ()[0m[2m ; ops [%109:value/borrow#0, %110:element/consume#1][0m
    %112 = const.int 81024
    %113 = ptr_offset %0, %112[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %112:index/borrow#1][0m
    %114 = primitive_cast %113 from Ptr of Char to Ptr of Double[2m ; ops [%113:value/borrow#0][0m
    %115 = const.double 0
    %116 = store %114, %115[2m : ()[0m[2m ; ops [%114:value/borrow#0, %115:element/consume#1][0m
    %117 = const.int 81008
    %118 = ptr_offset %0, %117[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %117:index/borrow#1][0m
    %119 = primitive_cast %118 from Ptr of Char to Ptr of Double[2m ; ops [%118:value/borrow#0][0m
    %120 = const.double 0
    %121 = store %119, %120[2m : ()[0m[2m ; ops [%119:value/borrow#0, %120:element/consume#1][0m
    %122 = const.int 80984
    %123 = ptr_offset %0, %122[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %122:index/borrow#1][0m
    %124 = primitive_cast %123 from Ptr of Char to Ptr of Double[2m ; ops [%123:value/borrow#0][0m
    %125 = const.double 0
    %126 = store %124, %125[2m : ()[0m[2m ; ops [%124:value/borrow#0, %125:element/consume#1][0m
    %127 = const.int 78936
    %128 = ptr_offset %0, %127[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %127:index/borrow#1][0m
    %129 = primitive_cast %128 from Ptr of Char to Ptr of Double[2m ; ops [%128:value/borrow#0][0m
    %130 = const.double 0
    %131 = store %129, %130[2m : ()[0m[2m ; ops [%129:value/borrow#0, %130:element/consume#1][0m
    %132 = const.int 76888
    %133 = ptr_offset %0, %132[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %132:index/borrow#1][0m
    %134 = primitive_cast %133 from Ptr of Char to Ptr of Double[2m ; ops [%133:value/borrow#0][0m
    %135 = const.double 0
    %136 = store %134, %135[2m : ()[0m[2m ; ops [%134:value/borrow#0, %135:element/consume#1][0m
    %137 = const.int 38464
    %138 = ptr_offset %0, %137[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %137:index/borrow#1][0m
    %139 = fn_ref $ylc_audio_memzero
    %140 = const.int 38424
    %141 = call %139(%138, %140)[2m : ()[0m[2m ; ops [%139:callee/borrow#0, %138:value/borrow#0, %140:value/borrow#1][0m
    %142 = const.int 40
    %143 = ptr_offset %0, %142[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %142:index/borrow#1][0m
    %144 = fn_ref $ylc_audio_memzero
    %145 = const.int 38424
    %146 = call %144(%143, %145)[2m : ()[0m[2m ; ops [%144:callee/borrow#0, %143:value/borrow#0, %145:value/borrow#1][0m
    %147 = const.int 0
    %148 = ptr_offset %0, %147[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %147:index/borrow#1][0m
    %149 = primitive_cast %148 from Ptr of Char to Ptr of Double[2m ; ops [%148:value/borrow#0][0m
    %150 = const.double 0
    %151 = store %149, %150[2m : ()[0m[2m ; ops [%149:value/borrow#0, %150:element/consume#1][0m
    %152 = const.int 81128
    %153 = ptr_offset %0, %152[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %152:index/borrow#1][0m
    %154 = primitive_cast %153 from Ptr of Char to Ptr of Double[2m ; ops [%153:value/borrow#0][0m
    %155 = const.double 1.5
    %156 = store %154, %155[2m : ()[0m[2m ; ops [%154:value/borrow#0, %155:element/consume#1][0m
    %157 = const.int 81120
    %158 = ptr_offset %0, %157[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %157:index/borrow#1][0m
    %159 = primitive_cast %158 from Ptr of Char to Ptr of Double[2m ; ops [%158:value/borrow#0][0m
    %160 = const.double 4
    %161 = store %159, %160[2m : ()[0m[2m ; ops [%159:value/borrow#0, %160:element/consume#1][0m
    %162 = const.int 81112
    %163 = ptr_offset %0, %162[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %162:index/borrow#1][0m
    %164 = primitive_cast %163 from Ptr of Char to Ptr of Double[2m ; ops [%163:value/borrow#0][0m
    %165 = const.double 2
    %166 = store %164, %165[2m : ()[0m[2m ; ops [%164:value/borrow#0, %165:element/consume#1][0m
    %167 = const.int 81104
    %168 = ptr_offset %0, %167[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %167:index/borrow#1][0m
    %169 = primitive_cast %168 from Ptr of Char to Ptr of Double[2m ; ops [%168:value/borrow#0][0m
    %170 = const.double 2
    %171 = store %169, %170[2m : ()[0m[2m ; ops [%169:value/borrow#0, %170:element/consume#1][0m
    %172 = const.int 81096
    %173 = ptr_offset %0, %172[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %172:index/borrow#1][0m
    %174 = primitive_cast %173 from Ptr of Char to Ptr of Double[2m ; ops [%173:value/borrow#0][0m
    %175 = const.double 1.5
    %176 = store %174, %175[2m : ()[0m[2m ; ops [%174:value/borrow#0, %175:element/consume#1][0m
    %177 = const.int 81088
    %178 = ptr_offset %0, %177[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %177:index/borrow#1][0m
    %179 = primitive_cast %178 from Ptr of Char to Ptr of Double[2m ; ops [%178:value/borrow#0][0m
    %180 = const.double 1.2
    %181 = store %179, %180[2m : ()[0m[2m ; ops [%179:value/borrow#0, %180:element/consume#1][0m
    %182 = const.int 81080
    %183 = ptr_offset %0, %182[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %182:index/borrow#1][0m
    %184 = primitive_cast %183 from Ptr of Char to Ptr of Double[2m ; ops [%183:value/borrow#0][0m
    %185 = const.double 1
    %186 = store %184, %185[2m : ()[0m[2m ; ops [%184:value/borrow#0, %185:element/consume#1][0m
    %187 = const.int 81072
    %188 = ptr_offset %0, %187[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %187:index/borrow#1][0m
    %189 = primitive_cast %188 from Ptr of Char to Ptr of Double[2m ; ops [%188:value/borrow#0][0m
    %190 = const.double 0.5
    %191 = store %189, %190[2m : ()[0m[2m ; ops [%189:value/borrow#0, %190:element/consume#1][0m
    %192 = const.int 195288
    %193 = ptr_offset %0, %192[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %192:index/borrow#1][0m
    %194 = fn_ref $lpf.init
    %195 = call %194(%193)[2m : ()[0m[2m ; ops [%194:callee/borrow#0, %193:value/borrow#0][0m
    %196 = const.int 195320
    %197 = ptr_offset %0, %196[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %196:index/borrow#1][0m
    %198 = call %194(%197)[2m : ()[0m[2m ; ops [%194:callee/borrow#0, %197:value/borrow#0][0m
    %199 = const.int 195496
    %200 = ptr_offset %0, %199[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %199:index/borrow#1][0m
    %201 = fn_ref $stutter.init
    %202 = call %201(%200)[2m : ()[0m[2m ; ops [%201:callee/borrow#0, %200:value/borrow#0][0m
    %203 = const.int 1731528
    %204 = ptr_offset %0, %203[2m : Ptr[0m[2m ; ops [%0:value/borrow#0, %203:index/borrow#1][0m
    %205 = call %201(%204)[2m : ()[0m[2m ; ops [%201:callee/borrow#0, %204:value/borrow#0][0m
    %206 = const.void
    return %206[2m ; ops [%206:return/consume#0][0m
}
fn fx.kernel(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 frame[2m: Int @consume[0m, %3 spf[2m: Double @consume[0m, %4 sel[2m: Double @borrow[0m, %5 trigger[2m: Double @consume[0m, %6 subdiv[2m: Double @borrow[0m, %7 beat_length[2m: Double @borrow[0m, %8 rate[2m: Double @borrow[0m, %9 inpp[2m: (Double, Double, ) @borrow[0m)[2m -> (Double, Double, ) @owned[0m[2m @skip_rc[0m {
  bb0 entry:
    %10 = extract.field %9, 0[2m : Double[0m[2m ; ops [%9:container/borrow#0][0m
    %11 = extract.field %9, 1[2m : Double[0m[2m ; ops [%9:container/borrow#0][0m
    %12 = const.double 100
    %13 = const.double 1
    %14 = const.double 1
    %15 = const.double 1
    %16 = const.double 0.01
    %17 = const.double 0.025
    %18 = fn_ref $ylc_audio_lfnoise_kernel
    %19 = const.int 0
    %20 = ptr_offset %1, %19[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %19:index/borrow#1][0m
    %21 = primitive_cast %20 from Ptr of Char to Ptr of Double[2m ; ops [%20:value/borrow#0][0m
    %22 = call %18(%21, %3, %15, %16, %17)[2m : Double[0m[2m ; ops [%18:callee/borrow#0, %21:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2, %16:value/borrow#3, %17:value/borrow#4][0m
    %23 = const.double 0.1
    %24 = const.double 0.5
    %25 = fn_ref $ylc_audio_comb_kernel
    %26 = const.int 40
    %27 = ptr_offset %1, %26[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %26:index/borrow#1][0m
    %28 = const.int 4802
    %29 = call %25(%27, %3, %28, %22, %24, %10)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %27:value/borrow#0, %3:value/borrow#1, %28:value/borrow#2, %22:value/borrow#3, %24:value/borrow#4, %10:value/borrow#5][0m
    %30 = const.int 38464
    %31 = ptr_offset %1, %30[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %30:index/borrow#1][0m
    %32 = const.int 4802
    %33 = call %25(%31, %3, %32, %22, %24, %11)[2m : Double[0m[2m ; ops [%25:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %32:value/borrow#2, %22:value/borrow#3, %24:value/borrow#4, %11:value/borrow#5][0m
    %34 = construct.tuple { %29, %33 }[2m : (Double, Double, )[0m[2m ; ops [%29:field/consume#0, %33:field/consume#1][0m
    %35 = fn_ref $ylc_audio_disperser_kernel
    %36 = const.int 76888
    %37 = ptr_offset %1, %36[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %36:index/borrow#1][0m
    %38 = primitive_cast %37 from Ptr of Char to Ptr of Double[2m ; ops [%37:value/borrow#0][0m
    %39 = call %35(%38, %3, %12, %13, %14, %29)[2m : Double[0m[2m ; ops [%35:callee/borrow#0, %38:value/borrow#0, %3:value/borrow#1, %12:value/borrow#2, %13:value/borrow#3, %14:value/borrow#4, %29:value/borrow#5][0m
    %40 = const.int 78936
    %41 = ptr_offset %1, %40[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %40:index/borrow#1][0m
    %42 = primitive_cast %41 from Ptr of Char to Ptr of Double[2m ; ops [%41:value/borrow#0][0m
    %43 = call %35(%42, %3, %12, %13, %14, %33)[2m : Double[0m[2m ; ops [%35:callee/borrow#0, %42:value/borrow#0, %3:value/borrow#1, %12:value/borrow#2, %13:value/borrow#3, %14:value/borrow#4, %33:value/borrow#5][0m
    %44 = construct.tuple { %39, %43 }[2m : (Double, Double, )[0m[2m ; ops [%39:field/consume#0, %43:field/consume#1][0m
    %45 = const.double 4000
    %46 = const.double 8000
    %47 = fn_ref $ylc_audio_rand_trig_kernel
    %48 = const.int 80984
    %49 = ptr_offset %1, %48[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %48:index/borrow#1][0m
    %50 = primitive_cast %49 from Ptr of Char to Ptr of Double[2m ; ops [%49:value/borrow#0][0m
    %51 = call %47(%50, %3, %45, %46, %5)[2m : Double[0m[2m ; ops [%47:callee/borrow#0, %50:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2, %46:value/borrow#3, %5:value/borrow#4][0m
    %52 = const.int 81008
    %53 = ptr_offset %1, %52[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %52:index/borrow#1][0m
    %54 = primitive_cast %53 from Ptr of Char to Ptr of Double[2m ; ops [%53:value/borrow#0][0m
    %55 = fn_ref $ylc_audio_trig_kernel
    %56 = call %55(%54, %3, %51)[2m : Double[0m[2m ; ops [%55:callee/borrow#0, %54:value/borrow#0, %3:value/borrow#1, %51:value/borrow#2][0m
    %57 = fn_ref $ylc_audio_sah_kernel
    %58 = const.int 81024
    %59 = ptr_offset %1, %58[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %58:index/borrow#1][0m
    %60 = primitive_cast %59 from Ptr of Char to Ptr of Double[2m ; ops [%59:value/borrow#0][0m
    %61 = call %57(%60, %3, %10, %56)[2m : Double[0m[2m ; ops [%57:callee/borrow#0, %60:value/borrow#0, %3:value/borrow#1, %10:value/borrow#2, %56:value/borrow#3][0m
    %62 = const.int 81048
    %63 = ptr_offset %1, %62[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %62:index/borrow#1][0m
    %64 = primitive_cast %63 from Ptr of Char to Ptr of Double[2m ; ops [%63:value/borrow#0][0m
    %65 = call %57(%64, %3, %11, %56)[2m : Double[0m[2m ; ops [%57:callee/borrow#0, %64:value/borrow#0, %3:value/borrow#1, %11:value/borrow#2, %56:value/borrow#3][0m
    %66 = construct.tuple { %61, %65 }[2m : (Double, Double, )[0m[2m ; ops [%61:field/consume#0, %65:field/consume#1][0m
    %67 = fn_ref $db_amp
    %68 = const.double 4
    %69 = call %67(%68)[2m : Double[0m[2m ; ops [%67:callee/borrow#0, %68:value/borrow#0][0m
    %70 = const.double 0.01
    %71 = const.int 81072
    %72 = ptr_offset %1, %71[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %71:index/borrow#1][0m
    %73 = primitive_cast %72 from Ptr of Char to Ptr of Double[2m ; ops [%72:value/borrow#0][0m
    %74 = const.int 8
    %75 = const.int 0
    %76 = construct.tuple { %74, %75, %73 }[2m : Array of Double[0m[2m ; ops [%74:field/borrow#0, %75:field/borrow#1, %73:field/borrow#2][0m
    %77 = extract.field %76, 0[2m : Int[0m[2m ; ops [%76:container/borrow#0][0m
    %78 = extract.field %76, 1[2m : Int[0m[2m ; ops [%76:container/borrow#0][0m
    %79 = extract.field %76, 2[2m : Ptr[0m[2m ; ops [%76:container/borrow#0][0m
    %80 = fn_ref $ylc_audio_arr_choose_kernel
    %81 = const.int 81136
    %82 = ptr_offset %1, %81[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %81:index/borrow#1][0m
    %83 = call %80(%82, %3, %77, %78, %79, %5)[2m : Double[0m[2m ; ops [%80:callee/borrow#0, %82:value/borrow#0, %3:value/borrow#1, %77:value/borrow#2, %78:value/borrow#3, %79:value/borrow#4, %5:value/borrow#5][0m
    %84 = const.double 1
    %85 = const.double 0.9
    %86 = const.double 1.2
    %87 = fn_ref $ylc_audio_lfnoise_kernel
    %88 = const.int 81160
    %89 = ptr_offset %1, %88[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %88:index/borrow#1][0m
    %90 = primitive_cast %89 from Ptr of Char to Ptr of Double[2m ; ops [%89:value/borrow#0][0m
    %91 = call %87(%90, %3, %84, %85, %86)[2m : Double[0m[2m ; ops [%87:callee/borrow#0, %90:value/borrow#0, %3:value/borrow#1, %84:value/borrow#2, %85:value/borrow#3, %86:value/borrow#4][0m
    %92 = fmul %83, %91[2m ; ops [%83:value/borrow#0, %91:value/borrow#1][0m
    %93 = const.double 0
    %94 = const.double 0
    %95 = fn_ref $ylc_audio_pitchshift_kernel
    %96 = const.int 81200
    %97 = ptr_offset %1, %96[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %96:index/borrow#1][0m
    %98 = const.int 2048
    %99 = call %95(%97, %3, %98, %70, %92, %93, %94, %10)[2m : Double[0m[2m ; ops [%95:callee/borrow#0, %97:value/borrow#0, %3:value/borrow#1, %98:value/borrow#2, %70:value/borrow#3, %92:value/borrow#4, %93:value/borrow#5, %94:value/borrow#6, %10:value/borrow#7][0m
    %100 = const.int 97752
    %101 = ptr_offset %1, %100[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %100:index/borrow#1][0m
    %102 = const.int 2048
    %103 = call %95(%101, %3, %102, %70, %92, %93, %94, %11)[2m : Double[0m[2m ; ops [%95:callee/borrow#0, %101:value/borrow#0, %3:value/borrow#1, %102:value/borrow#2, %70:value/borrow#3, %92:value/borrow#4, %93:value/borrow#5, %94:value/borrow#6, %11:value/borrow#7][0m
    %104 = construct.tuple { %99, %103 }[2m : (Double, Double, )[0m[2m ; ops [%99:field/consume#0, %103:field/consume#1][0m
    %105 = fmul %69, %99[2m ; ops [%69:value/borrow#0, %99:value/borrow#1][0m
    %106 = fmul %69, %103[2m ; ops [%69:value/borrow#0, %103:value/borrow#1][0m
    %107 = construct.tuple { %105, %106 }[2m : (Double, Double, )[0m[2m ; ops [%105:field/consume#0, %106:field/consume#1][0m
    %108 = const.double 100
    %109 = const.double 1
    %110 = const.double 1
    %111 = fn_ref $db_amp
    %112 = const.double -1
    %113 = call %111(%112)[2m : Double[0m[2m ; ops [%111:callee/borrow#0, %112:value/borrow#0][0m
    %114 = const.double 0.5
    %115 = const.double 0.01
    %116 = const.double 0.1
    %117 = fn_ref $ylc_audio_lfnoise_kernel
    %118 = const.int 114304
    %119 = ptr_offset %1, %118[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %118:index/borrow#1][0m
    %120 = primitive_cast %119 from Ptr of Char to Ptr of Double[2m ; ops [%119:value/borrow#0][0m
    %121 = call %117(%120, %3, %114, %115, %116)[2m : Double[0m[2m ; ops [%117:callee/borrow#0, %120:value/borrow#0, %3:value/borrow#1, %114:value/borrow#2, %115:value/borrow#3, %116:value/borrow#4][0m
    %122 = const.double 0.1
    %123 = const.double 0.5
    %124 = fn_ref $ylc_audio_comb_kernel
    %125 = const.int 114344
    %126 = ptr_offset %1, %125[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %125:index/borrow#1][0m
    %127 = const.int 4802
    %128 = call %124(%126, %3, %127, %121, %123, %10)[2m : Double[0m[2m ; ops [%124:callee/borrow#0, %126:value/borrow#0, %3:value/borrow#1, %127:value/borrow#2, %121:value/borrow#3, %123:value/borrow#4, %10:value/borrow#5][0m
    %129 = const.int 152768
    %130 = ptr_offset %1, %129[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %129:index/borrow#1][0m
    %131 = const.int 4802
    %132 = call %124(%130, %3, %131, %121, %123, %11)[2m : Double[0m[2m ; ops [%124:callee/borrow#0, %130:value/borrow#0, %3:value/borrow#1, %131:value/borrow#2, %121:value/borrow#3, %123:value/borrow#4, %11:value/borrow#5][0m
    %133 = construct.tuple { %128, %132 }[2m : (Double, Double, )[0m[2m ; ops [%128:field/consume#0, %132:field/consume#1][0m
    %134 = fmul %113, %128[2m ; ops [%113:value/borrow#0, %128:value/borrow#1][0m
    %135 = fmul %113, %132[2m ; ops [%113:value/borrow#0, %132:value/borrow#1][0m
    %136 = construct.tuple { %134, %135 }[2m : (Double, Double, )[0m[2m ; ops [%134:field/consume#0, %135:field/consume#1][0m
    %137 = fn_ref $ylc_audio_disperser_kernel
    %138 = const.int 191192
    %139 = ptr_offset %1, %138[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %138:index/borrow#1][0m
    %140 = primitive_cast %139 from Ptr of Char to Ptr of Double[2m ; ops [%139:value/borrow#0][0m
    %141 = call %137(%140, %3, %108, %109, %110, %134)[2m : Double[0m[2m ; ops [%137:callee/borrow#0, %140:value/borrow#0, %3:value/borrow#1, %108:value/borrow#2, %109:value/borrow#3, %110:value/borrow#4, %134:value/borrow#5][0m
    %142 = const.int 193240
    %143 = ptr_offset %1, %142[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %142:index/borrow#1][0m
    %144 = primitive_cast %143 from Ptr of Char to Ptr of Double[2m ; ops [%143:value/borrow#0][0m
    %145 = call %137(%144, %3, %108, %109, %110, %135)[2m : Double[0m[2m ; ops [%137:callee/borrow#0, %144:value/borrow#0, %3:value/borrow#1, %108:value/borrow#2, %109:value/borrow#3, %110:value/borrow#4, %135:value/borrow#5][0m
    %146 = construct.tuple { %141, %145 }[2m : (Double, Double, )[0m[2m ; ops [%141:field/consume#0, %145:field/consume#1][0m
    %147 = const.double 1000
    %148 = const.double 0.707
    %149 = fn_ref $lpf.kernel
    %150 = const.int 195288
    %151 = ptr_offset %1, %150[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %150:index/borrow#1][0m
    %152 = primitive_cast %151 from Ptr of Char to Ptr of Char[2m ; ops [%151:value/borrow#0][0m
    %153 = call %149(%0, %152, %2, %3, %147, %148, %10)[2m : Double[0m[2m ; ops [%149:callee/borrow#0, %0:value/consume#0, %152:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %147:value/borrow#4, %148:value/borrow#5, %10:value/consume#6][0m
    %154 = const.int 195320
    %155 = ptr_offset %1, %154[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %154:index/borrow#1][0m
    %156 = primitive_cast %155 from Ptr of Char to Ptr of Char[2m ; ops [%155:value/borrow#0][0m
    %157 = call %149(%0, %156, %2, %3, %147, %148, %11)[2m : Double[0m[2m ; ops [%149:callee/borrow#0, %0:value/consume#0, %156:value/borrow#1, %2:value/consume#2, %3:value/consume#3, %147:value/borrow#4, %148:value/borrow#5, %11:value/consume#6][0m
    %158 = construct.tuple { %153, %157 }[2m : (Double, Double, )[0m[2m ; ops [%153:field/consume#0, %157:field/consume#1][0m
    %159 = fn_ref $db_amp
    %160 = const.double 4
    %161 = call %159(%160)[2m : Double[0m[2m ; ops [%159:callee/borrow#0, %160:value/borrow#0][0m
    %162 = const.double 0.01
    %163 = fn_ref $ylc_audio_zero_xing_freq_kernel
    %164 = const.int 195352
    %165 = ptr_offset %1, %164[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %164:index/borrow#1][0m
    %166 = primitive_cast %165 from Ptr of Char to Ptr of Double[2m ; ops [%165:value/borrow#0][0m
    %167 = call %163(%166, %3, %153)[2m : Double[0m[2m ; ops [%163:callee/borrow#0, %166:value/borrow#0, %3:value/borrow#1, %153:value/borrow#2][0m
    %168 = const.int 195384
    %169 = ptr_offset %1, %168[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %168:index/borrow#1][0m
    %170 = primitive_cast %169 from Ptr of Char to Ptr of Double[2m ; ops [%169:value/borrow#0][0m
    %171 = call %163(%170, %3, %157)[2m : Double[0m[2m ; ops [%163:callee/borrow#0, %170:value/borrow#0, %3:value/borrow#1, %157:value/borrow#2][0m
    %172 = construct.tuple { %167, %171 }[2m : (Double, Double, )[0m[2m ; ops [%167:field/consume#0, %171:field/consume#1][0m
    %173 = fn_ref $ylc_audio_lag_kernel
    %174 = const.int 195416
    %175 = ptr_offset %1, %174[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %174:index/borrow#1][0m
    %176 = primitive_cast %175 from Ptr of Char to Ptr of Double[2m ; ops [%175:value/borrow#0][0m
    %177 = call %173(%176, %3, %162, %167)[2m : Double[0m[2m ; ops [%173:callee/borrow#0, %176:value/borrow#0, %3:value/borrow#1, %162:value/borrow#2, %167:value/borrow#3][0m
    %178 = const.int 195448
    %179 = ptr_offset %1, %178[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %178:index/borrow#1][0m
    %180 = primitive_cast %179 from Ptr of Char to Ptr of Double[2m ; ops [%179:value/borrow#0][0m
    %181 = call %173(%180, %3, %162, %171)[2m : Double[0m[2m ; ops [%173:callee/borrow#0, %180:value/borrow#0, %3:value/borrow#1, %162:value/borrow#2, %171:value/borrow#3][0m
    %182 = construct.tuple { %177, %181 }[2m : (Double, Double, )[0m[2m ; ops [%177:field/consume#0, %181:field/consume#1][0m
    %183 = fn_ref $ylc_audio_sin_osc_kernel
    %184 = const.int 195480
    %185 = ptr_offset %1, %184[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %184:index/borrow#1][0m
    %186 = primitive_cast %185 from Ptr of Char to Ptr of Double[2m ; ops [%185:value/borrow#0][0m
    %187 = call %183(%186, %3, %177)[2m : Double[0m[2m ; ops [%183:callee/borrow#0, %186:value/borrow#0, %3:value/borrow#1, %177:value/borrow#2][0m
    %188 = const.int 195488
    %189 = ptr_offset %1, %188[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %188:index/borrow#1][0m
    %190 = primitive_cast %189 from Ptr of Char to Ptr of Double[2m ; ops [%189:value/borrow#0][0m
    %191 = call %183(%190, %3, %181)[2m : Double[0m[2m ; ops [%183:callee/borrow#0, %190:value/borrow#0, %3:value/borrow#1, %181:value/borrow#2][0m
    %192 = construct.tuple { %187, %191 }[2m : (Double, Double, )[0m[2m ; ops [%187:field/consume#0, %191:field/consume#1][0m
    %193 = fmul %187, %10[2m ; ops [%187:value/borrow#0, %10:value/borrow#1][0m
    %194 = fmul %191, %11[2m ; ops [%191:value/borrow#0, %11:value/borrow#1][0m
    %195 = construct.tuple { %193, %194 }[2m : (Double, Double, )[0m[2m ; ops [%193:field/consume#0, %194:field/consume#1][0m
    %196 = fmul %161, %193[2m ; ops [%161:value/borrow#0, %193:value/borrow#1][0m
    %197 = fmul %161, %194[2m ; ops [%161:value/borrow#0, %194:value/borrow#1][0m
    %198 = construct.tuple { %196, %197 }[2m : (Double, Double, )[0m[2m ; ops [%196:field/consume#0, %197:field/consume#1][0m
    %199 = fn_ref $$builtin.op_eq
    %200 = const.double 6
    %201 = call %199 as $builtin_op_eq.Double.Double.Bool(%4, %200)[2m : Bool[0m[2m ; ops [%199:callee/borrow#0, %4:value/borrow#0, %200:value/borrow#1][0m
    %202 = primitive_cast %201 from Bool to Double[2m ; ops [%201:value/borrow#0][0m
    %203 = fn_ref $stutter.kernel
    %204 = const.int 195496
    %205 = ptr_offset %1, %204[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %204:index/borrow#1][0m
    %206 = primitive_cast %205 from Ptr of Char to Ptr of Char[2m ; ops [%205:value/borrow#0][0m
    %207 = call %203(%0, %206, %2, %3, %5, %202, %7, %6, %8, %10)[2m : Double[0m[2m ; ops [%203:callee/borrow#0, %0:value/consume#0, %206:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %5:value/consume#4, %202:value/borrow#5, %7:value/borrow#6, %6:value/borrow#7, %8:value/borrow#8, %10:value/consume#9][0m
    %208 = const.int 1731528
    %209 = ptr_offset %1, %208[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %208:index/borrow#1][0m
    %210 = primitive_cast %209 from Ptr of Char to Ptr of Char[2m ; ops [%209:value/borrow#0][0m
    %211 = call %203(%0, %210, %2, %3, %5, %202, %7, %6, %8, %11)[2m : Double[0m[2m ; ops [%203:callee/borrow#0, %0:value/consume#0, %210:value/borrow#1, %2:value/consume#2, %3:value/borrow#3, %5:value/consume#4, %202:value/borrow#5, %7:value/borrow#6, %6:value/borrow#7, %8:value/borrow#8, %11:value/consume#9][0m
    %212 = construct.tuple { %207, %211 }[2m : (Double, Double, )[0m[2m ; ops [%207:field/consume#0, %211:field/consume#1][0m
    %213 = fn_ref $db_amp
    %214 = const.double -2
    %215 = call %213(%214)[2m : Double[0m[2m ; ops [%213:callee/borrow#0, %214:value/borrow#0][0m
    %216 = const.double -12
    %217 = const.double 4
    %218 = const.double 0.005
    %219 = const.double 0.15
    %220 = const.double 6
    %221 = const.double 3
    %222 = const.double 6
    %223 = feq %4, %222[2m ; ops [%4:value/borrow#0, %222:value/borrow#1][0m
    %224 = primitive_cast %223 from Bool to Double[2m ; ops [%223:value/borrow#0][0m
    %225 = const.double 1
    %226 = fsub %225, %224[2m ; ops [%225:value/borrow#0, %224:value/borrow#1][0m
    %227 = fmul %224, %207[2m ; ops [%224:value/borrow#0, %207:value/borrow#1][0m
    %228 = fmul %226, %10[2m ; ops [%226:value/borrow#0, %10:value/borrow#1][0m
    %229 = fadd %227, %228[2m ; ops [%227:value/borrow#0, %228:value/borrow#1][0m
    %230 = fmul %224, %211[2m ; ops [%224:value/borrow#0, %211:value/borrow#1][0m
    %231 = fmul %226, %11[2m ; ops [%226:value/borrow#0, %11:value/borrow#1][0m
    %232 = fadd %230, %231[2m ; ops [%230:value/borrow#0, %231:value/borrow#1][0m
    %233 = construct.tuple { %229, %232 }[2m : (Double, Double, )[0m[2m ; ops [%229:field/consume#0, %232:field/consume#1][0m
    %234 = const.double 5
    %235 = feq %4, %234[2m ; ops [%4:value/borrow#0, %234:value/borrow#1][0m
    %236 = primitive_cast %235 from Bool to Double[2m ; ops [%235:value/borrow#0][0m
    %237 = const.double 1
    %238 = fsub %237, %236[2m ; ops [%237:value/borrow#0, %236:value/borrow#1][0m
    %239 = fmul %236, %10[2m ; ops [%236:value/borrow#0, %10:value/borrow#1][0m
    %240 = fmul %238, %229[2m ; ops [%238:value/borrow#0, %229:value/borrow#1][0m
    %241 = fadd %239, %240[2m ; ops [%239:value/borrow#0, %240:value/borrow#1][0m
    %242 = fmul %236, %11[2m ; ops [%236:value/borrow#0, %11:value/borrow#1][0m
    %243 = fmul %238, %232[2m ; ops [%238:value/borrow#0, %232:value/borrow#1][0m
    %244 = fadd %242, %243[2m ; ops [%242:value/borrow#0, %243:value/borrow#1][0m
    %245 = construct.tuple { %241, %244 }[2m : (Double, Double, )[0m[2m ; ops [%241:field/consume#0, %244:field/consume#1][0m
    %246 = const.double 4
    %247 = feq %4, %246[2m ; ops [%4:value/borrow#0, %246:value/borrow#1][0m
    %248 = primitive_cast %247 from Bool to Double[2m ; ops [%247:value/borrow#0][0m
    %249 = const.double 1
    %250 = fsub %249, %248[2m ; ops [%249:value/borrow#0, %248:value/borrow#1][0m
    %251 = fmul %248, %196[2m ; ops [%248:value/borrow#0, %196:value/borrow#1][0m
    %252 = fmul %250, %241[2m ; ops [%250:value/borrow#0, %241:value/borrow#1][0m
    %253 = fadd %251, %252[2m ; ops [%251:value/borrow#0, %252:value/borrow#1][0m
    %254 = fmul %248, %197[2m ; ops [%248:value/borrow#0, %197:value/borrow#1][0m
    %255 = fmul %250, %244[2m ; ops [%250:value/borrow#0, %244:value/borrow#1][0m
    %256 = fadd %254, %255[2m ; ops [%254:value/borrow#0, %255:value/borrow#1][0m
    %257 = construct.tuple { %253, %256 }[2m : (Double, Double, )[0m[2m ; ops [%253:field/consume#0, %256:field/consume#1][0m
    %258 = const.double 3
    %259 = feq %4, %258[2m ; ops [%4:value/borrow#0, %258:value/borrow#1][0m
    %260 = primitive_cast %259 from Bool to Double[2m ; ops [%259:value/borrow#0][0m
    %261 = const.double 1
    %262 = fsub %261, %260[2m ; ops [%261:value/borrow#0, %260:value/borrow#1][0m
    %263 = fmul %260, %141[2m ; ops [%260:value/borrow#0, %141:value/borrow#1][0m
    %264 = fmul %262, %253[2m ; ops [%262:value/borrow#0, %253:value/borrow#1][0m
    %265 = fadd %263, %264[2m ; ops [%263:value/borrow#0, %264:value/borrow#1][0m
    %266 = fmul %260, %145[2m ; ops [%260:value/borrow#0, %145:value/borrow#1][0m
    %267 = fmul %262, %256[2m ; ops [%262:value/borrow#0, %256:value/borrow#1][0m
    %268 = fadd %266, %267[2m ; ops [%266:value/borrow#0, %267:value/borrow#1][0m
    %269 = construct.tuple { %265, %268 }[2m : (Double, Double, )[0m[2m ; ops [%265:field/consume#0, %268:field/consume#1][0m
    %270 = const.double 2
    %271 = feq %4, %270[2m ; ops [%4:value/borrow#0, %270:value/borrow#1][0m
    %272 = primitive_cast %271 from Bool to Double[2m ; ops [%271:value/borrow#0][0m
    %273 = const.double 1
    %274 = fsub %273, %272[2m ; ops [%273:value/borrow#0, %272:value/borrow#1][0m
    %275 = fmul %272, %105[2m ; ops [%272:value/borrow#0, %105:value/borrow#1][0m
    %276 = fmul %274, %265[2m ; ops [%274:value/borrow#0, %265:value/borrow#1][0m
    %277 = fadd %275, %276[2m ; ops [%275:value/borrow#0, %276:value/borrow#1][0m
    %278 = fmul %272, %106[2m ; ops [%272:value/borrow#0, %106:value/borrow#1][0m
    %279 = fmul %274, %268[2m ; ops [%274:value/borrow#0, %268:value/borrow#1][0m
    %280 = fadd %278, %279[2m ; ops [%278:value/borrow#0, %279:value/borrow#1][0m
    %281 = construct.tuple { %277, %280 }[2m : (Double, Double, )[0m[2m ; ops [%277:field/consume#0, %280:field/consume#1][0m
    %282 = const.double 1
    %283 = feq %4, %282[2m ; ops [%4:value/borrow#0, %282:value/borrow#1][0m
    %284 = primitive_cast %283 from Bool to Double[2m ; ops [%283:value/borrow#0][0m
    %285 = const.double 1
    %286 = fsub %285, %284[2m ; ops [%285:value/borrow#0, %284:value/borrow#1][0m
    %287 = fmul %284, %61[2m ; ops [%284:value/borrow#0, %61:value/borrow#1][0m
    %288 = fmul %286, %277[2m ; ops [%286:value/borrow#0, %277:value/borrow#1][0m
    %289 = fadd %287, %288[2m ; ops [%287:value/borrow#0, %288:value/borrow#1][0m
    %290 = fmul %284, %65[2m ; ops [%284:value/borrow#0, %65:value/borrow#1][0m
    %291 = fmul %286, %280[2m ; ops [%286:value/borrow#0, %280:value/borrow#1][0m
    %292 = fadd %290, %291[2m ; ops [%290:value/borrow#0, %291:value/borrow#1][0m
    %293 = construct.tuple { %289, %292 }[2m : (Double, Double, )[0m[2m ; ops [%289:field/consume#0, %292:field/consume#1][0m
    %294 = const.double 0
    %295 = feq %4, %294[2m ; ops [%4:value/borrow#0, %294:value/borrow#1][0m
    %296 = primitive_cast %295 from Bool to Double[2m ; ops [%295:value/borrow#0][0m
    %297 = const.double 1
    %298 = fsub %297, %296[2m ; ops [%297:value/borrow#0, %296:value/borrow#1][0m
    %299 = fmul %296, %39[2m ; ops [%296:value/borrow#0, %39:value/borrow#1][0m
    %300 = fmul %298, %289[2m ; ops [%298:value/borrow#0, %289:value/borrow#1][0m
    %301 = fadd %299, %300[2m ; ops [%299:value/borrow#0, %300:value/borrow#1][0m
    %302 = fmul %296, %43[2m ; ops [%296:value/borrow#0, %43:value/borrow#1][0m
    %303 = fmul %298, %292[2m ; ops [%298:value/borrow#0, %292:value/borrow#1][0m
    %304 = fadd %302, %303[2m ; ops [%302:value/borrow#0, %303:value/borrow#1][0m
    %305 = construct.tuple { %301, %304 }[2m : (Double, Double, )[0m[2m ; ops [%301:field/consume#0, %304:field/consume#1][0m
    %306 = fn_ref $ylc_audio_glue_kernel
    %307 = const.int 3267560
    %308 = ptr_offset %1, %307[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %307:index/borrow#1][0m
    %309 = primitive_cast %308 from Ptr of Char to Ptr of Double[2m ; ops [%308:value/borrow#0][0m
    %310 = call %306(%309, %3, %216, %217, %218, %219, %220, %221, %301)[2m : Double[0m[2m ; ops [%306:callee/borrow#0, %309:value/borrow#0, %3:value/borrow#1, %216:value/borrow#2, %217:value/borrow#3, %218:value/borrow#4, %219:value/borrow#5, %220:value/borrow#6, %221:value/borrow#7, %301:value/borrow#8][0m
    %311 = const.int 3267584
    %312 = ptr_offset %1, %311[2m : Ptr[0m[2m ; ops [%1:value/borrow#0, %311:index/borrow#1][0m
    %313 = primitive_cast %312 from Ptr of Char to Ptr of Double[2m ; ops [%312:value/borrow#0][0m
    %314 = call %306(%313, %3, %216, %217, %218, %219, %220, %221, %304)[2m : Double[0m[2m ; ops [%306:callee/borrow#0, %313:value/borrow#0, %3:value/borrow#1, %216:value/borrow#2, %217:value/borrow#3, %218:value/borrow#4, %219:value/borrow#5, %220:value/borrow#6, %221:value/borrow#7, %304:value/borrow#8][0m
    %315 = construct.tuple { %310, %314 }[2m : (Double, Double, )[0m[2m ; ops [%310:field/consume#0, %314:field/consume#1][0m
    %316 = fmul %215, %310[2m ; ops [%215:value/borrow#0, %310:value/borrow#1][0m
    %317 = fmul %215, %314[2m ; ops [%215:value/borrow#0, %314:value/borrow#1][0m
    %318 = construct.tuple { %316, %317 }[2m : (Double, Double, )[0m[2m ; ops [%316:field/consume#0, %317:field/consume#1][0m
    return %318[2m ; ops [%318:return/consume#0][0m
}
fn fx.frame(%0 node[2m: Ptr @consume[0m, %1 state[2m: Ptr @borrow[0m, %2 inputs[2m: Ptr @borrow[0m, %3 frame[2m: Int @consume[0m, %4 spf[2m: Double @consume[0m)[2m -> () @none[0m[2m @skip_rc[0m {
  bb0 entry:
    %5 = const.int 0
    %6 = ptr_offset %2, %5[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %5:index/borrow#1][0m
    %7 = load %6[2m : Ptr[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = fn_ref $ylc_read_inlet_node_lane_i32
    %9 = const.int 0
    %10 = call %8(%7, %3, %9)[2m : Double[0m[2m ; ops [%8:callee/borrow#0, %7:value/borrow#0, %3:value/borrow#1, %9:value/borrow#2][0m
    %11 = const.int 1
    %12 = ptr_offset %2, %11[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %11:index/borrow#1][0m
    %13 = load %12[2m : Ptr[0m[2m ; ops [%12:value/borrow#0][0m
    %14 = fn_ref $ylc_read_inlet_node_lane_i32
    %15 = const.int 0
    %16 = call %14(%13, %3, %15)[2m : Double[0m[2m ; ops [%14:callee/borrow#0, %13:value/borrow#0, %3:value/borrow#1, %15:value/borrow#2][0m
    %17 = const.int 2
    %18 = ptr_offset %2, %17[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %17:index/borrow#1][0m
    %19 = load %18[2m : Ptr[0m[2m ; ops [%18:value/borrow#0][0m
    %20 = fn_ref $ylc_read_inlet_node_lane_i32
    %21 = const.int 0
    %22 = call %20(%19, %3, %21)[2m : Double[0m[2m ; ops [%20:callee/borrow#0, %19:value/borrow#0, %3:value/borrow#1, %21:value/borrow#2][0m
    %23 = const.int 3
    %24 = ptr_offset %2, %23[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %23:index/borrow#1][0m
    %25 = load %24[2m : Ptr[0m[2m ; ops [%24:value/borrow#0][0m
    %26 = fn_ref $ylc_read_inlet_node_lane_i32
    %27 = const.int 0
    %28 = call %26(%25, %3, %27)[2m : Double[0m[2m ; ops [%26:callee/borrow#0, %25:value/borrow#0, %3:value/borrow#1, %27:value/borrow#2][0m
    %29 = const.int 4
    %30 = ptr_offset %2, %29[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %29:index/borrow#1][0m
    %31 = load %30[2m : Ptr[0m[2m ; ops [%30:value/borrow#0][0m
    %32 = fn_ref $ylc_read_inlet_node_lane_i32
    %33 = const.int 0
    %34 = call %32(%31, %3, %33)[2m : Double[0m[2m ; ops [%32:callee/borrow#0, %31:value/borrow#0, %3:value/borrow#1, %33:value/borrow#2][0m
    %35 = const.int 5
    %36 = ptr_offset %2, %35[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %35:index/borrow#1][0m
    %37 = load %36[2m : Ptr[0m[2m ; ops [%36:value/borrow#0][0m
    %38 = fn_ref $ylc_read_inlet_node_lane_i32
    %39 = const.int 0
    %40 = call %38(%37, %3, %39)[2m : Double[0m[2m ; ops [%38:callee/borrow#0, %37:value/borrow#0, %3:value/borrow#1, %39:value/borrow#2][0m
    %41 = const.int 5
    %42 = ptr_offset %2, %41[2m : Ptr[0m[2m ; ops [%2:value/borrow#0, %41:index/borrow#1][0m
    %43 = load %42[2m : Ptr[0m[2m ; ops [%42:value/borrow#0][0m
    %44 = fn_ref $ylc_read_inlet_node_lane_i32
    %45 = const.int 1
    %46 = call %44(%43, %3, %45)[2m : Double[0m[2m ; ops [%44:callee/borrow#0, %43:value/borrow#0, %3:value/borrow#1, %45:value/borrow#2][0m
    %47 = construct.tuple { %40, %46 }[2m : (Double, Double, )[0m[2m ; ops [%40:field/consume#0, %46:field/consume#1][0m
    %48 = fn_ref $fx.kernel
    %49 = call %48(%0, %1, %3, %4, %10, %16, %22, %28, %34, %47)[2m : (Double, Double, )[0m[2m ; ops [%48:callee/borrow#0, %0:value/consume#0, %1:value/borrow#1, %3:value/consume#2, %4:value/consume#3, %10:value/borrow#4, %16:value/consume#5, %22:value/borrow#6, %28:value/borrow#7, %34:value/borrow#8, %47:value/borrow#9][0m
    %50 = fn_ref $ylc_get_output_buf
    %51 = call %50(%0)[2m : Ptr[0m[2m ; ops [%50:callee/borrow#0, %0:value/borrow#0][0m
    %52 = const.int 2
    %53 = imul %3, %52[2m ; ops [%3:value/borrow#0, %52:value/borrow#1][0m
    %54 = extract.field %49, 0[2m : Double[0m[2m ; ops [%49:container/borrow#0][0m
    %55 = ptr_offset %51, %53[2m : Ptr[0m[2m ; ops [%51:value/borrow#0, %53:index/borrow#1][0m
    %56 = store %55, %54[2m : ()[0m[2m ; ops [%55:value/borrow#0, %54:element/consume#1][0m
    %57 = extract.field %49, 1[2m : Double[0m[2m ; ops [%49:container/borrow#0][0m
    %58 = const.int 1
    %59 = iadd %53, %58[2m ; ops [%53:value/borrow#0, %58:value/borrow#1][0m
    %60 = ptr_offset %51, %59[2m : Ptr[0m[2m ; ops [%51:value/borrow#0, %59:index/borrow#1][0m
    %61 = store %60, %57[2m : ()[0m[2m ; ops [%60:value/borrow#0, %57:element/consume#1][0m
    %62 = const.void
    return %62[2m ; ops [%62:return/consume#0][0m
}
extern fn ylc_audio_disperser_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_rand_trig_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_trig_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_sah_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_arr_choose_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Int @borrow[0m, %4 arg4[2m: Ptr @borrow[0m, %5 arg5[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_pitchshift_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Int @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m, %7 arg7[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_zero_xing_freq_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_lag_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_sin_osc_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m)[2m -> Double @owned[0m;
extern fn ylc_audio_glue_kernel(%0 arg0[2m: Ptr @borrow[0m, %1 arg1[2m: Double @borrow[0m, %2 arg2[2m: Double @borrow[0m, %3 arg3[2m: Double @borrow[0m, %4 arg4[2m: Double @borrow[0m, %5 arg5[2m: Double @borrow[0m, %6 arg6[2m: Double @borrow[0m, %7 arg7[2m: Double @borrow[0m, %8 arg8[2m: Double @borrow[0m)[2m -> Double @owned[0m;
fn set_fx_rand(%0 _[2m: () @borrow[0m)[2m -> Ptr @owned[0m {
  bb0 entry:
    %1 = fn_ref $rand_int
    %2 = const.int 9
    %3 = call %1(%2)[2m : Int[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0][0m
    %4 = primitive_cast %3 from Int to Double[2m ; ops [%3:value/borrow#0][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %33 = const.void
    %34 = fn_ref $set_input_scalar
    %35 = global_load @$global.fx_node[2m : Ptr[0m
    %36 = const.int 0
    %37 = call %34(%35, %36, %4)[2m : Ptr[0m[2m ; ops [%34:callee/borrow#0, %35:value/borrow#0, %36:value/borrow#1, %4:value/borrow#2][0m
    %38 = fn_ref $set_input_trig
    %39 = global_load @$global.fx_node[2m : Ptr[0m
    %40 = const.int 1
    %41 = call %38(%39, %40)[2m : Ptr[0m[2m ; ops [%38:callee/borrow#0, %39:value/borrow#0, %40:value/borrow#1][0m
    return %41[2m ; ops [%41:return/consume#0][0m
  bb3 match.arm.0.test:
    %5 = const.double 6
    %6 = feq %4, %5[2m ; ops [%4:value/borrow#0, %5:value/borrow#1][0m
    cond %6, bb4, bb5[2m ; ops [%6:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %7 = fn_ref $set_input_scalar
    %8 = global_load @$global.fx_node[2m : Ptr[0m
    %9 = const.int 2
    %10 = fn_ref $Math.array_choose
    %11 = const.double 0.25
    %12 = const.double 0.25
    %13 = const.double 0.25
    %14 = const.double 0.125
    %15 = const.double 0.75
    %16 = const.double 0.5
    %17 = const.double 1.5
    %18 = construct.array_literal { %11, %12, %13, %14, %15, %16, %17 }[2m : Array of Double[0m[2m ; ea stack#0[0m[2m ; ops [%11:element/consume#0, %12:element/consume#1, %13:element/consume#2, %14:element/consume#3, %15:element/consume#4, %16:element/consume#5, %17:element/consume#6][0m
    %19 = call %10 as $Math_array_choose.Array_Double.Double(%18)[2m : Double[0m[2m ; ops [%10:callee/borrow#0, %18:value/borrow#0][0m
    %20 = call %7(%8, %9, %19)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %8:value/borrow#0, %9:value/borrow#1, %19:value/borrow#2][0m
    %21 = fn_ref $set_input_scalar
    %22 = global_load @$global.fx_node[2m : Ptr[0m
    %23 = const.int 4
    %24 = fn_ref $Math.array_choose
    %25 = const.double -1
    %26 = const.double 1
    %27 = const.double 2
    %28 = construct.array_literal { %25, %26, %27 }[2m : Array of Double[0m[2m ; ea stack#1[0m[2m ; ops [%25:element/consume#0, %26:element/consume#1, %27:element/consume#2][0m
    %29 = call %24 as $Math_array_choose.Array_Double.Double(%28)[2m : Double[0m[2m ; ops [%24:callee/borrow#0, %28:value/borrow#0][0m
    %30 = call %21(%22, %23, %29)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %22:value/borrow#0, %23:value/borrow#1, %29:value/borrow#2][0m
    %31 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %32 = const.void
    br bb2
}
fn Math_array_choose.Array_Double.Double(%0 arr[2m: Array of Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %2 = fn_ref $rand_int
    %3 = call %2(%1)[2m : Int[0m[2m ; ops [%2:callee/borrow#0, %1:value/borrow#0][0m
    %4 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %5 = ptr_offset %4, %3[2m : Ptr[0m[2m ; ops [%4:value/borrow#0, %3:index/borrow#1][0m
    %6 = load %5[2m : Double[0m[2m ; ops [%5:value/borrow#0][0m
[35m    %7 = dup %6[2m : ()[0m[2m ; ops [%6:value/borrow#0][0m[0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn kick_routine(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $kick_routine.<anonymous>
    %2 = global_load @$global.kick_durs[2m : Array of Double[0m
[35m    %7 = dup %2[2m : ()[0m[2m ; ops [%2:value/borrow#0][0m[0m
    %3 = fn_ref $$builtin.iter.array.417
    %4 = coro.new %3(%2)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#0[0m[2m ; ops [%3:callee/borrow#0, %2:value/consume#0][0m
    %5 = fn_ref $$builtin.cor_map.418
    %6 = coro.new %5(%1, %4)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#1[0m[2m ; ops [%5:callee/borrow#0, %1:value/consume#0, %4:value/consume#1][0m
    return %6[2m ; ops [%6:return/consume#0][0m
}
fn kick_routine.<anonymous>(%0 d[2m: Double @consume[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = fn_ref $play_node
    %2 = fn_ref $kd.cons
    %3 = call %2()[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = call %1(%3)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %3:value/borrow#0][0m
    return %0[2m ; ops [%0:return/consume#0][0m
}
fn $builtin.iter.array.417(%0 array[2m: Array of Double @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.int 0
    br bb1
  bb1 iter.array.check:
    %2 = phi [bb0: %1, bb3: %9][2m : Int[0m[2m ; ops [%1:value/borrow#0, %9:value/borrow#1][0m
    %3 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %4 = ilt %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 iter.array.value:
    %5 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %6 = ptr_offset %5, %2[2m : Ptr[0m[2m ; ops [%5:value/borrow#0, %2:index/borrow#1][0m
    %7 = load %6[2m : Double[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = const.int 1
    %9 = iadd %2, %8[2m ; ops [%2:value/borrow#0, %8:value/borrow#1][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 iter.array.resume:
    br bb1
  bb4 iter.array.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %10 = drop %0[2m : ()[0m[2m ; ops [%0:value/borrow#0][0m[0m
    br bb4
}
fn $builtin.cor_map.418(%0 map[2m: (Double -> Double) @consume[0m, %1 source[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    br bb1
  bb1 cor_map.check:
    %2 = coro.next %1()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 cor_map.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = call %0(%6)[2m : Double[0m[2m ; ops [%0:callee/borrow#0, %6:value/consume#0][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 cor_map.resume:
    br bb1
  bb4 cor_map.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %8 = drop %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m[0m
    br bb4
}
fn snr_routine(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.double 1
    yield %1, bb1[2m ; ops [%1:value/borrow#0][0m
  bb1 yield.resume:
    %2 = fn_ref $DSP.~>
    %3 = fn_ref $snr.cons
    %4 = call %3()[2m : Ptr[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = global_load @$global.fx_node[2m : Ptr[0m
    %6 = call %2(%4, %5)[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0, %4:value/borrow#0, %5:value/borrow#1][0m
    %7 = const.double 2
    yield %7, bb2[2m ; ops [%7:value/borrow#0][0m
  bb2 yield.resume:
    %8 = fn_ref $DSP.~>
    %9 = fn_ref $snr.cons
    %10 = call %9()[2m : Ptr[0m[2m ; ops [%9:callee/borrow#0][0m
    %11 = global_load @$global.fx_node[2m : Ptr[0m
    %12 = call %8(%10, %11)[2m : Ptr[0m[2m ; ops [%8:callee/borrow#0, %10:value/borrow#0, %11:value/borrow#1][0m
    %13 = const.double 1
    yield %13, bb3[2m ; ops [%13:value/borrow#0][0m
  bb3 yield.resume:
    %14 = const.double 1
    yield %14, bb4[2m ; ops [%14:value/borrow#0][0m
  bb4 yield.resume:
    %15 = fn_ref $DSP.~>
    %16 = fn_ref $snr.cons
    %17 = call %16()[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0][0m
    %18 = global_load @$global.fx_node[2m : Ptr[0m
    %19 = call %15(%17, %18)[2m : Ptr[0m[2m ; ops [%15:callee/borrow#0, %17:value/borrow#0, %18:value/borrow#1][0m
    %20 = const.double 2
    yield %20, bb5[2m ; ops [%20:value/borrow#0][0m
  bb5 yield.resume:
    %21 = fn_ref $DSP.~>
    %22 = fn_ref $snr.cons
    %23 = call %22()[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0][0m
    %24 = global_load @$global.fx_node[2m : Ptr[0m
    %25 = call %21(%23, %24)[2m : Ptr[0m[2m ; ops [%21:callee/borrow#0, %23:value/borrow#0, %24:value/borrow#1][0m
    %26 = const.double 1
    yield %26, bb6[2m ; ops [%26:value/borrow#0][0m
  bb6 yield.resume:
    %27 = const.double 1
    yield %27, bb7[2m ; ops [%27:value/borrow#0][0m
  bb7 yield.resume:
    %28 = fn_ref $DSP.~>
    %29 = fn_ref $snr.cons
    %30 = call %29()[2m : Ptr[0m[2m ; ops [%29:callee/borrow#0][0m
    %31 = global_load @$global.fx_node[2m : Ptr[0m
    %32 = call %28(%30, %31)[2m : Ptr[0m[2m ; ops [%28:callee/borrow#0, %30:value/borrow#0, %31:value/borrow#1][0m
    %33 = const.double 2
    yield %33, bb8[2m ; ops [%33:value/borrow#0][0m
  bb8 yield.resume:
    %34 = fn_ref $DSP.~>
    %35 = fn_ref $snr.cons
    %36 = call %35()[2m : Ptr[0m[2m ; ops [%35:callee/borrow#0][0m
    %37 = global_load @$global.fx_node[2m : Ptr[0m
    %38 = call %34(%36, %37)[2m : Ptr[0m[2m ; ops [%34:callee/borrow#0, %36:value/borrow#0, %37:value/borrow#1][0m
    %39 = const.double 1
    yield %39, bb9[2m ; ops [%39:value/borrow#0][0m
  bb9 yield.resume:
    %40 = const.double 1
    yield %40, bb10[2m ; ops [%40:value/borrow#0][0m
  bb10 yield.resume:
    %41 = fn_ref $DSP.~>
    %42 = fn_ref $snr.cons
    %43 = call %42()[2m : Ptr[0m[2m ; ops [%42:callee/borrow#0][0m
    %44 = global_load @$global.fx_node[2m : Ptr[0m
    %45 = call %41(%43, %44)[2m : Ptr[0m[2m ; ops [%41:callee/borrow#0, %43:value/borrow#0, %44:value/borrow#1][0m
    %46 = const.double 2
    yield %46, bb11[2m ; ops [%46:value/borrow#0][0m
  bb11 yield.resume:
    %47 = fn_ref $DSP.~>
    %48 = fn_ref $snr.cons
    %49 = call %48()[2m : Ptr[0m[2m ; ops [%48:callee/borrow#0][0m
    %50 = global_load @$global.fx_node[2m : Ptr[0m
    %51 = call %47(%49, %50)[2m : Ptr[0m[2m ; ops [%47:callee/borrow#0, %49:value/borrow#0, %50:value/borrow#1][0m
    %52 = const.double 1
    yield %52, bb12[2m ; ops [%52:value/borrow#0][0m
  bb12 yield.resume:
    coro.done
}
fn hh_routine(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $hh_routine.<anonymous>
    %2 = const.double 0.25
    %3 = const.int 64
    %4 = construct.array_fill_const %3, %2[2m : Array of Double[0m[2m ; ea heap#0[0m[2m ; ops [%3:value/borrow#0, %2:element/consume#1][0m
    %5 = fn_ref $$builtin.iter.array.427
    %6 = coro.new %5(%4)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#1[0m[2m ; ops [%5:callee/borrow#0, %4:value/consume#0][0m
    %7 = fn_ref $$builtin.cor_map.428
    %8 = coro.new %7(%1, %6)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#2[0m[2m ; ops [%7:callee/borrow#0, %1:value/consume#0, %6:value/consume#1][0m
    return %8[2m ; ops [%8:return/consume#0][0m
}
fn hh_routine.<anonymous>(%0 d[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = const.string "hihats!!!"
    %2 = print %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m
    %3 = str %0[2m : String[0m[2m ; ea stack#0[0m[2m ; ops [%0:value/borrow#0][0m
    %4 = print %3[2m : ()[0m[2m ; ops [%3:value/borrow#0][0m
    %5 = const.string "\n"
    %6 = print %5[2m : ()[0m[2m ; ops [%5:value/borrow#0][0m
    %7 = flush [2m : ()[0m
    %8 = fn_ref $Math.chance
    %9 = const.double 0.6
    %10 = fn_ref $hh_routine.<anonymous>.<anonymous>
    %11 = call %8 as $Math_chance.Double.Fn_Void_Void.Void(%9, %10)[2m : ()[0m[2m ; ops [%8:callee/borrow#0, %9:value/borrow#0, %10:value/borrow#1][0m
    %12 = global_load @$global.beat[2m : Double[0m
    %13 = fmul %0, %12[2m ; ops [%0:value/borrow#0, %12:value/borrow#1][0m
    %14 = const.double 0.5
    %15 = fmul %13, %14[2m ; ops [%13:value/borrow#0, %14:value/borrow#1][0m
    return %15[2m ; ops [%15:return/consume#0][0m
}
fn hh_routine.<anonymous>.<anonymous>(%0 _[2m: () @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %1 = fn_ref $Math.chance2
    %2 = const.double 0.5
    %3 = fn_ref $hh_routine.<anonymous>.<anonymous>.<anonymous>
    %4 = fn_ref $hh_routine.<anonymous>.<anonymous>.<anonymous>.424
    %5 = call %1 as $Math_chance2.Double.Fn_Void_Ptr.Fn_Void_Ptr.Void(%2, %3, %4)[2m : ()[0m[2m ; ops [%1:callee/borrow#0, %2:value/borrow#0, %3:value/borrow#1, %4:value/borrow#2][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn hh_routine.<anonymous>.<anonymous>.<anonymous>(%0 _[2m: () @borrow[0m)[2m -> Ptr @owned[0m {
  bb0 entry:
    %1 = fn_ref $DSP.~>
    %2 = fn_ref $hh.cons
    %3 = call %2()[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = global_load @$global.fx_node[2m : Ptr[0m
    %5 = call %1(%3, %4)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %3:value/borrow#0, %4:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn hh_routine.<anonymous>.<anonymous>.<anonymous>.424(%0 _[2m: () @borrow[0m)[2m -> Ptr @owned[0m {
  bb0 entry:
    %1 = fn_ref $DSP.~>
    %2 = fn_ref $hh2.cons
    %3 = call %2()[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = global_load @$global.fx_node[2m : Ptr[0m
    %5 = call %1(%3, %4)[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0, %3:value/borrow#0, %4:value/borrow#1][0m
    return %5[2m ; ops [%5:return/consume#0][0m
}
fn Math_chance2.Double.Fn_Void_Ptr.Fn_Void_Ptr.Void(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> Ptr) @borrow[0m, %2 g[2m: (() -> Ptr) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %3 = fn_ref $rand_double
    %4 = call %3()[2m : Double[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = flte %4, %0[2m ; ops [%4:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %6 = const.void
    return %6[2m ; ops [%6:return/consume#0][0m
  bb3 match.arm.0.test:
    %7 = const.bool true
    %8 = beq %5, %7[2m ; ops [%5:value/borrow#0, %7:value/borrow#1][0m
    cond %8, bb4, bb5[2m ; ops [%8:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %9 = call %1()[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0][0m
    %10 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %11 = call %2()[2m : Ptr[0m[2m ; ops [%2:callee/borrow#0][0m
    %12 = const.void
    br bb2
}
fn Math_chance.Double.Fn_Void_Void.Void(%0 thresh[2m: Double @borrow[0m, %1 f[2m: (() -> ()) @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    %2 = fn_ref $rand_double
    %3 = call %2()[2m : Double[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = flte %3, %0[2m ; ops [%3:value/borrow#0, %0:value/borrow#1][0m
    br bb3
  bb1 match.no_match:
    unreachable
  bb2 match.cont:
    %5 = const.void
    return %5[2m ; ops [%5:return/consume#0][0m
  bb3 match.arm.0.test:
    %6 = const.bool true
    %7 = beq %4, %6[2m ; ops [%4:value/borrow#0, %6:value/borrow#1][0m
    cond %7, bb4, bb5[2m ; ops [%7:condition/borrow#0][0m
  bb4 match.arm.0.body:
    %8 = call %1()[2m : ()[0m[2m ; ops [%1:callee/borrow#0][0m
    %9 = const.void
    br bb2
  bb5 match.arm.1.test:
    br bb6
  bb6 match.arm.1.body:
    %10 = const.void
    br bb2
}
fn $builtin.iter.array.427(%0 array[2m: Array of Double @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.int 0
    br bb1
  bb1 iter.array.check:
    %2 = phi [bb0: %1, bb3: %9][2m : Int[0m[2m ; ops [%1:value/borrow#0, %9:value/borrow#1][0m
    %3 = extract.field %0, 0[2m : Int[0m[2m ; ops [%0:container/borrow#0][0m
    %4 = ilt %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 iter.array.value:
    %5 = extract.field %0, 2[2m : Ptr[0m[2m ; ops [%0:container/borrow#0][0m
    %6 = ptr_offset %5, %2[2m : Ptr[0m[2m ; ops [%5:value/borrow#0, %2:index/borrow#1][0m
    %7 = load %6[2m : Double[0m[2m ; ops [%6:value/borrow#0][0m
    %8 = const.int 1
    %9 = iadd %2, %8[2m ; ops [%2:value/borrow#0, %8:value/borrow#1][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 iter.array.resume:
    br bb1
  bb4 iter.array.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %10 = drop %0[2m : ()[0m[2m ; ops [%0:value/borrow#0][0m[0m
    br bb4
}
fn $builtin.cor_map.428(%0 map[2m: (Double -> Double) @consume[0m, %1 source[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    br bb1
  bb1 cor_map.check:
    %2 = coro.next %1()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 cor_map.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = call %0(%6)[2m : Double[0m[2m ; ops [%0:callee/borrow#0, %6:value/consume#0][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 cor_map.resume:
    br bb1
  bb4 cor_map.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %8 = drop %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m[0m
    br bb4
}
fn drums(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.double 0
    %2 = fn_ref $drums.<anonymous>
    %3 = fn_ref $kick_routine
    %4 = call %3()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ops [%3:callee/borrow#0][0m
    %5 = fn_ref $$builtin.cor_map.431
    %6 = coro.new %5(%2, %4)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#0[0m[2m ; ops [%5:callee/borrow#0, %2:value/consume#0, %4:value/consume#1][0m
    %7 = fn_ref $ylc_play_pattern_start
    %8 = fn_ref $__ylc_play_pattern_step_0
    %9 = call %7(%1, %8, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %8:value/borrow#1, %6:value/consume#2][0m
    %10 = const.double 0
    %11 = fn_ref $drums.<anonymous>.436
    %12 = fn_ref $snr_routine
    %13 = coro.new %12()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#1[0m[2m ; ops [%12:callee/borrow#0][0m
    %14 = fn_ref $$builtin.cor_map.437
    %15 = coro.new %14(%11, %13)[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#2[0m[2m ; ops [%14:callee/borrow#0, %11:value/consume#0, %13:value/consume#1][0m
    %16 = fn_ref $ylc_play_pattern_start
    %17 = fn_ref $__ylc_play_pattern_step_1
    %18 = call %16(%10, %17, %15)[2m : Ptr[0m[2m ; ops [%16:callee/borrow#0, %10:value/borrow#0, %17:value/borrow#1, %15:value/consume#2][0m
    %19 = const.double 0
    %20 = fn_ref $hh_routine
    %21 = call %20()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ops [%20:callee/borrow#0][0m
    %22 = fn_ref $ylc_play_pattern_start
    %23 = fn_ref $__ylc_play_pattern_step_2
    %24 = call %22(%19, %23, %21)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %19:value/borrow#0, %23:value/borrow#1, %21:value/consume#2][0m
    %25 = const.double 16
    %26 = global_load @$global.beat[2m : Double[0m
    %27 = fmul %25, %26[2m ; ops [%25:value/borrow#0, %26:value/borrow#1][0m
    %28 = const.double 0.5
    %29 = fmul %27, %28[2m ; ops [%27:value/borrow#0, %28:value/borrow#1][0m
    yield %29, bb1[2m ; ops [%29:value/borrow#0][0m
  bb1 yield.resume:
    coro.restart bb0
}
fn drums.<anonymous>(%0 d[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = global_load @$global.beat[2m : Double[0m
    %2 = fmul %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    %3 = const.double 0.5
    %4 = fmul %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    return %4[2m ; ops [%4:return/consume#0][0m
}
fn $builtin.cor_map.431(%0 map[2m: (Double -> Double) @consume[0m, %1 source[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    br bb1
  bb1 cor_map.check:
    %2 = coro.next %1()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 cor_map.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = call %0(%6)[2m : Double[0m[2m ; ops [%0:callee/borrow#0, %6:value/consume#0][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 cor_map.resume:
    br bb1
  bb4 cor_map.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %8 = drop %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m[0m
    br bb4
}
fn __ylc_play_pattern_step_0(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
extern fn ylc_schedule_current_task_event(%0 arg0[2m: Uint64 @borrow[0m, %1 arg1[2m: Double @borrow[0m)[2m -> Ptr @owned[0m;
extern fn ylc_complete_current_task()[2m -> () @none[0m;
extern fn ylc_play_pattern_start(%0 arg0[2m: Double @borrow[0m, %1 arg1[2m: Ptr @borrow[0m, %2 arg2[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @consume[0m)[2m -> Ptr @owned[0m;
fn drums.<anonymous>.436(%0 d[2m: Double @borrow[0m)[2m -> Double @owned[0m {
  bb0 entry:
    %1 = global_load @$global.beat[2m : Double[0m
    %2 = fmul %0, %1[2m ; ops [%0:value/borrow#0, %1:value/borrow#1][0m
    %3 = const.double 0.5
    %4 = fmul %2, %3[2m ; ops [%2:value/borrow#0, %3:value/borrow#1][0m
    return %4[2m ; ops [%4:return/consume#0][0m
}
fn $builtin.cor_map.437(%0 map[2m: (Double -> Double) @consume[0m, %1 source[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @consume[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    br bb1
  bb1 cor_map.check:
    %2 = coro.next %1()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb5[2m ; ops [%4:condition/borrow#0][0m
  bb2 cor_map.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = call %0(%6)[2m : Double[0m[2m ; ops [%0:callee/borrow#0, %6:value/consume#0][0m
    yield %7, bb3[2m ; ops [%7:value/borrow#0][0m
  bb3 cor_map.resume:
    br bb1
  bb4 cor_map.done:
    coro.done
  bb5 perceus.edge.drop:
[35m    %8 = drop %1[2m : ()[0m[2m ; ops [%1:value/borrow#0][0m[0m
    br bb4
}
fn __ylc_play_pattern_step_1(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_2(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn fx_set_routine(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = fn_ref $set_fx_rand
    %2 = call %1()[2m : Ptr[0m[2m ; ops [%1:callee/borrow#0][0m
    %3 = global_load @$global.beat[2m : Double[0m
    %4 = const.double 0.5
    %5 = fmul %3, %4[2m ; ops [%3:value/borrow#0, %4:value/borrow#1][0m
    %6 = fn_ref $Math.array_choose
    %7 = const.double 0.5
    %8 = const.double 1.5
    %9 = const.double 0.25
    %10 = const.double 1
    %11 = construct.array_literal { %7, %8, %9, %10 }[2m : Array of Double[0m[2m ; ea stack#0[0m[2m ; ops [%7:element/consume#0, %8:element/consume#1, %9:element/consume#2, %10:element/consume#3][0m
    %12 = call %6 as $Math_array_choose.Array_Double.Double(%11)[2m : Double[0m[2m ; ops [%6:callee/borrow#0, %11:value/borrow#0][0m
    %13 = fmul %5, %12[2m ; ops [%5:value/borrow#0, %12:value/borrow#1][0m
    yield %13, bb1[2m ; ops [%13:value/borrow#0][0m
  bb1 yield.resume:
    coro.restart bb0
}
fn g(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.double 0
    %2 = fn_ref $c
    %3 = coro.new %2()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#0[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = fn_ref $ylc_play_pattern_start
    %5 = fn_ref $__ylc_play_pattern_step_3
    %6 = call %4(%1, %5, %3)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %1:value/borrow#0, %5:value/borrow#1, %3:value/consume#2][0m
    %7 = const.double 8
    %8 = global_load @$global.beat[2m : Double[0m
    %9 = fmul %7, %8[2m ; ops [%7:value/borrow#0, %8:value/borrow#1][0m
    yield %9, bb1[2m ; ops [%9:value/borrow#0][0m
  bb1 yield.resume:
    %10 = const.double 0
    %11 = fn_ref $c
    %12 = coro.new %11()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#1[0m[2m ; ops [%11:callee/borrow#0][0m
    %13 = fn_ref $ylc_play_pattern_start
    %14 = fn_ref $__ylc_play_pattern_step_4
    %15 = call %13(%10, %14, %12)[2m : Ptr[0m[2m ; ops [%13:callee/borrow#0, %10:value/borrow#0, %14:value/borrow#1, %12:value/consume#2][0m
    %16 = const.double 8
    %17 = global_load @$global.beat[2m : Double[0m
    %18 = fmul %16, %17[2m ; ops [%16:value/borrow#0, %17:value/borrow#1][0m
    yield %18, bb2[2m ; ops [%18:value/borrow#0][0m
  bb2 yield.resume:
    %19 = const.double 0
    %20 = fn_ref $drums
    %21 = coro.new %20()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#2[0m[2m ; ops [%20:callee/borrow#0][0m
    %22 = fn_ref $ylc_play_pattern_start
    %23 = fn_ref $__ylc_play_pattern_step_5
    %24 = call %22(%19, %23, %21)[2m : Ptr[0m[2m ; ops [%22:callee/borrow#0, %19:value/borrow#0, %23:value/borrow#1, %21:value/consume#2][0m
    %25 = const.double 0
    %26 = fn_ref $c
    %27 = coro.new %26()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#3[0m[2m ; ops [%26:callee/borrow#0][0m
    %28 = fn_ref $ylc_play_pattern_start
    %29 = fn_ref $__ylc_play_pattern_step_6
    %30 = call %28(%25, %29, %27)[2m : Ptr[0m[2m ; ops [%28:callee/borrow#0, %25:value/borrow#0, %29:value/borrow#1, %27:value/consume#2][0m
    %31 = const.double 8
    %32 = global_load @$global.beat[2m : Double[0m
    %33 = fmul %31, %32[2m ; ops [%31:value/borrow#0, %32:value/borrow#1][0m
    yield %33, bb3[2m ; ops [%33:value/borrow#0][0m
  bb3 yield.resume:
    %34 = const.double 0
    %35 = fn_ref $c
    %36 = coro.new %35()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#4[0m[2m ; ops [%35:callee/borrow#0][0m
    %37 = fn_ref $ylc_play_pattern_start
    %38 = fn_ref $__ylc_play_pattern_step_7
    %39 = call %37(%34, %38, %36)[2m : Ptr[0m[2m ; ops [%37:callee/borrow#0, %34:value/borrow#0, %38:value/borrow#1, %36:value/consume#2][0m
    %40 = const.double 8
    %41 = global_load @$global.beat[2m : Double[0m
    %42 = fmul %40, %41[2m ; ops [%40:value/borrow#0, %41:value/borrow#1][0m
    yield %42, bb4[2m ; ops [%42:value/borrow#0][0m
  bb4 yield.resume:
    %43 = fn_ref $g.g2
    %44 = const.double 0
    %45 = fn_ref $fx_set_routine
    %46 = coro.new %45()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#5[0m[2m ; ops [%45:callee/borrow#0][0m
    %47 = fn_ref $ylc_play_pattern_start
    %48 = fn_ref $__ylc_play_pattern_step_18
    %49 = call %47(%44, %48, %46)[2m : Ptr[0m[2m ; ops [%47:callee/borrow#0, %44:value/borrow#0, %48:value/borrow#1, %46:value/consume#2][0m
    %50 = coro.new %43()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea stack#6[0m[2m ; ops [%43:callee/borrow#0][0m
    br bb5
  bb5 yield_from.check:
    %51 = coro.next %50()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%50:callee/borrow#0][0m
    %52 = extract.variant_tag %51[2m : Char[0m[2m ; ops [%51:value/borrow#0][0m
    %53 = tag_eq %52, Some#0[2m ; ops [%52:tag/borrow#0][0m
    cond %53, bb6, bb8[2m ; ops [%53:condition/borrow#0][0m
  bb6 yield_from.value:
    %54 = extract.variant_payload %51, Some#0[2m : Some of Double[0m[2m ; ops [%51:container/borrow#0][0m
    %55 = extract.field %54, 0[2m : Double[0m[2m ; ops [%54:container/borrow#0][0m
    yield %55, bb7[2m ; ops [%55:value/borrow#0][0m
  bb7 yield_from.resume:
    br bb5
  bb8 yield_from.exit:
    %56 = const.void
    coro.done
}
fn __ylc_play_pattern_step_3(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_4(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_5(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_6(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_7(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn g.g2(%0 _[2m: () @borrow[0m)[2m -> Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @owned[0m {
  bb0 entry:
    %1 = const.double 0
    %2 = fn_ref $m
    %3 = coro.new %2()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#0[0m[2m ; ops [%2:callee/borrow#0][0m
    %4 = fn_ref $ylc_play_pattern_start
    %5 = fn_ref $__ylc_play_pattern_step_8
    %6 = call %4(%1, %5, %3)[2m : Ptr[0m[2m ; ops [%4:callee/borrow#0, %1:value/borrow#0, %5:value/borrow#1, %3:value/consume#2][0m
    %7 = const.double 0
    %8 = fn_ref $c
    %9 = coro.new %8()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#1[0m[2m ; ops [%8:callee/borrow#0][0m
    %10 = fn_ref $ylc_play_pattern_start
    %11 = fn_ref $__ylc_play_pattern_step_9
    %12 = call %10(%7, %11, %9)[2m : Ptr[0m[2m ; ops [%10:callee/borrow#0, %7:value/borrow#0, %11:value/borrow#1, %9:value/consume#2][0m
    %13 = const.double 8
    %14 = global_load @$global.beat[2m : Double[0m
    %15 = fmul %13, %14[2m ; ops [%13:value/borrow#0, %14:value/borrow#1][0m
    yield %15, bb1[2m ; ops [%15:value/borrow#0][0m
  bb1 yield.resume:
    %16 = const.double 0
    %17 = fn_ref $m
    %18 = coro.new %17()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#2[0m[2m ; ops [%17:callee/borrow#0][0m
    %19 = fn_ref $ylc_play_pattern_start
    %20 = fn_ref $__ylc_play_pattern_step_10
    %21 = call %19(%16, %20, %18)[2m : Ptr[0m[2m ; ops [%19:callee/borrow#0, %16:value/borrow#0, %20:value/borrow#1, %18:value/consume#2][0m
    %22 = const.double 0
    %23 = fn_ref $c
    %24 = coro.new %23()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#3[0m[2m ; ops [%23:callee/borrow#0][0m
    %25 = fn_ref $ylc_play_pattern_start
    %26 = fn_ref $__ylc_play_pattern_step_11
    %27 = call %25(%22, %26, %24)[2m : Ptr[0m[2m ; ops [%25:callee/borrow#0, %22:value/borrow#0, %26:value/borrow#1, %24:value/consume#2][0m
    %28 = const.double 8
    %29 = global_load @$global.beat[2m : Double[0m
    %30 = fmul %28, %29[2m ; ops [%28:value/borrow#0, %29:value/borrow#1][0m
    yield %30, bb2[2m ; ops [%30:value/borrow#0][0m
  bb2 yield.resume:
    %31 = const.double 0
    %32 = fn_ref $m2
    %33 = coro.new %32()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#4[0m[2m ; ops [%32:callee/borrow#0][0m
    %34 = fn_ref $ylc_play_pattern_start
    %35 = fn_ref $__ylc_play_pattern_step_12
    %36 = call %34(%31, %35, %33)[2m : Ptr[0m[2m ; ops [%34:callee/borrow#0, %31:value/borrow#0, %35:value/borrow#1, %33:value/consume#2][0m
    %37 = const.double 0
    %38 = fn_ref $c
    %39 = coro.new %38()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#5[0m[2m ; ops [%38:callee/borrow#0][0m
    %40 = fn_ref $ylc_play_pattern_start
    %41 = fn_ref $__ylc_play_pattern_step_13
    %42 = call %40(%37, %41, %39)[2m : Ptr[0m[2m ; ops [%40:callee/borrow#0, %37:value/borrow#0, %41:value/borrow#1, %39:value/consume#2][0m
    %43 = const.double 8
    %44 = global_load @$global.beat[2m : Double[0m
    %45 = fmul %43, %44[2m ; ops [%43:value/borrow#0, %44:value/borrow#1][0m
    yield %45, bb3[2m ; ops [%45:value/borrow#0][0m
  bb3 yield.resume:
    %46 = const.double 0
    %47 = fn_ref $m2
    %48 = coro.new %47()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#6[0m[2m ; ops [%47:callee/borrow#0][0m
    %49 = fn_ref $ylc_play_pattern_start
    %50 = fn_ref $__ylc_play_pattern_step_14
    %51 = call %49(%46, %50, %48)[2m : Ptr[0m[2m ; ops [%49:callee/borrow#0, %46:value/borrow#0, %50:value/borrow#1, %48:value/consume#2][0m
    %52 = const.double 0
    %53 = fn_ref $c
    %54 = coro.new %53()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#7[0m[2m ; ops [%53:callee/borrow#0][0m
    %55 = fn_ref $ylc_play_pattern_start
    %56 = fn_ref $__ylc_play_pattern_step_15
    %57 = call %55(%52, %56, %54)[2m : Ptr[0m[2m ; ops [%55:callee/borrow#0, %52:value/borrow#0, %56:value/borrow#1, %54:value/consume#2][0m
    %58 = const.double 8
    %59 = global_load @$global.beat[2m : Double[0m
    %60 = fmul %58, %59[2m ; ops [%58:value/borrow#0, %59:value/borrow#1][0m
    yield %60, bb4[2m ; ops [%60:value/borrow#0][0m
  bb4 yield.resume:
    %61 = const.double 0
    %62 = fn_ref $m3
    %63 = coro.new %62()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#8[0m[2m ; ops [%62:callee/borrow#0][0m
    %64 = fn_ref $ylc_play_pattern_start
    %65 = fn_ref $__ylc_play_pattern_step_16
    %66 = call %64(%61, %65, %63)[2m : Ptr[0m[2m ; ops [%64:callee/borrow#0, %61:value/borrow#0, %65:value/borrow#1, %63:value/consume#2][0m
    %67 = const.double 0
    %68 = fn_ref $c
    %69 = coro.new %68()[2m : Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>,[0m[2m ; ea heap#9[0m[2m ; ops [%68:callee/borrow#0][0m
    %70 = fn_ref $ylc_play_pattern_start
    %71 = fn_ref $__ylc_play_pattern_step_17
    %72 = call %70(%67, %71, %69)[2m : Ptr[0m[2m ; ops [%70:callee/borrow#0, %67:value/borrow#0, %71:value/borrow#1, %69:value/consume#2][0m
    %73 = const.double 8
    %74 = global_load @$global.beat[2m : Double[0m
    %75 = fmul %73, %74[2m ; ops [%73:value/borrow#0, %74:value/borrow#1][0m
    yield %75, bb5[2m ; ops [%75:value/borrow#0][0m
  bb5 yield.resume:
    coro.restart bb0
}
fn __ylc_play_pattern_step_8(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_9(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_10(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_11(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_12(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_13(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_14(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_15(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_16(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_17(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_18(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Double
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
fn __ylc_play_pattern_step_19(%0 handle[2m: Coroutine of Double with From <Array of Double>,From <List
  | []
  | :: of Double, List
>, @borrow[0m, %1 tick[2m: Uint64 @borrow[0m)[2m -> () @none[0m {
  bb0 entry:
    br bb1
  bb1 coro.resume:
    %2 = coro.next %0()[2m : Option
  | Some of Doub[32mInput devices:
  0: Scarlett 4i4 USB Analog Surround 2.1: capture_FL, capture_FR, capture_LFE (default)
  1: Scarlett 4i4 USB Analog Surround 4.0: monitor_FL, monitor_FR, monitor_RL, monitor_RR
  2: GB203 High Definition Audio Controller Digital Stereo (HDMI): monitor_FL, monitor_FR
Output devices:
  0: Scarlett 4i4 USB Analog Surround 4.0: playback_FL, playback_FR, playback_RL, playback_RR (default)
  1: GB203 High Definition Audio Controller Digital Stereo (HDMI): playback_FL, playback_FR
[0m[31mInput device: Scarlett 4i4 USB Analog Surround 2.1: capture_FL, capture_FR, capture_LFE
Output device: Scarlett 4i4 USB Analog Surround 4.0: playback_FL, playback_FR, playback_RL, playback_RR
Input device has 3 channels: 1,2,3
Output device has 4 channels: 1,2,3,4
Input device software latency - min: 0.0213 sec, max: 0.0213 sec, current: 0.0213 sec
Output device software latency - min: 0.0213 sec, max: 0.0213 sec, current: 0.0213 sec
[0m[31mActual output latency: 0.0213 sec
[34mFormat: float 32-bit LE
Sample rate: 48000 Hz
Channel configuration:

  Output channels: 1 2 Press Ctrl+C to exit.
[0m
Starting streams...
le
  | None
[0m[2m ; ops [%0:callee/borrow#0][0m
    %3 = extract.variant_tag %2[2m : Char[0m[2m ; ops [%2:value/borrow#0][0m
    %4 = tag_eq %3, Some#0[2m ; ops [%3:tag/borrow#0][0m
    cond %4, bb2, bb3[2m ; ops [%4:condition/borrow#0][0m
  bb2 coro.value:
    %5 = extract.variant_payload %2, Some#0[2m : Some of Double[0m[2m ; ops [%2:container/borrow#0][0m
    %6 = extract.field %5, 0[2m : Double[0m[2m ; ops [%5:container/borrow#0][0m
    %7 = fn_ref $ylc_schedule_current_task_event
    %8 = call %7(%1, %6)[2m : Ptr[0m[2m ; ops [%7:callee/borrow#0, %1:value/borrow#0, %6:value/borrow#1][0m
    %9 = const.void
    return %9[2m ; ops [%9:return/consume#0][0m
  bb3 coro.finished:
    %10 = fn_ref $ylc_complete_current_task
    %11 = call %10()[2m : ()[0m[2m ; ops [%10:callee/borrow#0][0m
    %12 = const.void
    return %12[2m ; ops [%12:return/consume#0][0m
}
  === Input Mapping ===
  ====================


[?2004h[31mλ [0m[36maudio: first output callback min=1024 max=1024 capacity=1280 layout=2 bytes/sample=4
audio: begin_write frames=1024 areas=0x55dbc45985e0
audio: end_write ok
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
hihats!!!0.250000
