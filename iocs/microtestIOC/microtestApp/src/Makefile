TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = Microtest
# Microtest.dbd will be created and installed
DBD += Microtest.dbd

# Microtest.dbd will be made up from these files:
Microtest_DBD += base.dbd

# Include dbd files from all support applications:
Microtest_DBD += asyn.dbd
Microtest_DBD += Microtest.dbd
Microtest_DBD += motorSupport.dbd
Microtest_DBD += devSoftMotor.dbd

# Add all the support libraries needed by this IOC
Microtest_LIBS += asyn Microtest motor softMotor

# Microtest_registerRecordDeviceDriver.cpp derives from Microtest.dbd
Microtest_SRCS += Microtest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
Microtest_SRCS_DEFAULT += microtestMain.cpp
Microtest_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#Microtest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
Microtest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
