#!../../bin/windows-x64-static/Microtest

#- You may have to change Microtest to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/Microtest.dbd"
Microtest_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate("iocBoot/${IOC}/Microtest.substitutions")

## Configure port driver
# MicrotestConfig(portName,        # The name to give to this asyn port driver
MicrotestConfig("Microtest")

#asynSetTraceMask Microtest -1 255

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=ovs"
