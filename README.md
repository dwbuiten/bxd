BXD: Binary Hex Diff
====================

Copyright (c) 2016 Derek Buitenhuis <derek.buitenhuis at gmail dot com>

![BXD](http://chromashift.org/bxd.png)

BXD is a small TUI tool to compare two arbitrarily sized files. It supports
diffing byte insetions, deletions, and changes. It uses block-based diff, so
comparing large files should be very fast.

It was written so that some thing similar to Beyond Compare could be used
via SSH, to fit the author's use case and needs. As such, the code may
be a bit messy, but should mostly be OK.

Features
--------

* Diff arbitrarily differently sized files.
* Block-based diff and display for diffing large files with relatively low
  memory and CPU usage.
* Memory mapped input.
* Seek-to-next-difference.

Usage
-----

Currently, the command like arguments are very simple:

    Binary Hex Diff
    Copyright (c) 2016 Derek Buitenhuis.

    A tool to compare two arbitrarily sized binary files.

    Usage: bxd file1 file2

Current Limitations
-------------------

This tool started life out of need for a quick tool for a hobby project,
and as such, lacks a few features that would be very nice to have.

This list will be updated as limitations are removed.

* Scrollback is limited to the current block and the previous block. This
  will go away in the future, optionally, for a memory usage and speed
  tradeoff.
* The seek-to-next-difference feature currently only seeks to the block which
  contains the next difference, even if it isn't necessarily displayed on
  the screen.
* There is no seek-to-offset featrue or dialogue yet.
* There may be UI text overlap when using very small terminal sizes.

Building
--------

To build BXD, you need a C99 compiler, a POSIX system, and a copy of
[termbox](https://github.com/nsf/termbox/) in your compiler's include
and library path. Then simply type `make`.

Bugs & Patches
--------------

Please file any bugs or problems on the GitHub Issues page, and send
any new code as a Pull Request. Please try to follow a similar coding
style to the exisiting codebase.
