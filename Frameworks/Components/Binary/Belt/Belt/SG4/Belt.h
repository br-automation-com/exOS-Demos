/* Automation Studio generated header file */
/* Do not edit ! */
/* Belt  */

#ifndef _BELT_
#define _BELT_
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
typedef struct BeltActuator
{	signed long On;
	signed long Off;
	signed long LatchIndex;
} BeltActuator;

typedef struct BeltStatus
{	plcbit Running;
	plcbit Latched;
	signed long LatchIndex;
	plcbit Actuator;
	signed long ActualSpeed;
	signed long ActualPosition;
	signed long LatchedPosition;
} BeltStatus;

typedef struct BeltIO
{	plcbit iRun;
	signed short iSpeed;
	plcbit iLatch;
	plcbit oActuator;
	signed short oActPos;
	plcbit oLatchActive;
} BeltIO;

typedef struct Belt
{	struct BeltIO IO;
	struct BeltStatus Status;
	struct BeltActuator Actuator;
	plcbit Framework;
} Belt;

typedef struct BeltCyclic
{
	/* VAR_INPUT (analog) */
	struct Belt* pBelt;
	/* VAR (analog) */
	unsigned long _Handle;
	/* VAR_INPUT (digital) */
	plcbit Enable;
	plcbit Start;
	/* VAR_OUTPUT (digital) */
	plcbit Connected;
	plcbit Operational;
	plcbit Error;
	/* VAR (digital) */
	plcbit _Start;
	plcbit _Enable;
} BeltCyclic_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void BeltCyclic(struct BeltCyclic* inst);


#ifdef __cplusplus
};
#endif
#endif /* _BELT_ */

