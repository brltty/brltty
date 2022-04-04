(load "brlapi")

(defvar brl (brlapi-open-connection))

(message "Driver Name: %s" (brlapi-get-driver-name brl))

(brlapi-close-connection brl)

