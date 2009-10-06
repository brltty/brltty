;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; libbrlapi - A library providing access to braille terminals for applications.
;
; Copyright (C) 2006-2009 by Mario Lang <mlang@delysid.org>
;
; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;
; This is free software, placed under the terms of the
; GNU Lesser General Public License, as published by the Free Software
; Foundation; either version 2.1 of the License, or (at your option) any
; later version. Please see the file LICENSE-LGPL for details.
;
; Web Page: http://mielke.cc/brltty/
;
; This software is maintained by Dave Mielke <dave@mielke.cc>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(eval-when (:compile-toplevel)
  (declaim (optimize (safety 3) (debug 3))))

;;;; * Package definition

(defpackage :brlapi
  (:use :common-lisp :cffi)
  (:export #:open-connection #:close-connection
	   #:driver-name #:display-size
           #:enter-tty-mode #:leave-tty-mode
           #:write-text #:write-dots #:write-region
           #:read-key #:expand-key))
(in-package :brlapi)


;;;; * C BrlAPI Library loading

(define-foreign-library libbrlapi
  (:unix (:or "libbrlapi.so.0.5.4" "libbrlapi.so.0.5"))
  (t (:default "libbrlapi")))
(use-foreign-library libbrlapi)

;;;; * The DISPLAY class

(defclass display ()
  ((handle :initarg :handle :reader display-handle)
   (fd :initarg :fd :reader display-file-descriptor)
   (auth :initarg :auth :reader display-auth)
   (host :initarg :host :reader display-host)
   (tty :initform nil :reader display-tty)))

(defmethod print-object ((obj display) stream)
  (print-unreadable-object (obj stream :type t)
    (if (not (display-file-descriptor obj))
        (format stream "disconnected")
      (apply #'format stream "~Dx~D fd=~D, host=~A, driver=~A"
             (concatenate 'list (multiple-value-list (display-size obj))
                          (list (display-file-descriptor obj) (display-host obj)
                                (driver-name obj)))))))


;;;; * Error handling

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


;;;; * Connection management

(defcstruct settings
  "Connection setting structure."
  (auth :string)
  (host :string))

(defun open-connection (&optional auth host)
  "Open a new connection to BRLTTY on HOST usng AUTH for authorization.
Return a DISPLAY object which can further be used to interact with BRLTTY."
  (with-foreign-object (settings 'settings)
    (setf (foreign-slot-value settings 'settings 'auth)
          (if (stringp auth) auth (null-pointer))
          (foreign-slot-value settings 'settings 'host)
          (if (stringp host) host (null-pointer)))
    (let* ((handle (foreign-alloc :char :count (foreign-funcall "brlapi_getHandleSize" :int)))
           (fd (foreign-funcall "brlapi__openConnection"
                                :pointer handle
                                :pointer settings
                                :pointer settings
                                brlapi-code))
           (display (make-instance 'display :handle handle :fd fd
                                   :auth (foreign-slot-value settings 'settings 'auth)
                                   :host (foreign-slot-value settings 'settings 'host))))
      #+sbcl (sb-ext:finalize display (lambda ()
                                        (foreign-funcall "brlapi__closeConnection" :pointer handle :void)
                                        (foreign-free handle)))
      display)))

(defmethod close-connection ((obj display))
  (foreign-funcall "brlapi__closeConnection" :pointer (display-handle obj) :void)
  (setf (slot-value obj 'fd) nil)
  (foreign-free (display-handle obj))
  (setf (slot-value obj 'handle) nil))

;;;; * Querying the display

(defmethod driver-name ((obj display))
  "Return the currently used driver name."
  (with-foreign-pointer-as-string ((str str-size) 64)
    (foreign-funcall "brlapi__getDriverName" :pointer (display-handle obj)
                     :string str :int str-size brlapi-code)))

(defmethod display-size ((obj display))
  "Return the dimensions of DISPLAY as multiple values.
The first value represents the x dimension and the second the y dimension."
  (with-foreign-objects ((x :int) (y :int))
    (foreign-funcall "brlapi__getDisplaySize" :pointer (display-handle obj) :pointer x :pointer y brlapi-code)
    (values (mem-ref x :int) (mem-ref y :int))))


;;;; * TTY mode

(defmethod enter-tty-mode ((obj display) tty &optional (driver ""))
  (declare (integer tty))
  (declare (string driver))
  (setf (slot-value obj 'tty) (foreign-funcall "brlapi__enterTtyMode" :pointer (display-handle obj) :int tty :string driver brlapi-code)))

(defmethod leave-tty-mode ((obj display))
  (foreign-funcall "brlapi__leaveTtyMode" :pointer (display-handle obj) brlapi-code)
  (setf (slot-value obj 'tty) nil))


;;;; * Output

(defmethod write-text ((obj display) text &key (cursor -1))
  "Write TEXT (a string) to the braille display."
  (declare (string text))
  (declare (integer cursor))
  (if (eql (foreign-funcall "brlapi__writeText"
                            :pointer (display-handle obj)
                            :int cursor :string text
                            brlapi-code)
           0)
      text))

(defbitfield (dots :uint8)
  (:dot1 #x01) :dot2 :dot3 :dot4 :dot5 :dot6 :dot7 :dot8)
(defmethod write-dots ((obj display) &rest dots-list)
  "Write the given dots list to the display."
  (with-foreign-object (dots 'dots (display-size obj))
    (loop for i below (min (display-size obj) (length dots-list))
          do (setf (mem-aref dots 'dots i)
                   (foreign-bitfield-value 'dots (nth i dots-list))))
    (loop for i from (length dots-list) below (display-size obj)
          do (setf (mem-aref dots 'dots i) 0))
    (foreign-funcall "brlapi__writeDots" :pointer (display-handle obj) :pointer dots brlapi-code)))

(defcstruct write-struct
  (display-number :int)
  (region-begin :int)
  (region-size :int)
  (text :pointer)
  (attr-and :pointer)
  (attr-or :pointer)
  (cursor :int)
  (charset :string))

(defmethod write-region ((obj display) text &key (begin 1) size (cursor -1) (display-number -1)
                         (charset "") attr-and attr-or)
  "Update a specific region of the braille display and apply and/or masks."
  (let ((size (or size (min (display-size obj)
                            (max (length text)
                                 (length attr-and)
                                 (length attr-or))))))    
    (with-foreign-objects ((ws 'write-struct)
                           (txt :string (1+ size))
                           (attra 'dots size)
                           (attro 'dots size))
      (loop for i below size
            do (setf (mem-aref txt :uint8 i) (char-code #\SPACE)))
      (setf (mem-aref txt :uint8 size) 0)
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
      (eql (foreign-funcall "brlapi__write" :pointer (display-handle obj) :pointer ws brlapi-code) 0))))


;;;; * Input

(defctype key-code :uint64)
(defmethod read-key ((obj display) &optional block)
  (with-foreign-object (key 'key-code)
    (case (foreign-funcall "brlapi__readKey" :pointer (display-handle obj) :boolean block :pointer key brlapi-code)
      (0 nil)
      (1 (mem-ref key 'key-code)))))

(defun expand-key (code)
  (with-foreign-objects ((command :int) (arg :int) (flags :int))
    (foreign-funcall "brlapi_expandKeyCode"
                     key-code code
                     :pointer command :pointer arg :pointer flags
                     :int)
    (values (intern (foreign-funcall "brlapi_getKeyName"
                                     :int (mem-ref command :int)
                                     :int (mem-ref arg :int)
                                     :string))
            (mem-ref arg :int)
            (mem-ref flags :int))))

;;;; * Example usage

(defun example (&optional (tty -1))
  "A basic example."
  (let ((display (open-connection)))
    (enter-tty-mode display tty)
    (write-text display "Press any key to continue...")
    (apply #'format t "; Command: ~A, argument: ~D, flags: ~D"
           (multiple-value-list (expand-key (read-key display t))))
    (leave-tty-mode display)
    (close-connection display)))

