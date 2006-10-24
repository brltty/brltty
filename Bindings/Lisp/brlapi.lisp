;;; -*- outline-regexp: ";;;;;*"; indent-tabs-mode: nil -*-

(defpackage :brlapi
  (:use :common-lisp :cffi)
  (:export #:connect #:disconnect
	   #:driver-id #:driver-name #:display-size
           #:get-tty #:leave-tty
           #:write-text #:read-key))
(in-package :brlapi)


;;;; Library loading

(define-foreign-library libbrlapi
  (:unix (:or "libbrlapi.so.0.4.1" "libbrlapi.so"))
  (t (:default "libbrlapi")))
(use-foreign-library libbrlapi)


;;;; Error handling

(defctype brlapi-code :int)
(define-condition brlapi-error (error)
  ((text :initarg :text :reader brlapi-error-text))
  (:report (lambda (c stream)
             (format stream "libbrlapi function returned ~A"
                     (brlapi-error-text c))))
  (:documentation "Signalled when a libbrlapi function answers with -1."))

(defmethod translate-from-foreign (value (name (eql 'brlapi-code)))
  "Raise a BRLAPI-ERROR if VALUE, a brlapi-code, is -1."
  (if (eql value -1)
      (error 'brlapi-error
             :text (foreign-funcall "brlapi_strerror"
                                    :pointer
                                    (foreign-funcall "brlapi_error_location"
                                                     :pointer)
                                    :string))
    value))


;;;; Connection management

(defcstruct settings
  "Connection settings."
  (authKey :string)
  (hostName :string))
(defcfun ("brlapi_closeConnection" disconnect) :void)
(defun connect (&optional authkey hostname)
  (with-foreign-object (settings 'settings)
    (setf (foreign-slot-value settings 'settings 'authKey)
          (if (stringp authkey) authkey (null-pointer))
	  (foreign-slot-value settings 'settings 'hostName)
          (if (stringp hostname) hostname (null-pointer)))
    (let ((handle (foreign-funcall "brlapi_initializeConnection"
				   :pointer settings
				   :pointer settings
				   brlapi-code)))
      (values handle
	      (foreign-slot-value settings 'settings 'authKey)
	      (foreign-slot-value settings 'settings 'hostName)))))

(defun driver-id ()
  (with-foreign-pointer-as-string (str 4 str-size)
    (foreign-funcall "brlapi_getDriverId"
                     :string str :int str-size brlapi-code)))

(defun driver-name ()
  (with-foreign-pointer-as-string (str 64 str-size)
    (foreign-funcall "brlapi_getDriverName"
                     :string str :int str-size brlapi-code)))

(defun display-size ()
  (with-foreign-objects ((x :int) (y :int))
    (foreign-funcall "brlapi_getDisplaySize" :pointer x :pointer y brlapi-code)
    (values (mem-ref x :int) (mem-ref y :int))))


;;;; TTY acquisition

(defun get-tty (tty &optional how)
  (unless (stringp how) (setf how ""))
  (foreign-funcall "brlapi_getTty" :int tty :string how brlapi-code))

(defcfun ("brlapi_leaveTty" leave-tty) brlapi-code)


;;;; Output

(defcfun ("brlapi_writeText" write-text) brlapi-code
  (cursor :int)
  (text :string))

(defbitfield (dots :uint8)
  (:dot1 #x01) :dot2 :dot3 :dot4 :dot5 :dot6 :dot7 :dot8)
(defun write-dots (&rest dots-list)
  (with-foreign-object (dots 'dots (display-size))
    (loop for i below (min (display-size) (length dots-list))
          do (setf (mem-aref dots 'dots i)
                   (foreign-bitfield-value 'dots (nth i dots-list))))
    (loop for i from (length dots-list) below (display-size)
          do (setf (mem-aref dots 'dots i) 0))
    (foreign-funcall "brlapi_writeDots" :pointer dots brlapi-code)))


;;;; Input

(defctype key-code :uint64)
(defun read-key (&optional block)
  (with-foreign-object (key 'key-code)
    (case (foreign-funcall "brlapi_readKey" :boolean block :pointer key brlapi-code)
      (0 nil)
      (1 (mem-ref key 'key-code)))))
