/*Automatically generated header file from Sequence.typ*/

#ifndef _EXOS_COMP_SEQUENCE_H_
#define _EXOS_COMP_SEQUENCE_H_

#include "exos_api.h"

#if defined(_SG4)
#include <Sequence.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct SequenceButtons
{
    bool ButtonLeft;
    bool ButtonRight;

} SequenceButtons;

typedef struct Sequence
{
    struct SequenceButtons Buttons; //PUB
    bool Start; //PUB
    bool Active; //SUB

} Sequence;

#endif // _SG4

EXOS_ERROR_CODE exos_datamodel_connect_sequence(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback);

#endif // _EXOS_COMP_SEQUENCE_H_
