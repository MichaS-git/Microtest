#include "MTDDE.h" // Include the Microtest header file
#include <windows.h>
#include <iostream>
#include <conio.h>

#include <iocsh.h>
#include <epicsExport.h>
#include <asynPortDriver.h>


static const char *driverName = "Microtest";

#define Mode			    "Mode"
#define StopMotor			"StopMotor"
#define MovingDone			"MovingDone"
#define Load			    "Load"
#define Force			    "Force_RBV"
#define Extension		    "Extension"
#define ExtensionRBV		"Extension_RBV"
#define StartPosition	    "StartPosition"
#define AbsolutePosition    "AbsolutePosition"
#define MotorSpeed			"MotorSpeed"
#define MotorSpeedRBV		"MotorSpeed_RBV"

/* Class definition for the Microtest */
class Microtest : public asynPortDriver
{
public:
    Microtest(const char *portName);

    /* These are the methods that we override from asynPortDriver */
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);

protected:

    int Mode_;
    int MovingDone_;
    int Load_;
    int Force_;
    int Extension_;
    int ExtensionRBV_;
    int StartPosition_;
    int AbsolutePosition_;
    int StopMotor_;
    int MotorSpeed_;
    int MotorSpeedRBV_;

private:
    HINSTANCE hLib;

    void loadLibrary();
};

/** Constructor for the Microtest class
  */
Microtest::Microtest(const char *portName)
    : asynPortDriver(portName, 1, 1,
                     asynInt32Mask | asynFloat64Mask | asynDrvUserMask,  // Interfaces that we implement
                     asynInt32Mask | asynFloat64Mask,    // Interfaces that do callbacks
                     ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
                     // this is how to start without ASYN_CANBLOCK
                     //ASYN_MULTIDEVICE, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
                     0, 0) /* Default priority and stack size */
{
    createParam(Mode,			    asynParamInt32,		&Mode_);
    createParam(StopMotor,			asynParamInt32,		&StopMotor_);
    createParam(MotorSpeed,			asynParamInt32,		&MotorSpeed_);
    createParam(MotorSpeedRBV,		asynParamInt32,		&MotorSpeedRBV_);
    createParam(MovingDone,		    asynParamInt32,	    &MovingDone_);
    createParam(Load,			    asynParamFloat64,	&Load_);
    createParam(Force,			    asynParamFloat64,	&Force_);
    createParam(Extension,		    asynParamFloat64,	&Extension_);
    createParam(ExtensionRBV,		asynParamFloat64,	&ExtensionRBV_);
    createParam(StartPosition,	    asynParamFloat64,	&StartPosition_);
    createParam(AbsolutePosition,	asynParamFloat64,	&AbsolutePosition_);

    loadLibrary();

	// Get the version information for the DLL
	int vermaj, vermin;// speedIndex;
	MT_Version(&vermaj, &vermin);
	std::cout << "Microtest DLL";
	std::cout << "  Version " << vermaj << "." << vermin << std::endl;

	// set the proper motor speed at start
    MT_Connect();
	setIntegerParam(MotorSpeed_, MT_GetMotorSpeedIndex());

    // Free the Library, otherwise it will be blocked
    FreeLibrary((HMODULE)hLib);
}

