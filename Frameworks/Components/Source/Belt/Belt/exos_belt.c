/*Automatically generated c file from Belt.typ*/

#include "exos_belt.h"

const char config_belt[] = "{\"name\":\"struct\",\"attributes\":{\"name\":\"<NAME>\",\"dataType\":\"Belt\",\"info\":\"<infoId0>\"},\"children\":[{\"name\":\"struct\",\"attributes\":{\"name\":\"IO\",\"dataType\":\"BeltIO\",\"info\":\"<infoId1>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"iRun\",\"dataType\":\"BOOL\",\"info\":\"<infoId2>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"iSpeed\",\"dataType\":\"INT\",\"info\":\"<infoId3>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"iLatch\",\"dataType\":\"BOOL\",\"info\":\"<infoId4>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"oActuator\",\"dataType\":\"BOOL\",\"info\":\"<infoId5>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"oActPos\",\"dataType\":\"INT\",\"info\":\"<infoId6>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"oLatchActive\",\"dataType\":\"BOOL\",\"info\":\"<infoId7>\"}}]},{\"name\":\"struct\",\"attributes\":{\"name\":\"Status\",\"dataType\":\"BeltStatus\",\"comment\":\"PUB\",\"info\":\"<infoId8>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"Running\",\"dataType\":\"BOOL\",\"info\":\"<infoId9>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"Latched\",\"dataType\":\"BOOL\",\"info\":\"<infoId10>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"LatchIndex\",\"dataType\":\"DINT\",\"info\":\"<infoId11>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"Actuator\",\"dataType\":\"BOOL\",\"info\":\"<infoId12>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"ActualSpeed\",\"dataType\":\"DINT\",\"info\":\"<infoId13>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"ActualPosition\",\"dataType\":\"DINT\",\"info\":\"<infoId14>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"LatchedPosition\",\"dataType\":\"DINT\",\"info\":\"<infoId15>\"}}]},{\"name\":\"struct\",\"attributes\":{\"name\":\"Actuator\",\"dataType\":\"BeltActuator\",\"comment\":\"SUB\",\"info\":\"<infoId16>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"On\",\"dataType\":\"DINT\",\"info\":\"<infoId17>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"Off\",\"dataType\":\"DINT\",\"info\":\"<infoId18>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"LatchIndex\",\"dataType\":\"DINT\",\"info\":\"<infoId19>\"}}]},{\"name\":\"variable\",\"attributes\":{\"name\":\"Framework\",\"dataType\":\"BOOL\",\"comment\":\"SUB\",\"info\":\"<infoId20>\"}}]}";

/*Connect the Belt datamodel to the server*/
EXOS_ERROR_CODE exos_datamodel_connect_belt(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback)
{
    Belt data;
    exos_dataset_info_t datasets[] = {
        {EXOS_DATASET_BROWSE_NAME_INIT,{}},
        {EXOS_DATASET_BROWSE_NAME(IO),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.iRun),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.iSpeed),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.iLatch),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.oActuator),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.oActPos),{}},
        {EXOS_DATASET_BROWSE_NAME(IO.oLatchActive),{}},
        {EXOS_DATASET_BROWSE_NAME(Status),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.Running),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.Latched),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.LatchIndex),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.Actuator),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.ActualSpeed),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.ActualPosition),{}},
        {EXOS_DATASET_BROWSE_NAME(Status.LatchedPosition),{}},
        {EXOS_DATASET_BROWSE_NAME(Actuator),{}},
        {EXOS_DATASET_BROWSE_NAME(Actuator.On),{}},
        {EXOS_DATASET_BROWSE_NAME(Actuator.Off),{}},
        {EXOS_DATASET_BROWSE_NAME(Actuator.LatchIndex),{}},
        {EXOS_DATASET_BROWSE_NAME(Framework),{}}
    };

    exos_datamodel_calc_dataset_info(datasets, sizeof(datasets));

    return exos_datamodel_connect(datamodel, config_belt, datasets, sizeof(datasets), datamodel_event_callback);
}
