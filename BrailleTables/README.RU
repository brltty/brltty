README.RU
by Hans Schou and Dave Mielke

This is the description for the russian braille table.
It is based on the koi8-r character set which to our
knowledge is the most used in Russia.

As the russian cyrillic definition conflicts with latin
definition some decitions has to be taken. Russians
needs both to type latin for the command prompt and
cyrillic while reading and writing letters and mail.

In the following latin letters are quoted with '
like in 'a', and cyrillic similar letters are in [
like in [a].

Dot 1 in the cyrillic definition is the cyrillic
letter which looks and sounds like 'a'. The problem
is that in the koi8-r character set, there is both
a latin 'a' and a cyrillic [a].
In decimal the character number is 97 and 193 respectively.

To handle this conflict we have decide and prioritirise
which characters is most important to match the standard.

RULES:
1. All cyrillic characters must follow the russian standard.
   Koi8-r character 193 [a] must have dot-1 and so on.
2. Capital cyrillic has dot 7 on.
3. Latin alphabet implemented to follow international
   standard except it has dot 8 on.
4. Capital latin has dot 78 on.
5. Numbers are defined as in the american standard.
   This is dot-2 for number '1' and so on.
   (moved one row down)
   This will conflict with the cyrillic comma which
   is also dot-2.
4. Special characters like !"#¤%&/()=?
   follows the the american standard if possible.
2. Control characters is often used on Linux. These is
   not very well implemented. More work should be done.


Changelog
2003-02-01:
Initial release. Comments from russians are very welcome
as neither Hans Schou or Dave Mielke understands a
single word in russian.

