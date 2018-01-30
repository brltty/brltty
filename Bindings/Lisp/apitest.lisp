(require 'asdf)
(require 'sb-posix)

(setf asdf:*central-registry*
  (list*
    (concatenate 'string (sb-posix:getcwd) "/")
    asdf:*central-registry*
  )
)

(asdf:operate 'asdf:load-op 'brlapi)
(require 'brlapi)

(defun brlapi-test (&optional)
  "Perform a test of various BrlAPI functiins."

  (let (
      (display (brlapi:open-connection))
    )

    (brlapi:close-connection display)
  )
)

(brlapi-test)
