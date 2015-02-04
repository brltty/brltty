The Assign Directive
--------------------

::

   assign *variable* *value*

Use this directive to create or update a variable associated with the current
include level. The variable is visible to the current and to lower include
levels, but not to higher include levels.

*variable*
   The name of the variable. If the variable doesn't already exist at the
   current include level then it is created.

*value*
   The value which is to be assigned to the variable. If it's not supplied then
   a zero-length (null) value is assigned.

Examples::

   assign nullValue
   assign shortValue word
   assign longValue a\svalue\swith\sspaces
   assign IndirectValue \{variableName}

The AssignDefault Directive
---------------------------

The IfVar Directive
-------------------

::

   ifVar *variable* *directive*

Use this directive to only process one or more directives if a variable exists.

*variable*
   The name of the variable whose existence is to be tested.

*directive*
   The directive which is to be conditionally processed. It may contain spaces.
   This operand is optional. If it isn't supplied then this directive applies
   to all subsequent lines until `The EndIf Directive`_ or
   `The Else Directive`_ that is at the same conditional nesting level.

Examples::

   ifVar var1 ifVar var2 assign concatenation \{var1}\{var2}

The IfNotVar Directive
----------------------

::

   ifNotVar *variable* *directive*

Use this directive to only process one or more directives if a variable doesn't
exist.

*variable*
   The name of the variable whose existence is to be tested.

*directive*
   The directive which is to be conditionally processed. It may contain spaces.
   This operand is optional. If it isn't supplied then this directive applies
   to all subsequent lines until `The EndIf Directive`_ or
   `The Else Directive`_ that is at the same conditional nesting level.

Examples::

   ifNotVar var1 assign var1 default\svalue

