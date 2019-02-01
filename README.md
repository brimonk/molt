# Implementation of Method of Lines Transpose (MOLT)

This project aims to create a proper implementation of MOLT, researched by
Matthew Causley, Andrew Christlieb, et al.

## Installation

Installation requires a Linux machine with GCC and GNU Make installed.

Reason for this is that the program heavily relies on memory-mapped files by way
of mmap, and the Makefile I've written generates Makefile rules from each C
source file. GCC automates this process, and effectively, will rebuild the
portions of a program that need it when a header file changes.

To install, run the following:

```
git clone https://github.com/brimonk/molt.git
cd molt/
make
```

And you'll be left with a binary called, `molt` in that directory.

## Usage

_Usage Details Will Go Here When The Program Gets Close to Completion_

## Implementation Details

To allow the program to have a chance at being able to simulate interesting
things, the program attempts to `mmap` some space on disk, as working space for
the simulation. This data is expected to be filled with binary content, and for
helper/visualization programs, it is expected this file has Little Endian data
within.

*Warning*: Not a lot of thought went into how to make this portable.
