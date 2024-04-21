
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <Arduino.h>
#include <common.h>
#include <HCSR04.h>
#include <CheapStepper.h>
#include <queue> 
#include <TaskSchedulerDeclarations.h>
#include <queue> 
#include <functional>
#include <list>
#include <memory>

#define MAIN_DOOR   "Garage"
#define DEBOUCE_OK  3

class Application 
{    

public:

    void init                           (void);
    void loop                           (void);
         Application                    ();
         ~Application                   ();

    void readProximityCallback          (void);         
    void queueCallback                  (void);
    void moveCallback                   (void);

private:

    void establishConnection            (void);
    void connectToWiFi                  (void);
    void initializeTime                 (void);
    void initializeIoTHubClient         (void);
    int  initializeMqttClient           (void);
    void sendTelemetry                  (String sentense);
        
private:

    Scheduler                   _scheduler;
    Task                        _proximityTask;
    Task                        _queueTask;
    Task                        _moveTask;
    UltraSonicDistanceSensor *  _pDistanceSensor;
    CheapStepper *              _pStepper;

    String                      _deviceId;
    std::queue <String>         _eventQueue;    
    volatile bool               _isDoorOpen;
    volatile short              _openDebounce;
    volatile short              _closeDebounce;
    char                        _buffer[256];
};

#endif // __APPLICATION_H__
