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

(defun current-directory ()
  "The current working directory as a directory path."
  (concatenate 'string (sb-posix:getcwd) "/")
)

(pushnew (current-directory) asdf:*central-registry* :test #'equal)
(asdf:operate 'asdf:load-op 'brlapi)

(defun brlapi-test (&key auth host)
  "Perform a test of various BrlAPI functiins."

  (let (
      (display (brlapi:open-connection :auth auth :host host))
      (output *standard-output*)
    )

    (if (brlapi:is-connected display)
      (progn
        (brlapi:print-properties display output)
        (brlapi:close-connection display)
      )
      (format output "not connected")
    )
  )
)

(defun program-arguments ()
  "The command-line arguments."

  sb-ext:*posix-argv*
)

(defun arguments->keyword-options (arguments)
  "Convert a list of argument strings into a list of keyword options."

  (loop
    for cons on arguments by #'cddr
    append (list (intern (string-upcase (car cons)) "KEYWORD") (car (cdr cons)))
  )
)

(apply #'brlapi-test (arguments->keyword-options (cdr (program-arguments))))
(exit)
