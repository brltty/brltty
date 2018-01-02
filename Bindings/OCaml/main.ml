(*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2018 by
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 * All rights reserved.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
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

type writeArguments = {
  mutable displayNumber : int;
  mutable regionBegin : int;
  mutable regionSize : int;
  text : string;
  attrAnd : int array;
  attrOr : int array;
  mutable cursor : int;
  mutable charset : string
}

let writeArguments_initializer = {
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
  | AUTHENTICATION
  | Unknown of int

type error = {
  brlerrno : int;
  libcerrno : int;
  gaierrno : int;
  errfun : string;
}

external errorCode_of_error :
  error -> errorCode = "brlapiml_errorCode_of_error"

external strerror :
  error -> string = "brlapiml_strerror"

exception Brlapi_error of error
exception Brlapi_exception of errorCode * int32 * string

external openConnection :
  settings -> Unix.file_descr * settings = "brlapiml_openConnection"
external openConnectionWithHandle :
  settings -> handle = "brlapiml_openConnectionWithHandle"
external closeConnection :
  ?h:handle -> unit -> unit = "brlapiml_closeConnection"
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
  ?h:handle -> writeArguments -> unit = "brlapiml_write"

external readKey :
  ?h:handle -> unit -> int64 option = "brlapiml_readKey"
external waitKey :
  ?h:handle -> unit -> int64 = "brlapiml_waitKey"

type expandedKeyCode = {
  type_ : int32;
  command : int32;
  argument : int32;
  flags : int32
}

external expandKeyCode :
  ?h:handle -> int64 -> expandedKeyCode = "brlapiml_expandKeyCode"

type rangeType =
  | RT_all
  | RT_type
  | RT_command
  | RT_key
  | RT_code

external ignoreKeys :
  ?h:handle -> rangeType -> int64 array -> unit = "brlapiml_ignoreKeys"
external acceptKeys :
  ?h:handle -> rangeType -> int64 array -> unit = "brlapiml_acceptKeys"
external ignoreAllKeys :
  ?h:handle -> unit = "brlapiml_ignoreAllKeys"
external acceptAllKeys :
  ?h:handle -> unit = "brlapiml_acceptAllKeys"  
external ignoreKeyRanges :
  ?h:handle -> (int64 * int64) array -> unit = "brlapiml_ignoreKeyRanges"
external acceptKeyRanges :
  ?h:handle -> (int64 * int64) array -> unit = "brlapiml_acceptKeyRanges"

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
  let ignoreKeys ?h t a = ignoreKeys ?h t (Array.map M1.int64_of_key a)
  let acceptKeys ?h t a = acceptKeys ?h t (Array.map M1.int64_of_key a)
  let f (x,y) = (M1.int64_of_key x, M1.int64_of_key y)
  let g a = Array.map f a
  let ignoreKeyRanges ?h a = ignoreKeyRanges ?h (g a)
  let acceptKeyRanges ?h a = acceptKeyRanges ?h (g a)
end

  

external setExceptionHandler :
  unit -> unit = "brlapiml_setExceptionHandler"

let _ =
  let x = { brlerrno = 0; libcerrno = 0; gaierrno = 0; errfun = "" } in
  Callback.register_exception "Brlapi_error"
    (Brlapi_error x);
  Callback.register_exception "Brlapi_exception"
    (Brlapi_exception (SUCCESS, 0l, ""));
  setExceptionHandler()
