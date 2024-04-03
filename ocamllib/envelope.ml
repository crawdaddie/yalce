open Ctypes
open Foreign

let trig_env_node =
  foreign "env_node" (int @-> ptr double @-> ptr double @-> returning Node.node)
;;

let autotrig_env_node =
  foreign "autotrig_env_node" (int @-> ptr double @-> ptr double @-> returning Node.node)
;;

let trig levels times =
  trig_env_node
    (List.length times)
    (levels |> CArray.of_list double |> CArray.start)
    (times |> CArray.of_list double |> CArray.start)
;;

let autotrig levels times =
  autotrig_env_node
    (List.length times)
    (levels |> CArray.of_list double |> CArray.start)
    (times |> CArray.of_list double |> CArray.start)
;;
