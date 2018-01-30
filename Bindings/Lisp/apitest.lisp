;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; libbrlapi - A library providing access to braille terminals for applications.
;
; Copyright (C) 2006-2018 by Mario Lang <mlang@delysid.org>
;
; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;
; This is free software, placed under the terms of the
; GNU Lesser General Public License, as published by the Free Software
; Foundation; either version 2.1 of the License, or (at your option) any
; later version. Please see the file LICENSE-LGPL for details.
;
; Web Page: http://brltty.com/
;
; This software is maintained by Dave Mielke <dave@mielke.cc>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(require 'asdf)
(require 'sb-posix)

(pushnew
  (concatenate 'string (sb-posix:getcwd) "/")
  asdf:*central-registry*
  :test #'equal
)

(asdf:operate 'asdf:load-op 'brlapi)
(require 'brlapi)

(defun brlapi-test (&optional)
  "Perform a test of various BrlAPI functiins."

  (let (
      (display (brlapi:open-connection))
    )

    (brlapi:print-object display *standard-output*)
    (brlapi:close-connection display)
  )
)

(brlapi-test)
