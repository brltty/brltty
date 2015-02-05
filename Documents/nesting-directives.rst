The Include Directive
---------------------

.. parsed-literal:: include *file* # *comment*

Use this directive to include the content of another file. It is recursive,
which means that an included file can itself include yet another file.
Care must be taken to ensure that an "include loop" is not created.

*file*
   The file to be included. It may be either a relative or an absolute path. If
   relative, it is anchored at the directory containing the including file.

