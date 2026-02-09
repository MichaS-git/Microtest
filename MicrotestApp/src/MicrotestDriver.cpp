/*
 * All DLL access is handled in the poller thread to guarantee
 * single-threaded interaction with the vendor library.
 *
 * EPICS record callbacks may be executed from multiple threads.
 * To avoid unsafe concurrent DLL calls, write callbacks only set
 * command flags and parameters, while the poller performs all
 * hardware communication.
 */

#include "MTDDE.h" // Include the Microtest header file
#include <windows.h>
#include <iostream>

#include <iocsh.h>
#include <epicsExport.h>
#include <asynPortDriver.h>

static const char *driverName = "Microtest";

#define Mode                    "Mode"
#define StopMotor               "StopMotor"
#define MovingDone              "MovingDone"
#define Load                    "Load"
#define Force                   "Force_RBV"
#define Extension               "Extension"
#define ExtensionRBV            "Extension_RBV"
#define StartPosition           "StartPosition"
#define AbsolutePosition        "AbsolutePosition"
#define MotorSpeed              "MotorSpeed"
#define MotorSpeedRBV           "MotorSpeed_RBV"
#define UseForceStability       "UseForceStability"
#define ForceDeltaPercent       "ForceDeltaPercent"
#define ForceDeltaPercentRBV    "ForceDeltaPercent_RBV"
#define ForceStableTime         "ForceStableTime"
#define ForceStableTimeRBV      "ForceStableTime_RBV"
#define ForceThreshold          "ForceThreshold"

/* Class definition for the Microtest */
class Microtest : public asynPortDriver
{
public:
    Microtest(const char *portName, const char *dllPath);
    virtual ~Microtest();

    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

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
    int UseForceStability_;
    int ForceDeltaPercent_;
    int ForceDeltaPercentRBV_;
    int ForceStableTime_;
    int ForceStableTimeRBV_;
    int ForceThreshold_;

private:
    epicsThreadId pollerThreadId;
    bool pollerRunning;
    HINSTANCE hLib;
    epicsMutexId dllLock;
    std::string dllPath_;

    /* ===== Funktionspointer aus der Microtest-DLL ===== */
    DLL_MT_NoParams   MT_Connect;
    DLL_MT_NoParams   MT_Disconnect;
    DLL_MT_Version    MT_Version;
    DLL_MT_Bool       MT_IsMotorRunning;
    //DLL_MT_NoParams   MT_StartMotor;
    DLL_MT_NoParams   MT_StopMotor;

    DLL_MT_Double     MT_GetForce;
    DLL_MT_Double     MT_GetPosition;
    DLL_MT_Double     MT_GetExtension;

    DLL_MT_Integer    MT_GetMotorSpeedIndex;

    DLL_MT_SetDouble  MT_GotoLoad;
    DLL_MT_SetDouble  MT_GotoAbsolutePosition;
    DLL_MT_SetDouble  MT_GotoExtension;

    DLL_MT_NoParams   MT_ModeTensile;
    DLL_MT_NoParams   MT_ModeCompressive;
    DLL_MT_SetInteger MT_SetMotorSpeed;

    /* ===== Steuerflags für Poller ===== */
    volatile bool doSetMode = false;
    int setModeValue = 0;

    volatile bool doSetMotorSpeed = false;
    int setMotorSpeedValue = 0;

    volatile bool doStopMotor = false;

    volatile bool doGotoAbsolutePosition = false;
    double gotoAbsolutePositionValue = 0.0;

    volatile bool doGotoLoad = false;
    double gotoLoadValue = 0.0;

    volatile bool doGotoExtension = false;
    double gotoExtensionValue = 0.0;

    /* Poller */
    static void pollerTaskC(void *drvPvt);
    void pollerTask();

    /* Initialisierung */
    bool initDLL();
};

/** Constructor for the Microtest class
  */
