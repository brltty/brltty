(load "brlapi")

(defvar brl (apply #'brlapi-open-connection command-line-args-left))

(let ((version (brlapi-get-library-version)))
  (message "Library Version: %s.%s.%s"
            (plist-get version :major)
            (plist-get version :minor)
            (plist-get version :revision)))
(message "Driver Name: %s" (brlapi-get-driver-name brl))
(message "Model Identifier: %s" (brlapi-get-model-identifier brl))
(let ((size (brlapi-get-display-size brl)))
  (message "Display Size: %dx%d" (car size) (cdr size)))

(message "Bound Commands: %S"
         (mapcar (lambda (code) (brlapi-get-command-keycode-name brl code))
                 (brlapi-get-bound-command-key-codes brl)))

(brlapi-close-connection brl)
