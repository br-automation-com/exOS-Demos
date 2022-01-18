/* Automation Studio generated header file */
/* Do not edit ! */
/* Sequence  */

#ifndef _SEQUENCE_
#define _SEQUENCE_
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
typedef struct SequenceButtons
{	plcbit ButtonLeft;
	plcbit ButtonRight;
} SequenceButtons;

typedef struct Sequence
{	struct SequenceButtons Buttons;
	plcbit Start;
	plcbit Active;
} Sequence;

typedef struct SequenceInit
{
	/* VAR_OUTPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} SequenceInit_typ;

typedef struct SequenceCyclic
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	struct Sequence* pSequence;
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
} SequenceCyclic_typ;

typedef struct SequenceExit
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} SequenceExit_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void SequenceInit(struct SequenceInit* inst);
_BUR_PUBLIC void SequenceCyclic(struct SequenceCyclic* inst);
_BUR_PUBLIC void SequenceExit(struct SequenceExit* inst);


#ifdef __cplusplus
};
#endif
#endif /* _SEQUENCE_ */

