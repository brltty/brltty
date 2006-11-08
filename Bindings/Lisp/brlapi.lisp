;;; -*- outline-regexp: ";;;;;*"; indent-tabs-mode: nil -*-

(eval-when (:compile-toplevel)
  (declaim (optimize (safety 3) (debug 3))))
(defpackage :brlapi
  (:use :common-lisp :cffi)
  (:export #:connect #:disconnect
	   #:driver-id #:driver-name #:display-size
           #:get-tty #:leave-tty
           #:write-text #:write-dots #:write-region
           #:read-key #:expand-key))
(in-package :brlapi)


;;;; Library loading

(define-foreign-library libbrlapi
  (:unix (:or "libbrlapi.so.0.5.0" "libbrlapi.so"))
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
  (declare (integer value))
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
  (host :string))
(defcfun ("brlapi_closeConnection" disconnect) :void)
(defun connect (&optional authkey host)
  (with-foreign-object (settings 'settings)
    (setf (foreign-slot-value settings 'settings 'authKey)
          (if (stringp authkey) authkey (null-pointer))
	  (foreign-slot-value settings 'settings 'host)
          (if (stringp host) host (null-pointer)))
    (let ((handle (foreign-funcall "brlapi_openConnection"
				   :pointer settings
				   :pointer settings
				   brlapi-code)))
      (values handle
	      (foreign-slot-value settings 'settings 'authKey)
	      (foreign-slot-value settings 'settings 'host)))))

(defun driver-id ()
  (with-foreign-pointer-as-string (str 4 str-size)
    (foreign-funcall "brlapi_getDriverId"
                     :string str :int str-size
                     brlapi-code)))

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
  (declare (integer tty))
  (unless (stringp how) (setf how ""))
  (foreign-funcall "brlapi_enterTtyMode" :int tty :string how brlapi-code))

(defcfun ("brlapi_leaveTtyMode" leave-tty) brlapi-code)


;;;; Output

(defun write-text (text &key (cursor -1))
  "Write TEXT (a string) to the braille display."
  (declare (string text))
  (declare (integer cursor))
  (if (eql (foreign-funcall "brlapi_writeText"
                            :int cursor :string text
                            brlapi-code)
           0)
      text))

(defbitfield (dots :uint8)
  (:dot1 #x01) :dot2 :dot3 :dot4 :dot5 :dot6 :dot7 :dot8)
(defun write-dots (&rest dots-list)
  "Write the given dots list to the display."
  (with-foreign-object (dots 'dots (display-size))
    (loop for i below (min (display-size) (length dots-list))
          do (setf (mem-aref dots 'dots i)
                   (foreign-bitfield-value 'dots (nth i dots-list))))
    (loop for i from (length dots-list) below (display-size)
          do (setf (mem-aref dots 'dots i) 0))
    (foreign-funcall "brlapi_writeDots" :pointer dots brlapi-code)))

(defcstruct write-struct
  (display-number :int)
  (region-begin :int)
  (region-size :int)
  (text :pointer)
  (attr-and :pointer)
  (attr-or :pointer)
  (cursor :int)
  (charset :string))

(defun write-region (text &key (begin 1) size (cursor -1) (display-number -1)
                          (charset "") attr-and attr-or)
  "Update a specific region of the braille display and apply and/or masks."
  (let ((size (or size (min (display-size)
                            (max (length text)
                                 (length attr-and)
                                 (length attr-or))))))    
    (with-foreign-objects ((ws 'write-struct)
                           (txt :string size)
                           (attra 'dots size)
                           (attro 'dots size))
      (loop for i below size
            do (setf (mem-aref txt :uint8 i) (char-code #\SPACE)))
      (loop for i below (min size (length text))
            do (setf (mem-aref txt :uint8 i) (char-code (aref text i))))
      (loop for i below size
            do (setf (mem-aref attra :uint8 i) #XFF (mem-aref attro :uint8 i) 0))
      (loop for i below (min size (length attr-and))
            do (setf (mem-aref attra :uint8 i) (foreign-bitfield-value 'dots (nth i attr-and))))
      (loop for i below (min size (length attr-or))
            do (setf (mem-aref attro :uint8 i) (foreign-bitfield-value 'dots (nth i attr-or))))
      (setf (foreign-slot-value ws 'write-struct 'display-number) display-number
            (foreign-slot-value ws 'write-struct 'cursor) cursor
            (foreign-slot-value ws 'write-struct 'region-begin) begin
            (foreign-slot-value ws 'write-struct 'region-size) size
            (foreign-slot-value ws 'write-struct 'charset) charset
            (foreign-slot-value ws 'write-struct 'text) txt
            (foreign-slot-value ws 'write-struct 'attr-or) attro
            (foreign-slot-value ws 'write-struct 'attr-and) attra)
      (eql (foreign-funcall "brlapi_write" :pointer ws brlapi-code) 0))))


;;;; Input

(defctype key-code :uint64)
(defun read-key (&optional block)
  (with-foreign-object (key 'key-code)
    (case (foreign-funcall "brlapi_readKey" :boolean block :pointer key brlapi-code)
      (0 nil)
      (1 (mem-ref key 'key-code)))))

(defun expand-key (keycode)
  (with-foreign-objects ((command :int) (arg :int) (flags :int))
    (foreign-funcall "brlapi_expandKeyCode"
                     key-code keycode
                     :pointer command :pointer arg :pointer flags
                     :int)
    (values (intern (foreign-funcall "brlapi_getKeyName"
                                     :int (mem-ref command :int)
                                     :int (mem-ref arg :int)
                                     :string))
            (mem-ref arg :int)
            (mem-ref flags :int))))
