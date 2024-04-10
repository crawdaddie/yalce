open Ctypes
open Foreign

let clock_push = foreign "clock_push" (ptr void @-> double @-> returning void)
