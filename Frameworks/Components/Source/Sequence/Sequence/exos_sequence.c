/*Automatically generated c file from Sequence.typ*/

#include "exos_sequence.h"

const char config_sequence[] = "{\"name\":\"struct\",\"attributes\":{\"name\":\"<NAME>\",\"dataType\":\"Sequence\",\"info\":\"<infoId0>\"},\"children\":[{\"name\":\"struct\",\"attributes\":{\"name\":\"Buttons\",\"dataType\":\"SequenceButtons\",\"comment\":\"PUB\",\"info\":\"<infoId1>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"ButtonLeft\",\"dataType\":\"BOOL\",\"info\":\"<infoId2>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"ButtonRight\",\"dataType\":\"BOOL\",\"info\":\"<infoId3>\"}}]},{\"name\":\"variable\",\"attributes\":{\"name\":\"Start\",\"dataType\":\"BOOL\",\"comment\":\"PUB\",\"info\":\"<infoId4>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"Active\",\"dataType\":\"BOOL\",\"comment\":\"SUB\",\"info\":\"<infoId5>\"}}]}";

/*Connect the Sequence datamodel to the server*/
EXOS_ERROR_CODE exos_datamodel_connect_sequence(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback)
{
    Sequence data;
    exos_dataset_info_t datasets[] = {
        {EXOS_DATASET_BROWSE_NAME_INIT,{}},
        {EXOS_DATASET_BROWSE_NAME(Buttons),{}},
        {EXOS_DATASET_BROWSE_NAME(Buttons.ButtonLeft),{}},
        {EXOS_DATASET_BROWSE_NAME(Buttons.ButtonRight),{}},
        {EXOS_DATASET_BROWSE_NAME(Start),{}},
        {EXOS_DATASET_BROWSE_NAME(Active),{}}
    };

    exos_datamodel_calc_dataset_info(datasets, sizeof(datasets));

    return exos_datamodel_connect(datamodel, config_sequence, datasets, sizeof(datasets), datamodel_event_callback);
}
