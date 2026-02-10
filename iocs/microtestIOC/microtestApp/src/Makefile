TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = microtest
# Microtest.dbd will be created and installed
DBD += microtest.dbd

# Microtest.dbd will be made up from these files:
microtest_DBD += base.dbd

# Include dbd files from all support applications:
microtest_DBD += asyn.dbd
microtest_DBD += MicrotestSupport.dbd
microtest_DBD += motorSupport.dbd
microtest_DBD += devSoftMotor.dbd

# Add all the support libraries needed by this IOC
microtest_LIBS += asyn Microtest motor softMotor

# Microtest_registerRecordDeviceDriver.cpp derives from Microtest.dbd
microtest_SRCS += microtest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
microtest_SRCS_DEFAULT += microtestMain.cpp
microtest_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#Microtest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
microtest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
