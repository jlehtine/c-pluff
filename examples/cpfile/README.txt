C-PLUFF FILE COMMAND EXAMPLE
============================

Overview
--------

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


Building
--------

The example application must be installed before it can be used. You
can build and install it by running "make all" or "make install"
in the build directory corresponding to this source directory.
Before building/installing this example, you must build/install the
C-Pluff framework implementation. All this is done automatically if you run
"make examples" or "make examples-install" in the top level build directory.


Running
-------

This example uses the generic plug-in loader, cpluff-loader, as the main
program. The executable cpfile installed into the bin directory is just
a shell script invoking the cpluff-loader.

The included plug-ins provide following features:

  org.c-pluff.examples.cpfile.core
    The core application logic and an extension point for file classifiers.

  org.c-pluff.examples.cpfile.special
    A classifier for special files (directories, symbolic links, etc).

  org.c-pluff.examples.cpfile.extension
    A classifier using file name extension to determine file type.
    This plug-in provides an extension point for registering file
    types and corresponding file name extensions.

  org.c-pluff.examples.cpfile.cext
    File types and extensions for some C program source files.

You can experiment with different configurations by adding and removing
plug-ins into cpfile/plugins directory in the library directory. The core
plug-in must be always included for the application to work as intended.


Example runs
------------

Here are couple of examples of using the resulting cpfile application.

  $ cpfile /tmp/testdir
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/testdir: directory
  
  $ cpfile /tmp/test.foo
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.foo: unknown file type
  
  $ cpfile /tmp/test.c
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.c: C source file

  $ cpfile /tmp/test.nonexisting
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  /tmp/test.nonexisting: stat failed: No such file or directory

You can make cpfile more quiet by giving it -q option, or more verbose by
giving it -v option (repeated for more verbosity up to -vvv). Actually,
these options are processed by cpluff-loader which configures logging
accordingly.

  $ cpfile -q /tmp/test.c
  /tmp/test.c: C source file
  
  $ cpfile -vv /tmp/test.c
  C-Pluff Loader, version 0.1.0
  C-Pluff Library, version 0.1.0 for i686-pc-linux-gnu
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.cext has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been installed.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core runtime has been loaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core is starting.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension runtime has been loaded.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension is starting.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.extension has been started.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.special runtime has been loaded.
  C-Pluff: INFO: [org.c-pluff.examples.cpfile.core] Plug-in org.c-pluff.examples.cpfile.special has been started.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been started.
  /tmp/test.c: C source file
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core is stopping.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been stopped.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.core has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.extension has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.cext has been uninstalled.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special runtime has been unloaded.
  C-Pluff: INFO: [loader] Plug-in org.c-pluff.examples.cpfile.special has been uninstalled.
