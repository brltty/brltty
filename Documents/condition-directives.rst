The EndIf Directive
-------------------

.. parsed-literal:: endIf

Use this directive to terminate the current conditional nesting level.

Examples::

   ifVar x
      These lines will be processed if a variable named x exists.
   endIf

The Else Directive
------------------

.. parsed-literal:: else

Use this directive to negate the test associated with the current conditional
nesting level.

Examples::

   assign x some\svalue
   ifVar x
      These lines will be processed.
   else
      These lines won't be processed.
   endIf

