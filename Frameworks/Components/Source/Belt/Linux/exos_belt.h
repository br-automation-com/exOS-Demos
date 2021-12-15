/*Automatically generated header file from Belt.typ*/

#ifndef _EXOS_COMP_BELT_H_
#define _EXOS_COMP_BELT_H_

#include "exos_api.h"

#if defined(_SG4)
#include <Belt.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct BeltActuator
{
    int32_t On;
    int32_t Off;
    int32_t LatchIndex;

} BeltActuator;

typedef struct BeltStatus
{
    bool Running;
    bool Latched;
    int32_t LatchIndex;
    bool Actuator;
    int32_t ActualSpeed;
    int32_t ActualPosition;
    int32_t LatchedPosition;

} BeltStatus;

typedef struct BeltIO
{
    bool iRun;
    int16_t iSpeed;
    bool iLatch;
    bool oActuator;
    int16_t oActPos;
    bool oLatchActive;

} BeltIO;

typedef struct Belt
{
    struct BeltIO IO; //I/O channels
    struct BeltStatus Status; //Current Belt Values PUB
    struct BeltActuator Actuator; //Actuator set values from Linux SUB
    bool Framework; //Linux Application is connected to Framework SUB

} Belt;

#endif // _SG4

EXOS_ERROR_CODE exos_datamodel_connect_belt(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback);

#endif // _EXOS_COMP_BELT_H_
