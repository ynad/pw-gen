Pw-Gen - Sequences generator
============================

Version:
--------
1.2.7 (2014-05-28)
See ChangeLog for details. 

    TODO:
    - proper arg checks
    - verbose/debug mode
    - use of wchar.h
    - status save and restart (full process exit)

    BUGS:
    - args check


Description:
------------
Sequences generator: recursive generation of all alphanumeric sequences, to use with brute-force tools.


Installation:
-------------
Linux:
  System requirements: build-essential ('make' and 'gcc').

  - Compile:
      `make`

  - Compile and install:
      `make install`

  - Compile and install in DEBUG mode:
      `make debug`

  - Remove only objects:
      `make clean`

  - Uninstall:
      `make uninstall`

Windows:
  Compile sources under "src\" with any software you prefer, then run from Windows CMD.
  It's mandatory to compile with the define "_FILE_OFFSET_BITS=64" in compiler settings!


License:
--------
GPLv2, see LICENSE.md


Author:
-------
ynad (github.com/ynad/pw-gen)

