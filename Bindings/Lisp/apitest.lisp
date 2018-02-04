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

(load "apisetup.lisp")

(defun brlapi-test-tty (output session tty)
  "Perform a test of BrlAPI's input functions."

  (brlapi:enter-tty-mode session tty)
  (brlapi:write-text session "Press any key to continue...")
  (apply #'format output "Key: type=~A command=~A argument=~D flags=~{~A~^,~}~%"
         (brlapi:expand-key-code (brlapi:read-key session t)))
  (brlapi:leave-tty-mode session)
)

(defun brlapi-test (&key auth host tty)
  "Perform a test of various BrlAPI functiins."

  (let (
      (output *standard-output*)
      (session (brlapi:open-connection :auth auth :host host))
    )

    (if (brlapi:is-connected session)
      (progn
        (format output "Session: ")
        (brlapi:print-properties session output)
        (format output "~%")

        (if tty (brlapi-test-tty output session (parse-integer tty)))
      )
      (format output "connection failure~%")
    )

    (brlapi:close-connection session)
  )
)

(defun program-arguments ()
  "The command-line arguments."

  sb-ext:*posix-argv*
)

(defun string->keyword (string)
  "Convert a string to a keyword."

  (values (intern string "KEYWORD"))
)

(defun arguments->keyword-options (arguments)
  "Convert a list of argument strings into a list of keyword options."

  (loop
    for (option value) on arguments by #'cddr
    append (list (string->keyword (string-upcase option)) value)
  )
)

(apply #'brlapi-test (arguments->keyword-options (cdr (program-arguments))))
(exit)
