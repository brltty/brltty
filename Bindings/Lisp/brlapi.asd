;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; libbrlapi - A library providing access to braille terminals for applications.
;
; Copyright (C) 2006-2008 by Mario Lang <mlang@delysid.org>
;
; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;
; This is free software, placed under the terms of the
; GNU Lesser General Public License, as published by the Free Software
; Foundation; either version 2.1 of the License, or (at your option) any
; later version. Please see the file LICENSE-LGPL for details.
;
; Web Page: http://mielke.cc/brltty/
;
; This software is maintained by Dave Mielke <dave@mielke.cc>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; * BrlAPI System Definition

(asdf:defsystem :brlapi
  :name "brlapi"
  :author "Mario Lang <mlang@delysid.org>"
  :depends-on (:cffi)
  :serial t
  :components ((:file "brlapi")))

;;;;@include "brlapi.lisp"
