;;;; libbrlapi - A library providing access to braille terminals for applications.
;;;;
;;;; Copyright (C) 2006-2021 by Mario Lang <mlang@delysid.org>
;;;;
;;;; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;;;;
;;;; This is free software, placed under the terms of the
;;;; GNU Lesser General Public License, as published by the Free Software
;;;; Foundation; either version 2.1 of the License, or (at your option) any
;;;; later version. Please see the file LICENSE-LGPL for details.
;;;;
;;;; Web Page: http://brltty.app/
;;;;
;;;; This software is maintained by Dave Mielke <dave@mielke.cc>.

(eval-when (:compile-toplevel)
  (declaim (optimize (safety 3) (debug 3))))

(in-package :brlapi)

;;;; * The DISPLAY class

(defclass display ()
  ((handle :initarg :handle :reader connection-handle)
   (fd :initarg :fd :reader file-descriptor)
   (auth :initarg :auth :reader server-auth)
   (host :initarg :host :reader server-host)
   (tty :initform nil :reader entered-tty)))

(defmethod property-list ((obj display))
  "Return various properties as a lsit of three-element lists.
The sublist for each returned property contains, in order, its name, its value, and its format string."
  (list*
    (list "version" (multiple-value-list (library-version)) "~{~D~^.~}")
    (if (is-connected obj)
      (multiple-value-bind (width height) (display-size obj)
        (list
          (list "host" (server-host obj) "~A")
          (list "auth" (server-auth obj) "~A")
          (list "fd" (file-descriptor obj) "~D")
          (list "driver" (driver-name obj) "~A")
          (list "model" (model-identifier obj) "~A")
          (list "width" width "~D")
          (list "height" height "~D")
        )
      )
    )
  )
)

(defmethod print-object ((obj display) stream)
  (print-unreadable-object (obj stream :type t)
    (format stream "~{~A~^ ~}"
      (loop
        for (name value format) in (property-list obj) by #'cdr
        append (list (format nil "~A=~@?" name format value))
      )
    )
  )
)


;;;; * Error handling

(defctype brlapi-code :int)
(define-condition brlapi-error (error)
  ((text :initarg :text :reader brlapi-error-text))
  (:report (lambda (c stream)
             (format stream "libbrlapi function returned ~A"
                     (brlapi-error-text c))))
  (:documentation "Signalled when a libbrlapi function answers with -1."))

(defmethod error-message ()
  "The message for the most recent error."
  (foreign-funcall "brlapi_strerror"
    :pointer (foreign-funcall "brlapi_error_location" :pointer)
    :string)
)

