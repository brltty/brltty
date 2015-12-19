The Assign Directive
--------------------

.. parsed-literal:: assign *variable* *value*

Use this directive to create or update a variable associated with
the current nesting level (see `The BeginVariables Directive`_)
of the current include level (see `The Include Directive`_).
The variable is visible to the current and to lower include levels,
but not to higher include levels.

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

Use this directive to assign a default value to a variable associated with
the current nesting level (see `The BeginVariables Directive`_)
of the current include level (see `The Include Directive`_).
It's functionally equivalent to:

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

The BeginVariables Directive
----------------------------

.. parsed-literal:: beginVariables

Use this directive to open a new variable nesting level.
`The Assign Directive`_) will define variables at this new nesting level,
and will hide variables with the same names in any previous nesting level.
These variables will remain defined until `The EndVariables Directive`_
that is at the same variable nesting level.

Examples::

   assign x 1
   # \{x} evaluates to 1
   beginVariables
   # \{x} still evaluates to 1
   assign x 2
   # \{x} now evaluates to 2
   endVariables
   # \{x} evaluates to 1 again

The EndVariables Directive
--------------------------

.. parsed-literal:: endVariables

Use this directive to close the current variable nesting level.
See `The BeginVariables Directive`_ for details.

The ListVariables Directive
---------------------------

.. parsed-literal:: listVariables

Use this directive to list all of the currently defiined variables.
It can be helpful when debugging.

