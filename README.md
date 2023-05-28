#  Pirevision

## Introduction

This is a program to help interpret revision codes as found in /proc/cpuinfo
on Raspberry based systems. It is based on information provided by
https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-revision-codes


The revision code can be used to extract some particular feature information
about the hardware:
* Whether over voltage is allowed
* Whether programming of the OTP register is allowed
* Whether reading of the OTP register is allowed
* Whether warranty is still intact
* The type or model designation of the Raspbian product (e.g. Zero, 4B, etc.)
* The hardware release version (e.g. 1.2)
* The designation of the processor/SOC used
* The amount of physical memory installed

Some of this information, in particular the SOC code may actually differ from
what is reported in /proc/cpuinfo as the kernal does not distinsguish between
all possible SOC values. In particular, the documentation contains the
following note:
> It's just a kernel/device tree anomaly. Upstream have chosen to denote all
> the Pi's as BCM2835, whereas the original Pi specific kernels would display
> BCM2708, BCM2709, or BCM2710 as appropriate.
>
> Whether it is more correct to use 2708/9/10 or 2835/6/7 is also sometimes
> debated - the former are the silicon, the latter are the packaged chip
> (which in some non-Pi cases also included an independent RAM die within the
> same package).

These codes are typically expressed in hexadecimal notation and are either
old style, or new style. Old style is basically an index which then needs
to be used against a documented table to extract some particular feature
information. The new style is interpreted as a number of interpreted bit
fields, each of which typically is an index into a table of specific documented
values.

This program handles old style codes by mapping them onto corresponding new
style ones, and then interpreting the new style code's fields. It looks up
values in the corresponding documentation tables and prints them in a human
readable format.

The program can (optionally) output a JSON formatted interpretation, but
a text output is default. It can handle a list of supplied revision codes
in hexadecimal notation or, when none is provided, it will attempt to extract
one for the host system by looking inside "/proc/cpuinfo".

## Usage

```
Usage: pirevision [-j|--json] [revision code...]
```
 * -j flag causes JSON output instead of text
 * If no revision code(s) supplied, attempt to get it from /proc/cpuinfo and
 * use that, if succesful.
 * Otherwise process each argument as a separate revision code.
 * These must be specified as hexadecimal codes, with, or without 0x or 0X
   prefix.

## Installation

### Compilation

The program can be compiled with almost any C compiler. It has been tested using
* gcc on macos:
  `gcc -o pirevision pirevision.c`
* Xcode on on macos: Xcode project (not provided here)
* cc on a 32-bit Rasperry OS installation (bullseye):
  `cc -o pirevision pirevision.c`
  
### Installation

Move the binary resulting from compilation to the desired location.


### Configuration

There are no configuration steps.
