This code is on the bbt-canute branch of BRLTTY's source repository:

   https://github.com/brltty/brltty

Relevant files can be found at:

   https://brltty.app/archive/Canute

To install this software:

*  Download these files:

   + Any of the (.tar.*) compressed archives.
   + The canute-install script.

*  Perform these steps:

   1) Uncompress the archive (giving you the .tar file).
   2) As root, run: canute-install /path/to/the/archive
   3) Reboot the host.

The following canute-* scripts will be in the command search path.
Each has a -h (help) option.

canute-screen
   This script starts a text mode session with a terminal that
   has the same dimensions as the Canute's braille display.

canute-startx
   This script starts an X session with a terminal that has
   the same dimensions as the Canute's braille display.

canute-ttysize
   This script configures the current terminal to have
   the same dimensions as the Canute's braille display.

canute-start
   Start the Canute braille display process now.

canute-stop
   Stop the Canute braille display process now.

canute-enable
   The Canute braille display process will be started when the system boots.

canute-disable
   The Canute braille display process won't be started when the system boots.

canute-status
   Show the current status of the Canute braille display process.

canute-client
   Run a BrlAPI client within a prepared environment.
   It doesn't work for Java clients.

canute-java
   Run a Java BrlAPI client.

canute-apitest
   Perform a BrlAPI protocol test.