void Microtest::loadLibrary()
{
    // we need to execute this function before every call to the Microtest (maybe there is a better way to code it ...)

    // Load the MicrotestMT64.dll DLL
    hLib=LoadLibraryW(L"C:\\Program Files (x86)\\Deben UK Ltd\\Microtest DLL\\DebenMT64.dll");
    if (hLib==NULL)
    {
        std::cout << "Unable to load library!" << std::endl;
    }

    // Get the functions from the DLL
    MT_Connect 				= (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_Connect");
    MT_Disconnect			= (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_Disconnect");
    MT_Version 				= (DLL_MT_Version   )GetProcAddress((HMODULE)hLib, "MT_Version");
    MT_IsMotorRunning       = (DLL_MT_Bool      )GetProcAddress((HMODULE)hLib, "MT_IsMotorRunning");
    MT_StartMotor           = (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_StartMotor");
    MT_StopMotor            = (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_StopMotor");
    MT_GetForce				= (DLL_MT_Double    )GetProcAddress((HMODULE)hLib, "MT_GetForce");
    MT_GetPosition          = (DLL_MT_Double    )GetProcAddress((HMODULE)hLib, "MT_GetPosition");
    MT_GetExtension         = (DLL_MT_Double    )GetProcAddress((HMODULE)hLib, "MT_GetExtension");
    MT_GetMotorSpeedIndex   = (DLL_MT_Integer   )GetProcAddress((HMODULE)hLib, "MT_GetMotorSpeedIndex");
    MT_GotoLoad             = (DLL_MT_SetDouble )GetProcAddress((HMODULE)hLib, "MT_GotoLoad");
    MT_GotoAbsolutePosition = (DLL_MT_SetDouble )GetProcAddress((HMODULE)hLib, "MT_GotoAbsolutePosition");
    MT_GotoExtension        = (DLL_MT_SetDouble )GetProcAddress((HMODULE)hLib, "MT_GotoExtension");
    MT_ModeTensile          = (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_ModeTensile");
    MT_ModeCompressive      = (DLL_MT_NoParams  )GetProcAddress((HMODULE)hLib, "MT_ModeCompressive");
    MT_SetMotorSpeed        = (DLL_MT_SetInteger)GetProcAddress((HMODULE)hLib, "MT_SetMotorSpeed");
}

asynStatus Microtest::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    loadLibrary();
    MT_Connect();

    int addr;
    int function = pasynUser->reason;
    int status=0;
    int intVal;
    //static const char *functionName = "readInt32";

    this->getAddress(pasynUser, &addr);

    // Analog input function
    if (function == MotorSpeedRBV_)
    {
        // read the motor speed index at start and set the corresponding record
        intVal = MT_GetMotorSpeedIndex();
        *value = intVal;
        setIntegerParam(addr, MotorSpeedRBV_, *value);
    }
    if (function == MovingDone_)
    {
        intVal = MT_IsMotorRunning();
        *value = intVal;
        if (*value)
        {
            *value = 0;
            setIntegerParam(addr, MovingDone_, *value);
        }
        else
        {
            *value = 1;
            setIntegerParam(addr, MovingDone_, *value);
        }
    }

    // Other functions we call the base class method
    else
    {
        status = asynPortDriver::readInt32(pasynUser, value);
    }

    callParamCallbacks(addr);

    // Free the Library, otherwise it will be blocked
    FreeLibrary((HMODULE)hLib);

    return (status==0) ? asynSuccess : asynError;
}

asynStatus Microtest::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    loadLibrary();
    MT_Connect();

    int addr;
    int function = pasynUser->reason;
    int status = 0;
    static const char *functionName = "writeInt32";

    this->getAddress(pasynUser, &addr);
    setIntegerParam(addr, function, value);

    if (function == Mode_)
    {
        if (value)
        {
            MT_ModeCompressive();
        }
        else
        {
            MT_ModeTensile();
        }
    }
    if (function == StopMotor_)
    {
        MT_StopMotor();
    }
    if (function == MotorSpeed_)
    {
        MT_SetMotorSpeed(value);
    }

    callParamCallbacks(addr);

    // Free the Library, otherwise it will be blocked
    FreeLibrary((HMODULE)hLib);

    if (status == 0)
    {
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s, port %s, wrote %d to address %d\n",
                  driverName, functionName, this->portName, value, addr);
    }
    else
    {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s, port %s, ERROR writing %d to address %d, status=%d\n",
                  driverName, functionName, this->portName, value, addr, status);
    }
    return (status==0) ? asynSuccess : asynError;
}

asynStatus Microtest::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    loadLibrary();
    MT_Connect();

    int addr;
    int function = pasynUser->reason;
    int status = 0;
    double doubleVal = 0;
    //static const char *functionName = "readFloat64";

    this->getAddress(pasynUser, &addr);

    // Analog input function
    if (function == Force_)
    {
        doubleVal = MT_GetForce();
        //std::cout << "Force " << doubleVal << "N" << std::endl;
        *value = doubleVal;
        setDoubleParam(addr, Force_, *value);
    }
    if (function == ExtensionRBV_)
    {
        doubleVal = MT_GetExtension();
        *value = doubleVal;
        setDoubleParam(addr, ExtensionRBV_, *value);
    }
    if (function == StartPosition_)
    {
        doubleVal = MT_GetPosition();
        *value = doubleVal;
        setDoubleParam(addr, StartPosition_, *value);
    }

    // Other functions we call the base class method
    else
    {
        status = asynPortDriver::readFloat64(pasynUser, value);
    }

    callParamCallbacks(addr);

    // Free the Library, otherwise it will be blocked
    FreeLibrary((HMODULE)hLib);

    return (status==0) ? asynSuccess : asynError;
}

asynStatus Microtest::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    loadLibrary();
    MT_Connect();

    int addr;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64";

    getAddress(pasynUser, &addr);

    // Set the parameter in the parameter library.
    status = setDoubleParam(addr, function, value);

    if (function == AbsolutePosition_)
    {
        //std::cout << "Param " << value << std::endl;
        MT_GotoAbsolutePosition(value);
    }
    if (function == Load_)
    {
        MT_GotoLoad(value);
    }
    if (function == Extension_)
    {
        MT_GotoExtension(value);
    }

    // Do callbacks so higher layers see any changes
    status = callParamCallbacks();
    // Free the Library, otherwise it will be blocked
    FreeLibrary((HMODULE)hLib);

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s, status=%d function=%d, value=%f\n",
                  driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s: function=%d, value=%f\n",
                  driverName, functionName, function, value);

    return (status==0) ? asynSuccess : asynError;
}


/** Configuration command, called directly or from iocsh */
extern "C" int MicrotestConfig(const char *portName)
{
    Microtest *pMicrotest = new Microtest(portName);
    pMicrotest = NULL;  /* This is just to avoid compiler warnings */
    return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg * const configArgs[] = {&configArg0};
static const iocshFuncDef configFuncDef = {"MicrotestConfig", 1, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
    MicrotestConfig(args[0].sval);
}

void drvMicrotestRegister(void)
{
    iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
    epicsExportRegistrar(drvMicrotestRegister);
}
