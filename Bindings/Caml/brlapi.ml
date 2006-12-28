(*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2005-2006 by
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 *)

(* BrlAPI Interface for the OCaml language *)

let dot1 = 1 lsl 0
let dot2 = 1 lsl 1
let dot3 = 1 lsl 2
let dot4 = 1 lsl 3
let dot5 = 1 lsl 4
let dot6 = 1 lsl 5
let dot7 = 1 lsl 6
let dot8 = 1 lsl 7

type settings = {
  auth : string;
  host : string
}

let settings_initializer = {
  auth = "";
  host = ""
}

type writeStruct = {
  mutable displayNumber : int;
  mutable regionBegin : int;
  mutable regionSize : int;
  text : string;
  attrAnd : int array;
  attrOr : int array;
  mutable cursor : int;
  mutable charset : string
}

let writeStruct_initializer = {
  displayNumber = -1;
  regionBegin = 0;
  regionSize = 0;
  text = "";
  attrAnd = [| |];
  attrOr = [| |];
  cursor = -1;
  charset = ""
}

type handle

let max_packet_size = 512
let max_keyset_size = 128

type errorCode =
  | SUCCESS
  | NOMEM
  | TTYBUSY
  | DEVICEBUSY
  | UNKNOWN_INSTRUCTION
  | ILLEGAL_INSTRUCTION
  | INVALID_PARAMETER
  | INVALID_PACKET
  | CONNREFUSED
  | OPNOTSUPP
  | GAIERR of int
  | LIBCERR of Unix.error
  | UNKNOWNTTY
  | PROTOCOL_VERSION
  | EOF
  | EMPTYKEY
  | DRIVERERROR
  | Unknown of int

type error = {
  errorCode : errorCode;
  errorFunction : string;
}

external strerror :
  error -> string = "brlapiml_strerror"

exception Brlapi_error of error
exception Brlapi_exception of errorCode * int32 * string

external openConnection :
  settings -> Unix.file_descr * settings = "brlapiml_openConnection"
external openConnectionHandle :
  settings -> handle = "brlapiml_openConnectionHandle"
external closeConnection :
  ?h:handle -> unit -> unit = "brlapiml_closeConnection"
external getDriverId :
  ?h:handle -> string = "brlapiml_getDriverId"
external getDriverName :
  ?h:handle -> unit -> string = "brlapiml_getDriverName"
external getDisplaySize :
  ?h:handle -> unit -> int * int = "brlapiml_getDisplaySize"

external enterTtyMode :
  ?h:handle -> int -> string -> int = "brlapiml_enterTtyMode"
external enterTtyModeWithPath :
  ?h:handle -> int array -> string -> int = "brlapiml_enterTtyModeWithPath"
external leaveTtyMode :
  ?h:handle -> unit -> unit = "brlapiml_leaveTtyMode"
external setFocus :
  ?h:handle -> int -> unit = "brlapiml_setFocus"

external writeText :
  ?h:handle -> int -> string -> unit = "brlapiml_writeText"
external writeDots :
  ?h:handle -> int array -> unit = "brlapiml_writeDots"
external write :
  ?h:handle -> writeStruct -> unit = "brlapiml_write_"

external readKey :
  ?h:handle -> unit -> int64 option = "brlapiml_readKey"
external waitKey :
  ?h:handle -> unit -> int64 = "brlapiml_waitKey"

type key =
  | BrailleCommand of BrailleCommands.t
  | Keysym of int32

type expandedKeyCode = {
  key : key;
  argument : int;
  flags : int32
}

external expandKeyCode :
  int64 -> expandedKeyCode = "brlapiml_expandKeyCode"

external ignoreKeyRange :
  ?h:handle -> int64 -> int64 -> unit = "brlapiml_ignoreKeyRange"
external acceptKeyRange :
  ?h:handle -> int64 -> int64 -> unit = "brlapiml_acceptKeyRange"
external  ignoreKeySet :
  ?h:handle -> int64 array -> unit = "brlapiml_ignoreKeySet"
external acceptKeySet :
  ?h:handle -> int64 array -> unit = "brlapiml_acceptKeySet"

external enterRawMode :
  ?h:handle -> string -> unit = "brlapiml_enterRawMode"
external leaveRawMode :
  ?h:handle -> unit -> unit = "brlapiml_leaveRawMode"
external sendRaw :
  ?h:handle -> string -> int = "brlapiml_sendRaw"
external recvRaw :
  ?h:handle -> unit -> string = "brlapiml_recvRaw"
  
external suspendDriver :
  ?h:handle -> string -> unit = "brlapiml_suspendDriver"
external resumeDriver :
  ?h:handle -> unit -> unit = "brlapiml_resumeDriver"

module type KEY = sig
  type key
  val key_of_int64 : int64 -> key
  val int64_of_key : key -> int64
end

module MakeKeyHandlers (M1 : KEY) = struct
  type key = M1.key
  let readKey ?h () = match readKey ?h () with
    | None -> None
    | Some x -> Some (M1.key_of_int64 x)

  let waitKey ?h () = M1.key_of_int64 (waitKey ?h ()) 
  let ignoreKeyRange ?h k1 k2 = ignoreKeyRange ?h (M1.int64_of_key k1) (M1.int64_of_key k2)
  let acceptKeyRange ?h k1 k2 = acceptKeyRange ?h (M1.int64_of_key k1) (M1.int64_of_key k2)
  let ignoreKeySet ?h keySet = ignoreKeySet ?h (Array.map M1.int64_of_key keySet)
  let acceptKeySet ?h keySet = acceptKeySet ?h (Array.map M1.int64_of_key keySet)
end

  

external setExceptionHandler :
  unit -> unit = "brlapiml_setExceptionHandler"

let _ =
  Callback.register_exception "Brlapi_error"
    (Brlapi_error { errorCode = SUCCESS; errorFunction = "" } );
  Callback.register_exception "Brlapi_exception"
    (Brlapi_exception (SUCCESS, 0l, ""));
  setExceptionHandler()
