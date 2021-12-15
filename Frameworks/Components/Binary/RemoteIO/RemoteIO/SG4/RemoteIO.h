/* Automation Studio generated header file */
/* Do not edit ! */
/* RemoteIO  */

#ifndef _REMOTEIO_
#define _REMOTEIO_
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
typedef struct RemoteIOAnalog
{	plcstring Name[81];
	signed short State;
} RemoteIOAnalog;

typedef struct RemoteIODigital
{	plcstring Name[81];
	plcbit State;
} RemoteIODigital;

typedef struct RemoteIO
{	struct RemoteIOAnalog AnalogInput;
	struct RemoteIODigital DigitalInput;
	struct RemoteIODigital DigitalOutput;
	plcstring NewAnalogInput[81];
	plcstring NewDigitalInput[81];
	plcstring NewDigitalOutput[81];
} RemoteIO;

typedef struct RemoteDI
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	plcstring Name[81];
	/* VAR (analog) */
	unsigned char _state;
	/* VAR_INPUT (digital) */
	plcbit Input;
	/* VAR_OUTPUT (digital) */
	plcbit Ready;
	plcbit Error;
	/* VAR (digital) */
	plcbit _input;
} RemoteDI_typ;

typedef struct RemoteAI
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	plcstring Name[81];
	signed short Input;
	/* VAR (analog) */
	unsigned char _state;
	signed short _input;
	/* VAR_OUTPUT (digital) */
	plcbit Ready;
	plcbit Error;
} RemoteAI_typ;

typedef struct RemoteDO
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	plcstring Name[81];
	/* VAR (analog) */
	unsigned char _state;
	unsigned long _inst;
	/* VAR_OUTPUT (digital) */
	plcbit Ready;
	plcbit Error;
	plcbit Output;
} RemoteDO_typ;

typedef struct RemoteIOInit
{
	/* VAR_OUTPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} RemoteIOInit_typ;

typedef struct RemoteIOCyclic
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
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
} RemoteIOCyclic_typ;

typedef struct RemoteIOExit
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} RemoteIOExit_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void RemoteDI(struct RemoteDI* inst);
_BUR_PUBLIC void RemoteAI(struct RemoteAI* inst);
_BUR_PUBLIC void RemoteDO(struct RemoteDO* inst);
_BUR_PUBLIC void RemoteIOInit(struct RemoteIOInit* inst);
_BUR_PUBLIC void RemoteIOCyclic(struct RemoteIOCyclic* inst);
_BUR_PUBLIC void RemoteIOExit(struct RemoteIOExit* inst);


#ifdef __cplusplus
};
#endif
#endif /* _REMOTEIO_ */

