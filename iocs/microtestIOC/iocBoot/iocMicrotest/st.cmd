#!../../bin/windows-x64-static/microtest

#- You may have to change Microtest to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/microtest.dbd"
microtest_registerRecordDeviceDriver pdbbase

# The port name for the device
epicsEnvSet("PORT", "Microtest")

###
# MicrotestConfig(
#    portName (will be created),
#    dllPath (path where DebenMT64.dll is located),
###
MicrotestConfig("$(PORT)", "C:\Program Files (x86)\Deben UK Ltd\Microtest DLL\DebenMT64.dll")

## Load record instances
dbLoadRecords("$(MICROTEST)/MicrotestApp/Db/Microtest.template", "P=Deben, PORT=$(PORT)")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=ovs"
