# simple-synth
simple C synthesis environment for learning purposes  
including bindings for ocaml for creating synth nodes 
highly unsafe  

## dependencies
- [libsoundio](http://libsound.io/)
- [ocaml](https://ocaml.org/)

# build
```
make synth
```
# build ocaml bindings
```make ocamlbindings```

```make utop_test```
opens utop ocaml repl with synth bindings loaded
# make a square wave
```ocaml
let s = sq_detune 100.;; (* create a square wave node *)
play s;; (* add it to the live synthesis graph which writes its result to the DAC *)
```

