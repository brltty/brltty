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
(*
Ajouter des fichiers .in
*)

(*
BRLAPI_RELEASE
BRLAPI_MAJOR
BRLAPI_MINOR
BRLAPI_REVISION
Faire un fichier .in: @VAR@ est remplacé par la valeur de VAR
telle que définie dans configure.ac.
*)

(*
BRLAPI_SOCKETPORTNUM
BRLAPI_SOCKETPORT (string)
BRLAPI_MAXPACKETSIZE
BRLAPI_MAXNAMELENGTH
BRLAPI_SOCKETPATH
BRLAPI_ETCDIR
BRLAPI_AUTHKEYFILE
BRLAPI_DEFAUTH
Idem que précédemmment
C'est du ni<eau proto. Faut-il binder ?
*)

type settings = {
  auth : string;
  host : string
}

val settings_initializer : settings

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

val writeStruct_initializer : writeStruct

type handle

val max_packet_size : int
val max_keyset_size : int

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
(*
val expandHost : string -> string * string
Idem: proto
*)
external getDriverId :
  ?h:handle -> string = "brlapiml_getDriverId"
external getDriverName :
  ?h:handle -> unit -> string = "brlapiml_getDriverName"
external getDisplaySize :
  ?h:handle -> unit -> int * int = "brlapiml_getDisplaySize"

external enterTtyMode :
  ?h:handle -> int -> string -> int = "brlapiml_enterTtyMode"
val enterTtyModeWithPath :
  ?h:handle -> int array -> string -> int
external leaveTtyMode :
  ?h:handle -> unit -> unit = "brlapiml_leaveTtyMode"
external setFocus :
  ?h:handle -> int -> unit = "brlapiml_setFocus"

external writeText :
  ?h:handle -> int -> string -> unit = "brlapiml_writeText"
(* Arg optionnel pour curseur ? *)
external writeDots :
  ?h:handle -> int array -> unit = "brlapiml_writeDots"
external write :
  ?h:handle -> writeStruct -> unit = "brlapiml_write_"

(*
BRLAPI_KEYCODE_MAX
Les constantes pour travailler sur les flags de touches ?
Les KEY_SYM ?
Oui: pour être cohérent avec le fait de fournir le int64
Flags: entiers.
*)

(* val getKeyName : expandedKeyCode -> string *)

(*
brlapi_dotNumberToBit
brlapi_dotBitToNumber
Unicode ?
Oui, utile pour les key_sym
*)

external readKey :
  ?h:handle -> unit -> int64 option = "brlapiml_readKey"
external waitKey :
  ?h:handle -> unit -> int64 = "brlapiml_waitKey"

type expandedKeyCode = {
  kc_type : int;
  kc_command : int;
  kc_argument : int;
  kc_flags : int
}

external expandKeyCode :
  int64 -> expandedKeyCode = "brlapiml_expandKeyCode"

external ignoreKeyRange :
  ?h:handle -> int64 -> int64 -> unit = "brlapiml_ignoreKeyRange"
external acceptKeyRange :
  ?h:handle -> int64 -> int64 -> unit = "brlapiml_acceptKeyRange"
external ignoreKeySet :
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

module MakeKeyHandlers (M1 : KEY) : sig
  type key = M1.key
  val readKey : ?h:handle -> unit -> key option
  val waitKey : ?h:handle -> unit -> key

  val ignoreKeyRange : ?h:handle -> key -> key -> unit
  val acceptKeyRange : ?h:handle -> key -> key -> unit
  val ignoreKeySet : ?h:handle -> key array -> unit
  val acceptKeySet : ?h:handle -> key array -> unit
end
