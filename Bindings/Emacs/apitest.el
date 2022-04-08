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

(message "Computer Braille Table: %s" (brlapi-get-computer-braille-table brl))
(message "Literary Braille Table: %s" (brlapi-get-literary-braille-table brl))
(message "Message Locale: %s" (brlapi-get-message-locale brl))

(message "Bound Command Keycode Names: %S"
         (mapcar (lambda (code) (brlapi-get-command-keycode-name brl code))
                 (brlapi-get-bound-command-keycodes brl)))

(message "Defined Driver Keycode Names: %S"
         (mapcar (lambda (code) (brlapi-get-driver-keycode-name brl code))
                 (brlapi-get-defined-driver-keycodes brl)))

(brlapi-close-connection brl)