Microtest::Microtest(const char *portName, const char *dllPath)
    : asynPortDriver(portName, 1, 1,
                     asynInt32Mask | asynFloat64Mask | asynDrvUserMask,  // Interfaces that we implement
                     asynInt32Mask | asynFloat64Mask,    // Interfaces that do callbacks
                     ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
                     0, 0), /* Default priority and stack size */
    hLib(NULL),
    dllPath_(dllPath)
{
    createParam(Mode,			    asynParamInt32,		&Mode_);
    createParam(StopMotor,			asynParamInt32,		&StopMotor_);
    createParam(MotorSpeed,			asynParamInt32,		&MotorSpeed_);
    createParam(MotorSpeedRBV,		asynParamInt32,		&MotorSpeedRBV_);
    createParam(MovingDone,		    asynParamInt32,	    &MovingDone_);
    createParam(UseForceStability,	asynParamInt32,		&UseForceStability_);
    createParam(Load,			    asynParamFloat64,	&Load_);
    createParam(Force,			    asynParamFloat64,	&Force_);
    createParam(Extension,		    asynParamFloat64,	&Extension_);
    createParam(ExtensionRBV,		asynParamFloat64,	&ExtensionRBV_);
    createParam(StartPosition,	    asynParamFloat64,	&StartPosition_);
    createParam(AbsolutePosition,	asynParamFloat64,	&AbsolutePosition_);
    createParam(ForceDeltaPercent,	asynParamFloat64,	&ForceDeltaPercent_);
    createParam(ForceDeltaPercentRBV,   asynParamFloat64,	&ForceDeltaPercentRBV_);
    createParam(ForceStableTime,	asynParamFloat64,	&ForceStableTime_);
    createParam(ForceStableTimeRBV,	asynParamFloat64,	&ForceStableTimeRBV_);
    createParam(ForceThreshold,	    asynParamFloat64,	&ForceThreshold_);

    dllLock = epicsMutexCreate();

    /* start poller-thread*/
    pollerRunning = true;
    pollerThreadId = epicsThreadCreate(
        "MicrotestPoller",
        epicsThreadPriorityMedium,
        epicsThreadGetStackSize(epicsThreadStackSmall),
                                       (EPICSTHREADFUNC)pollerTaskC,
                                       this
    );
}

/* Destructor */
Microtest::~Microtest()
{
    pollerRunning = false;

    if (pollerThreadId) {
        epicsThreadSleep(0.3);  // Thread sauber beenden lassen
    }

    if (hLib) {
        epicsMutexLock(dllLock);
        MT_Disconnect();
        epicsMutexUnlock(dllLock);
        FreeLibrary(hLib);
    }
    epicsMutexDestroy(dllLock);
}

void Microtest::pollerTaskC(void *drvPvt)
{
    Microtest *pDrv = static_cast<Microtest *>(drvPvt);
    pDrv->pollerTask();
}

bool Microtest::initDLL()
{
    if (dllPath_.empty()) return false;

    hLib = LoadLibraryA(dllPath_.c_str());
    if (!hLib) {
        printf("Microtest: cannot load DLL at %s\n", dllPath_.c_str());
        return false;
    }

    MT_Connect              = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_Connect");
    MT_Disconnect           = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_Disconnect");
    MT_Version              = (DLL_MT_Version)    GetProcAddress(hLib, "MT_Version");
    MT_IsMotorRunning       = (DLL_MT_Bool)       GetProcAddress(hLib, "MT_IsMotorRunning");
    //MT_StartMotor           = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_StartMotor");
    MT_StopMotor            = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_StopMotor");
    MT_GetForce             = (DLL_MT_Double)     GetProcAddress(hLib, "MT_GetForce");
    MT_GetPosition          = (DLL_MT_Double)     GetProcAddress(hLib, "MT_GetPosition");
    MT_GetExtension         = (DLL_MT_Double)     GetProcAddress(hLib, "MT_GetExtension");
    MT_GetMotorSpeedIndex   = (DLL_MT_Integer)    GetProcAddress(hLib, "MT_GetMotorSpeedIndex");
    MT_GotoLoad             = (DLL_MT_SetDouble)  GetProcAddress(hLib, "MT_GotoLoad");
    MT_GotoAbsolutePosition = (DLL_MT_SetDouble)  GetProcAddress(hLib, "MT_GotoAbsolutePosition");
    MT_GotoExtension        = (DLL_MT_SetDouble)  GetProcAddress(hLib, "MT_GotoExtension");
    MT_ModeTensile          = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_ModeTensile");
    MT_ModeCompressive      = (DLL_MT_NoParams)   GetProcAddress(hLib, "MT_ModeCompressive");
    MT_SetMotorSpeed        = (DLL_MT_SetInteger) GetProcAddress(hLib, "MT_SetMotorSpeed");

    if (!MT_Connect) {
        printf("Microtest: missing required function in DLL\n");
        FreeLibrary(hLib);
        hLib = NULL;
        return false;
    }

    return true;
}

