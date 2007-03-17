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

This example shows how a minimalistic file clone could be implemented as
extensible application based on C-Pluff. We will call the resulting utility
cpfile. Notice that this is intentionally a very straightforward
implementation. It could be made more efficient and elegant but the focus
here is simplicity.

Before building/installing this example, you must build/install the
C-Pluff framework implementation. This is done automatically if you run
"make examples" or "make examples-install" in the top level build directory.
