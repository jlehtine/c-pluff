C-PLUFF FILE COMMAND EXAMPLE
============================

On Linux, the file(1) utility can be used to determine file type and to
get information about contents of a file. Here are couple of examples of
file usage.

  $ file /sbin/init
  /sbin/init: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV),
  for GNU/Linux 2.4.1, dynamically linked (uses shared libs), for
  GNU/Linux 2.4.1, stripped
  
  $ file COPYRIGHT.txt
  COPYRIGHT.txt: ASCII English text

This example shows how a minimalistic file clone could be implemented as an
extensible application based on C-Pluff. We will call the resulting utility
cpfile. It can recognize some special files and some file types based on
file extension. But it could be further extended to recognize files based
on their content by deploying a suitable plug-in. But notice that the focus
here was on creating a straightforward example rather than an efficient one.

Before building/installing this example, you must build/install the
C-Pluff framework implementation. This is done automatically if you run
"make examples" or "make examples-install" in the top level build directory.

This example uses the generic plug-in loader, cpluff-loader, as the main
program. The executable cpfile installed into the bin directory is just
a shell script invoking the cpluff-loader.
