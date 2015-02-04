The String Operand
------------------

A *string* operand may be specified as a non-whitespace sequence of:

*  Any single character other than a backslash (\\\\) or a white-space
   character.

*  A backslash-prefixed special character. These are:

   ===============  ==========================================================
   Sequence         Meaning
   ---------------  ----------------------------------------------------------
   ``\b``           The backspace character.
   ``\f``           The formfeed character.
   ``\n``           The newline character.
   ``\o###``        The three-digit octal representation of a character.
   ``\r``           The carriage return character.
   ``\s``           The space character.
   ``\t``           The horizontal tab character.
   ``\u####``       The four-digit hexadecimal representation of a character.
   ``\U########``   The eight-digit hexadecimal representation of a character.
   ``\v``           The vertical tab character.
   ``\x##``         The two-digit hexadecimal representation of a character.
   ``\X##``         (the case of the X and of the digits isn't significant)
   ``\<name>``      The Unicode name of a character (use _ for space).
   ``\{variable}``  The value of a variable.
   ``\\``           A literal backslash.
   ``\#``           A literal number sign.
   ===============  ==========================================================

