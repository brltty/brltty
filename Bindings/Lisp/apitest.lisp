;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; libbrlapi - A library providing access to braille terminals for applications.
;
; Copyright (C) 2006-2019 by Mario Lang <mlang@delysid.org>
;
; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;
; This is free software, placed under the terms of the
; GNU Lesser General Public License, as published by the Free Software
; Foundation; either version 2.1 of the License, or (at your option) any
; later version. Please see the file LICENSE-LGPL for details.
;
; Web Page: http://brltty.app/
;
; This software is maintained by Dave Mielke <dave@mielke.cc>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "apisetup.lisp")

(defun brlapi-test-tty (output session &key tty timeout)
  "Perform a test of BrlAPI's input functions."

  (setf tty (if tty  (parse-integer tty) -1))
  (setf timeout (if timeout  (parse-integer timeout) 10))

  (brlapi:enter-tty-mode session tty)
  (brlapi:write-text session (format nil "press keys (timeout is ~D seconds)" timeout))

  (loop
    with code
    with text
    while (setf code (brlapi:read-key-with-timeout session timeout))
    do (setf text
         (apply #'format nil
           "code=0X~X type=~A cmd=~A arg=~D flg=~{~A~^,~}"
           (list* code (brlapi:describe-key-code code))))
       (format output "Key: ~A~%" text)
       (brlapi:write-text session text)
  )
  (brlapi:leave-tty-mode session)
)

(defun brlapi-test (&key host auth tty timeout)
  "Perform a test of various BrlAPI functiins."

  (let (
      (output *standard-output*)
      (session (brlapi:open-connection :auth auth :host host))
    )

    (loop
      for (name value format) in (brlapi:property-list session) by #'cdr
      do (format output "~A: ~@?~%" name format value)
    )

    (if (brlapi:is-connected session)
      (progn
        (if (or tty timeout)
          (brlapi-test-tty output session :tty tty :timeout timeout))
      )
      (format output "connection failure: ~A~%" (brlapi:error-message))
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
