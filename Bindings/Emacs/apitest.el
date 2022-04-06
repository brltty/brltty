(load "brlapi")

(defvar brl (apply #'brlapi-open-connection command-line-args-left))

(message "Driver Name: %s" (brlapi-get-driver-name brl))
(message "Model Identifier: %s" (brlapi-get-model-identifier brl))
(let ((size (brlapi-get-display-size brl)))
  (message "Display Size: %dx%d" (car size) (cdr size)))

(brlapi-close-connection brl)
