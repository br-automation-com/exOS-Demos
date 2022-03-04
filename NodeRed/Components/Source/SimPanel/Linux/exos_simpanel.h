/*Automatically generated header file from SimPanel.typ*/

#ifndef _EXOS_COMP_SIMPANEL_H_
#define _EXOS_COMP_SIMPANEL_H_

#include "exos_api.h"

#if defined(_SG4)
#include <SimPanel.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct SimPanelKnobs
{
    int16_t P1;
    int16_t P2;

} SimPanelKnobs;

typedef struct SimPanelLED
{
    bool Green;
    bool Red;
    bool Yellow;

} SimPanelLED;

typedef struct SimPanelLEDs
{
    bool DI1;
    bool DI2;
    struct SimPanelLED DI3;
    struct SimPanelLED DI4;
    struct SimPanelLED DI5;
    struct SimPanelLED DI6;

} SimPanelLEDs;

typedef struct SimPanelButtons
{
    bool DI3;
    bool DI4;
    bool DI5;
    bool DI6;
    bool Encoder;

} SimPanelButtons;

typedef struct SimPanelSwitches
{
    bool DI1;
    bool DI2;

} SimPanelSwitches;

typedef struct SimPanel
{
    struct SimPanelSwitches Switches; //PUB
    struct SimPanelButtons Buttons; //PUB
    struct SimPanelLEDs LEDs; //SUB
    struct SimPanelKnobs Knobs; //PUB
    int16_t Display; //SUB
    uint16_t Encoder; //PUB

} SimPanel;

#endif // _SG4

EXOS_ERROR_CODE exos_datamodel_connect_simpanel(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback);

#endif // _EXOS_COMP_SIMPANEL_H_
