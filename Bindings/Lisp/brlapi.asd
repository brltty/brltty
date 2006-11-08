;;;; * BrlAPI System Definition

(asdf:defsystem :brlapi
  :name "brlapi"
  :author "Mario Lang <mlang@delysid.org>"
  :depends-on (:cffi)
  :serial t
  :components ((:file "brlapi")))

;;;;@include "brlapi.lisp"
