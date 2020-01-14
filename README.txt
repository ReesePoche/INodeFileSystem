Please Read filesystem_design.pdf to fully understand the design of the following file system.


Fs.C uses the framework provided by softwaredisk.c to create an INode 
based filesystem to save any series of bytes.

The code assumes that the size of unsigned long and int is 8 bytes.
If your system has a different sized ints and longs, fs will not work

