The Assign Directive
--------------------

.. parsed-literal:: assign *variable* *value*

Use this directive to create or update a variable associated with the current
include level. The variable is visible to the current and to lower include
levels, but not to higher include levels.

*variable*
   The name of the variable. If the variable doesn't already exist at the
   current include level then it is created.

*value*
   The value that is to be assigned to the variable. If it's not supplied then
   a zero-length (null) value is assigned.

Examples::

   assign nullValue
   assign shortValue word
   assign longValue a\svalue\swith\sspaces
   assign IndirectValue \{variableName}

The AssignDefault Directive
---------------------------

.. parsed-literal:: assignDefault *variable* *value*

Use this directive to assign a default value to a variable associated with the
current include level. It's functionally equivalent to:

.. parsed-literal:: ifNotVar *variable* assign *variable* *value*

See `The Assign Directive`_ and `The IfNotVar Directive`_ for more details.

*variable*
   The name of the variable. If the variable doesn't already exist at the
   current include level then it is created. If it does already exist then it
   is **not** modified.

*value*
   The value that is to be assigned to the variable if it doesn't already
   exist. If it's not supplied then a zero-length (null) value is assigned.

Examples::

   assignDefault format plain\stext

The IfVar Directive
-------------------

.. parsed-literal:: ifVar *variable* *directive*

Use this directive to only process one or more directives if a variable exists.

*variable*
   The name of the variable whose existence is to be tested.

*directive*
   The directive that is to be conditionally processed. It may contain spaces.
   This operand is optional. If it isn't supplied then this directive applies
   to all subsequent lines until `The EndIf Directive`_ or
   `The Else Directive`_ that is at the same conditional nesting level.

Examples::

   ifVar var1 ifVar var2 assign concatenation \{var1}\{var2}

The IfNotVar Directive
----------------------

.. parsed-literal:: ifNotVar *variable* *directive*

Use this directive to only process one or more directives if a variable doesn't
exist.

*variable*
   The name of the variable whose existence is to be tested.

*directive*
   The directive that is to be conditionally processed. It may contain spaces.
   This operand is optional. If it isn't supplied then this directive applies
   to all subsequent lines until `The EndIf Directive`_ or
   `The Else Directive`_ that is at the same conditional nesting level.

Examples::

   ifNotVar var1 assign var1 default\svalue

