# Deben Microtest
EPICS Support for Deben Microtest tensile/compression stages.

A simple Asyn-Driver to control the Deben Microtest software via EPICS. Not all possible commands are implemented.

The driver uses the “DebenMT64.dll”, which is a 64 bit DLL that allows access to the Microtest control software. So it needs the vendor software.
In order to compile it, one needs to put the "MTDDE.h" header-file into the .../MicrotestApp/src folder. Also you have to locate the “DebenMT64.dll”
on your system and adapt the path to it in "MicrotestDriver.cpp" line ~104. 

Usage: setup an experiment using the vendor software (Module Setup, Offset settings etc.). Once you can control your experiment with the vendor software,
start the IOC and send/read commands via EPICS.