void Microtest::pollerTask()
{
    if (!initDLL()) {
        printf("Microtest: DLL initialization failed\n");
        return;
    }

    epicsMutexLock(dllLock);
    MT_Connect();
    epicsMutexUnlock(dllLock);

    int maj = 0, min = 0;
    MT_Version(&maj, &min);
    printf("Microtest DLL version %d.%d\n", maj, min);

    /* --- Force stability tracking --- */
    double stableTime = 0.0;
    double lastForce  = 0.0;
    double pct = 0.0;
    double forceThreshold;
    double stableDeltaPct;
    double stableTimeReq;
    const double dt    = 0.1;  // wie Poll-Periode
    int    stableEnable;
    bool   newStabilityCycle = false;

    while (pollerRunning) {

        epicsMutexLock(dllLock);

        if (doSetMode) {
            setModeValue ? MT_ModeCompressive() : MT_ModeTensile();
            doSetMode = false;
        }
        if (doSetMotorSpeed) {
            MT_SetMotorSpeed(setMotorSpeedValue);
            doSetMotorSpeed = false;
        }
        if (doStopMotor) {
            MT_StopMotor();
            doStopMotor = false;
        }

        if (doGotoAbsolutePosition) {
            MT_GotoAbsolutePosition(gotoAbsolutePositionValue);
            doGotoAbsolutePosition = false;
        }
        if (doGotoLoad) {
            MT_GotoLoad(gotoLoadValue);
            doGotoLoad = false;
        }
        if (doGotoExtension) {
            MT_GotoExtension(gotoExtensionValue);
            doGotoExtension = false;
        }

        // ----- regular polling -----
        double force      = MT_GetForce();
        double position   = MT_GetPosition();
        double extension  = MT_GetExtension();
        int    speed      = MT_GetMotorSpeedIndex();
        int    running    = MT_IsMotorRunning();

        epicsMutexUnlock(dllLock);

        // ----- Update EPICS parameters -----
        setDoubleParam(Force_,          force);
        setDoubleParam(ExtensionRBV_,   extension);
        setDoubleParam(StartPosition_,  position);
        setIntegerParam(MotorSpeedRBV_, speed);

        getIntegerParam(UseForceStability_, &stableEnable);
        getDoubleParam(ForceThreshold_,     &forceThreshold);
        getDoubleParam(ForceStableTime_,    &stableTimeReq);

        if (stableEnable && running && (force >= forceThreshold)) {
            newStabilityCycle = true;
        } else if (stableEnable && running && (force < forceThreshold)) {
            newStabilityCycle = false;
        }

        if ((!running) && newStabilityCycle) {
            getDoubleParam(ForceDeltaPercent_, &stableDeltaPct);
            //printf("ForceDeltaPercent: %f\n", stableDeltaPct);

            double diff = fabs(force - lastForce);
            pct  = (lastForce != 0.0) ? fabs(diff / lastForce) * 100.0 : 0.0;

            if (pct <= stableDeltaPct) {
                stableTime += dt;
            } else {
                stableTime = 0.0;
            }

            lastForce = force;

            if (stableTime >= stableTimeReq) {
                newStabilityCycle = false;
            }
        }
        setDoubleParam(ForceDeltaPercentRBV_, pct);
        setDoubleParam(ForceStableTimeRBV_, stableTime);
        // Motor is done only if it has stopped AND
        // force stability is either disabled or fulfilled or force is below threshold
        int done = (!running) && ( (!stableEnable) || (stableTime >= stableTimeReq) || (force < forceThreshold) );
        setIntegerParam(MovingDone_, done);

        callParamCallbacks();

        epicsThreadSleep(dt);   // 100 ms
    }

    epicsMutexLock(dllLock);
    MT_Disconnect();
    epicsMutexUnlock(dllLock);
}

asynStatus Microtest::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;

    // Set the parameter in the parameter library.
    setIntegerParam(function, value);

    if (function == Mode_)
    {
        setModeValue = value;
        doSetMode = true;
    }
    if (function == StopMotor_)
    {
        doStopMotor = true;
    }
    if (function == MotorSpeed_)
    {
        setMotorSpeedValue = value;
        doSetMotorSpeed = true;
    }

    // Notify records
    callParamCallbacks();

    return asynSuccess;
}

asynStatus Microtest::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;

    // Set the parameter in the parameter library.
    setDoubleParam(function, value);

    // Set command flags (executed in poller thread)
    if (function == AbsolutePosition_)
    {
        gotoAbsolutePositionValue = value;
        doGotoAbsolutePosition = true;
    }
    if (function == Load_)
    {
        gotoLoadValue = value;
        doGotoLoad = true;
    }
    if (function == Extension_)
    {
        gotoExtensionValue = value;
        doGotoExtension = true;
    }

    // Notify records
    callParamCallbacks();

    return asynSuccess;
}


/** Configuration command, called directly or from iocsh */
extern "C" int MicrotestConfig(const char *portName, const char *dllPath)
{
    //Microtest *pMicrotest = new Microtest(portName, dllPath);
    new Microtest(portName, dllPath);
    return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg configArg1 = { "DLL path",  iocshArgString };
static const iocshArg * const configArgs[] = {&configArg0, &configArg1};
static const iocshFuncDef configFuncDef = {"MicrotestConfig", 2, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
    MicrotestConfig(args[0].sval, args[1].sval);
}

void drvMicrotestRegister(void)
{
    iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
    epicsExportRegistrar(drvMicrotestRegister);
}
