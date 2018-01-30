(require 'asdf)

(setf asdf:*central-registry*
  (list*
    #p"/home/dave/brltty/Bindings/Lisp/"
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
