/* Automation Studio generated header file */
/* Do not edit ! */
/* SimPanel  */

#ifndef _SIMPANEL_
#define _SIMPANEL_
#ifdef __cplusplus
extern "C" 
{
#endif

#include <bur/plctypes.h>

#ifndef _BUR_PUBLIC
#define _BUR_PUBLIC
#endif
#ifdef _SG3
		#include "ExData.h"
#endif
#ifdef _SG4
		#include "ExData.h"
#endif
#ifdef _SGC
		#include "ExData.h"
#endif


/* Datatypes and datatypes of function blocks */
typedef struct SimPanelKnobs
{	signed short P1;
	signed short P2;
} SimPanelKnobs;

typedef struct SimPanelSwitches
{	plcbit DI1;
	plcbit DI2;
} SimPanelSwitches;

typedef struct SimPanelLED
{	plcbit Green;
	plcbit Red;
	plcbit Yellow;
} SimPanelLED;

typedef struct SimPanelLEDs
{	plcbit DI1;
	plcbit DI2;
	struct SimPanelLED DI3;
	struct SimPanelLED DI4;
	struct SimPanelLED DI5;
	struct SimPanelLED DI6;
} SimPanelLEDs;

typedef struct SimPanelButtons
{	plcbit DI3;
	plcbit DI4;
	plcbit DI5;
	plcbit DI6;
	plcbit Encoder;
} SimPanelButtons;

typedef struct SimPanel
{	struct SimPanelSwitches Switches;
	struct SimPanelButtons Buttons;
	struct SimPanelLEDs LEDs;
	struct SimPanelKnobs Knobs;
	signed short Display;
	unsigned short Encoder;
} SimPanel;

typedef struct SimPanelInit
{
	/* VAR_OUTPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} SimPanelInit_typ;

typedef struct SimPanelCyclic
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	struct SimPanel* pSimPanel;
	/* VAR (analog) */
	unsigned char _state;
	/* VAR_INPUT (digital) */
	plcbit Enable;
	plcbit Start;
	/* VAR_OUTPUT (digital) */
	plcbit Active;
	plcbit Error;
	plcbit Disconnected;
	plcbit Connected;
	plcbit Operational;
	plcbit Aborted;
} SimPanelCyclic_typ;

typedef struct SimPanelExit
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} SimPanelExit_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void SimPanelInit(struct SimPanelInit* inst);
_BUR_PUBLIC void SimPanelCyclic(struct SimPanelCyclic* inst);
_BUR_PUBLIC void SimPanelExit(struct SimPanelExit* inst);


#ifdef __cplusplus
};
#endif
#endif /* _SIMPANEL_ */

