TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += Microtest

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install Microtest.dbd into <top>/dbd
DBD += Microtest.dbd

# specify all source files to be compiled and added to the library
Microtest_SRCS += MicrotestDriver.cpp

Microtest_LIBS += asyn

Microtest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

