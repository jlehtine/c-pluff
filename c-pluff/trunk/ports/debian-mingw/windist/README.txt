C-PLUFF WINDOWS DISTRIBUTION BY CROSS-COMPILATION
=================================================

This directory includes files needed to create a Windows distribution of
C-Pluff and its dependencies on Debian GNU/Linux using installed MinGW
cross-compiled binaries.

To create a Windows distribution on Debian GNU/Linux, follow these steps.


Install MinGW cross-compiled binaries
-------------------------------------

Install the following packages.

  - cpluff-mingw, depends on following packages
    - expat-mingw
    - libltdl-mingw
    - gettext-runtime-mingw

Precompiled binaries are available by pointing APT to

  deb http://www.c-pluff.org/downloads/debian stable cpluff 3rdparty

Source packages are available using the corresponding deb-src line.


Build Windows distribution
--------------------------

Execute the script makedist located in this directory.

  ./makedist

This will create the following files:

  cpluff-VER-only.zip
  cpluff-VER-deps.zip
  cpluff-VER.zip

Where VER is the used version of C-Pluff.

The first archive, cpluff-VER-only.zip, includes only the endemic C-Pluff
components and the second archive, cpluff-VER-deps.zip, includes the
required third party dependencies. The third archive, cpluff-VER.zip,
is a union of the two other archives.