(defmethod translate-from-foreign (value (name (eql 'brlapi-code)))
  "Raise a BRLAPI-ERROR if VALUE, a brlapi-code, is -1."
  (declare (integer value))
  (if (eql value -1)
      (error 'brlapi-error :text (error-message))
      value))


;;;; * Connection management

(defmethod is-connected ((obj display))
  (and
    obj
    (let
      ((fd (file-descriptor obj)))
      (and fd (numberp fd) (not (< fd 0)))
    )
  )
)

(defcstruct settings
  "Connection setting structure."
  (auth :string)
  (host :string))

(defun open-connection (&key auth host)
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
  (foreign-funcall "brlapi__closeConnection" :pointer (connection-handle obj) :void)
  (setf (slot-value obj 'fd) nil)
  (foreign-free (connection-handle obj))
  (setf (slot-value obj 'handle) nil))

;;;; * Querying the display

(defmethod library-version ()
  "Return the version of the API as multiple values (major, minor, revision)."
  (with-foreign-objects ((major :int) (minor :int) (revision :int))
    (foreign-funcall "brlapi_getLibraryVersion"
      :pointer major :pointer minor :pointer revision
      :void
    )
    (values (mem-ref major :int) (mem-ref minor :int) (mem-ref revision :int))
  )
)

(defmethod driver-name ((obj display))
  "Return the currently used driver name."
  (with-foreign-pointer-as-string ((str str-size) 64)
    (foreign-funcall "brlapi__getDriverName" :pointer (connection-handle obj)
                     :string str :int str-size brlapi-code)))

(defmethod model-identifier ((obj display))
  "Return the currently used display model."
  (with-foreign-pointer-as-string ((str str-size) 64)
    (foreign-funcall "brlapi__getModelIdentifier" :pointer (connection-handle obj)
                     :string str :int str-size brlapi-code)))

(defmethod display-size ((obj display))
  "Return the dimensions of DISPLAY as multiple values.
The first value represents the x dimension and the second the y dimension."
  (with-foreign-objects ((x :int) (y :int))
    (foreign-funcall "brlapi__getDisplaySize" :pointer (connection-handle obj) :pointer x :pointer y brlapi-code)
    (values (mem-ref x :int) (mem-ref y :int))))

;;;; * TTY mode

(defmethod enter-tty-mode ((obj display) tty &optional (driver ""))
  (declare (integer tty))
  (declare (string driver))
  (setf (slot-value obj 'tty) (foreign-funcall "brlapi__enterTtyMode" :pointer (connection-handle obj) :int tty :string driver brlapi-code)))

(defmethod leave-tty-mode ((obj display))
  (foreign-funcall "brlapi__leaveTtyMode" :pointer (connection-handle obj) brlapi-code)
  (setf (slot-value obj 'tty) nil))


;;;; * Output

(defmethod write-text ((obj display) text &key (cursor -1))
  "Write TEXT (a string) to the braille display."
  (declare (string text))
  (declare (integer cursor))
  (if (eql (foreign-funcall "brlapi__writeText"
                            :pointer (connection-handle obj)
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
    (foreign-funcall "brlapi__writeDots" :pointer (connection-handle obj) :pointer dots brlapi-code)))

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
      (eql (foreign-funcall "brlapi__write" :pointer (connection-handle obj) :pointer ws brlapi-code) 0))))


;;;; * Input

(defctype key-code :uint64)
(defmethod read-key ((obj display) &optional block)
  (with-foreign-object (key 'key-code)
    (case (foreign-funcall "brlapi__readKey" :pointer (connection-handle obj) :boolean block :pointer key brlapi-code)
      (0 nil)
      (1 (mem-ref key 'key-code)))))

(defmethod read-key-with-timeout ((obj display) &optional (timeout -1))
  (with-foreign-object (key 'key-code)
    (case (foreign-funcall "brlapi__readKeyWithTimeout" :pointer (connection-handle obj) :int (* timeout 1000) :pointer key brlapi-code)
      (0 nil)
      (1 (mem-ref key 'key-code)))))

(defcstruct expanded-key-code
  "A key code broken down into its individual fields."
  (type :int)
  (command :int)
  (argument :int)
  (flags :int)
)

(defcstruct key-code-description
  "The description of a key code."
  (type-name :string)
  (command-name :string)
  (argument-value :int)
  (flag-count :int)
  (flag-names :string :count 32)
  (field-values expanded-key-code)
)

(defun expand-key-code (code)
  "Return the individual fields of a key code as a list of numeric values.
The list contains, in order, the type, command, argument, and flags."

  (with-foreign-object (expansion 'expanded-key-code)
    (foreign-funcall "brlapi_expandKeyCode" key-code code :pointer expansion :int)
    (with-foreign-slots
      ((type command argument flags) expansion expanded-key-code)
      (list type command argument flags)
    )
  )
)

(defun describe-key-code (code)
  "Return the individual fields of the key code as a list.
The list contains:
  0: type (a string)
  1: command (a string)
  2: argument (a non-negative integer)
  3: flags (a list of strings)"

  (with-foreign-object (description 'key-code-description)
    (foreign-funcall "brlapi_describeKeyCode" key-code code :pointer description :int)
    (with-foreign-slots
      ((type-name command-name argument-value flag-count flag-names) description key-code-description)

      (list 
        type-name command-name argument-value
        (loop
          for index below flag-count
          collect (mem-aref (foreign-slot-pointer description 'key-code-description 'flag-names) :string index))
      )
    )
  )
)

;;;; * Example usage

(defun example (&optional (tty -1))
  "A basic example."
  (let ((display (open-connection)))
    (enter-tty-mode display tty)
    (write-text display "Press any key to continue...")
    (apply #'format t "; Command: ~A, argument: ~D, flags: ~D"
           (multiple-value-list (expand-key-code (read-key display t))))
    (leave-tty-mode display)
    (close-connection display)))

