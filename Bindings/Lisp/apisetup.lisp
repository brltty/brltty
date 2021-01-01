;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; libbrlapi - A library providing access to braille terminals for applications.
;
; Copyright (C) 2006-2021 by Mario Lang <mlang@delysid.org>
;
; libbrlapi comes with ABSOLUTELY NO WARRANTY.
;
; This is free software, placed under the terms of the
; GNU Lesser General Public License, as published by the Free Software
; Foundation; either version 2.1 of the License, or (at your option) any
; later version. Please see the file LICENSE-LGPL for details.
;
; Web Page: http://brltty.app/
;
; This software is maintained by Dave Mielke <dave@mielke.cc>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(require 'asdf)
(require 'sb-posix)

(defun current-directory ()
  "The current working directory as a directory path."
  (concatenate 'string (sb-posix:getcwd) "/")
)

(defun add-package-directory (path)
  "Add a directory from which packages can be loaded."
  (pushnew path asdf:*central-registry* :test #'equal)
)

(defun load-package (name)
  "Load a package."
  (asdf:operate 'asdf:load-op name)
)

(add-package-directory (current-directory))
(load-package 'brlapi)

