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
   Start the Canute braille display now.

canute-stop
   Stop the Canute braille display now.

canute-enable
   The Canute braille display will be started when the system boots.

canute-disable
   The Canute braille display won't be started when the system boots.

canute-status
   Show the current status of the Canute braille display process.

