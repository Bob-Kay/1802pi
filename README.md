# 1802pi

## Purpose
This project uses a Raspberry Pi to emulate the ROM, RAM and peripherals thast may be connected 
to a COSMAC 1802 processor chip.  It can be used for very low level analysis of what happens
while running an 1802 program.

This is a sister project of Bob Kucxewski's 1802/Raspberry Pi project.
see https://github.com/BobKuczewski/Run1802.git  Discussion of these projects, and just about
anything related to the RCA Cosmac 1802 processor family, can be found att
https://groups.io/g/cosmacelf

## Hardware
All that's needed is an 1802 chip, a Raspberry Pi, and a breadboard/circuit board with the
interconnections.  Power for the 1802 is supplied by the Pi.  Since the 1802 has a very low
current drain, this should not adversely affect the oftentimes barely adequate power supply
typically used with a Raspberry Pi.

## Invocation
From the CLI command line, issue this command:

1802pi [options] [code]

where options are:

    -b <binaryfile>    load binary file
    -d                 Dump 256 bytes of 1802 memory at end of program
    -f <file>          load file, see format below
    -h <hexfile>       load Intel hex format file
    -H help            Print help message and exit
    -s speed           set clock speed, default to 500Hz if not specified.

"code" is an optional arbitrary string of ASCII hex characters representing 1802 instructions 
and/or data.

The program maintains an internal "address load counter" which is updated as each byte is loaded 
into the simulated memory.  Files will be loaded in consecutive memory locations starting from 
zero if no address imformation is contained within the files.

The format of files loaded with the -f option is a string of ASCII hex characters, for example:
7B7A3000
Case is not significant, "7b7a3000" would work as well.  If a characet is a colon ":" the 
following four characters are considered a 16-bit address representing the memory adress where 
the following bytes are written.  If no address is specified, the current value of the load 
address counter is used.

