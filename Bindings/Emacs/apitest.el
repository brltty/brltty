(load "brlapi")

(defvar brl (brlapi-open-connection))

(message "Driver Name: %s" (brlapi-get-driver-name brl))
(message "Display Size: %S" (brlapi-get-display-size brl))

(brlapi-close-connection brl)
