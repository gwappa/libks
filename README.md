# libks

`libks` is a C++03-based simple OS-independent layer for:

+ threading (threads, mutex, condition variables)
+ logging (with different levels such as INFO, WARNING, ERROR)
+ timing (nanosecond-precision clocks)

## current status

the version is at `0.1.0a1`, meaning:

+ thread priority may be better controllable
+ tests remain to be implemented
+ easy installation process remains to be provided

## building

on the mac, you can use `make static` or `make dynamic` in the
top-level directory.

The `make static` comamnd should also work (untested) on
other UNIX environments as well.

For Windows, I have to make something...

## using

In addition to the library, you have to use `-lpthread` (on \*nix)
or `WinSock` (on Windows) upon linkage.

## license

the MIT license

