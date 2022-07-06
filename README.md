# Deben Microtest
EPICS Support for Deben Microtest tensile/compression stages.

A simple Asyn-Driver to control the Deben Microtest software via EPICS. Not all possible commands are implemented. The driver uses the “DebenMT64.dll”, which is a 64 bit DLL that allows access to the Microtest control software. So this driver needs the original vendor software. Tested on Windows 7 (64bit) and Windows 10 (64bit) with "Deben Microtest materials testing system control software V6.2.70".

In order to compile it, you need to put the "MTDDE.h" header-file into the .../MicrotestApp/src folder. Also you have to locate the “DebenMT64.dll” on your system and adapt the path to it in "MicrotestDriver.cpp" line ~104. 

Usage: setup an experiment using the vendor software (Module Setup, Offset settings etc.). Once you can control your experiment with the vendor software, start the IOC and send/read commands via EPICS. Because the Microtest DLL has no function for reading the Mode (only a function for setting the mode), you need to set the Mode PV (either Tensile or Compression) manually right after IOC start.
