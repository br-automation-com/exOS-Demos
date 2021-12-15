/*Automatically generated header file from RemoteIO.typ*/

#ifndef _EXOS_COMP_REMOTEIO_H_
#define _EXOS_COMP_REMOTEIO_H_

#include "exos_api.h"

#if defined(_SG4)
#include <RemoteIO.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct RemoteIODigital
{
    char Name[81];
    bool State;

} RemoteIODigital;

typedef struct RemoteIOAnalog
{
    char Name[81];
    int16_t State;

} RemoteIOAnalog;

typedef struct RemoteIO
{
    struct RemoteIOAnalog AnalogInput; //PUB
    struct RemoteIODigital DigitalInput; //PUB
    struct RemoteIODigital DigitalOutput; //SUB
    char NewAnalogInput[81]; //PUB
    char NewDigitalInput[81]; //PUB
    char NewDigitalOutput[81]; //PUB

} RemoteIO;

#endif // _SG4

EXOS_ERROR_CODE exos_datamodel_connect_remoteio(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback);

#endif // _EXOS_COMP_REMOTEIO_H_
