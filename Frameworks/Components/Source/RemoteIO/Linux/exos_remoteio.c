/*Automatically generated c file from RemoteIO.typ*/

#include "exos_remoteio.h"

const char config_remoteio[] = "{\"name\":\"struct\",\"attributes\":{\"name\":\"<NAME>\",\"dataType\":\"RemoteIO\",\"info\":\"<infoId0>\"},\"children\":[{\"name\":\"struct\",\"attributes\":{\"name\":\"AnalogInput\",\"dataType\":\"RemoteIOAnalog\",\"comment\":\"PUB\",\"info\":\"<infoId1>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"Name\",\"dataType\":\"STRING\",\"stringLength\":81,\"info\":\"<infoId2>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"State\",\"dataType\":\"INT\",\"info\":\"<infoId3>\"}}]},{\"name\":\"struct\",\"attributes\":{\"name\":\"DigitalInput\",\"dataType\":\"RemoteIODigital\",\"comment\":\"PUB\",\"info\":\"<infoId4>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"Name\",\"dataType\":\"STRING\",\"stringLength\":81,\"info\":\"<infoId5>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"State\",\"dataType\":\"BOOL\",\"info\":\"<infoId6>\"}}]},{\"name\":\"struct\",\"attributes\":{\"name\":\"DigitalOutput\",\"dataType\":\"RemoteIODigital\",\"comment\":\"SUB\",\"info\":\"<infoId7>\"},\"children\":[{\"name\":\"variable\",\"attributes\":{\"name\":\"Name\",\"dataType\":\"STRING\",\"stringLength\":81,\"info\":\"<infoId8>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"State\",\"dataType\":\"BOOL\",\"info\":\"<infoId9>\"}}]},{\"name\":\"variable\",\"attributes\":{\"name\":\"NewAnalogInput\",\"dataType\":\"STRING\",\"stringLength\":81,\"comment\":\"PUB\",\"info\":\"<infoId10>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"NewDigitalInput\",\"dataType\":\"STRING\",\"stringLength\":81,\"comment\":\"PUB\",\"info\":\"<infoId11>\"}},{\"name\":\"variable\",\"attributes\":{\"name\":\"NewDigitalOutput\",\"dataType\":\"STRING\",\"stringLength\":81,\"comment\":\"PUB\",\"info\":\"<infoId12>\"}}]}";

/*Connect the RemoteIO datamodel to the server*/
EXOS_ERROR_CODE exos_datamodel_connect_remoteio(exos_datamodel_handle_t *datamodel, exos_datamodel_event_cb datamodel_event_callback)
{
    RemoteIO data;
    exos_dataset_info_t datasets[] = {
        {EXOS_DATASET_BROWSE_NAME_INIT,{}},
        {EXOS_DATASET_BROWSE_NAME(AnalogInput),{}},
        {EXOS_DATASET_BROWSE_NAME(AnalogInput.Name),{}},
        {EXOS_DATASET_BROWSE_NAME(AnalogInput.State),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalInput),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalInput.Name),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalInput.State),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalOutput),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalOutput.Name),{}},
        {EXOS_DATASET_BROWSE_NAME(DigitalOutput.State),{}},
        {EXOS_DATASET_BROWSE_NAME(NewAnalogInput),{}},
        {EXOS_DATASET_BROWSE_NAME(NewDigitalInput),{}},
        {EXOS_DATASET_BROWSE_NAME(NewDigitalOutput),{}}
    };

    exos_datamodel_calc_dataset_info(datasets, sizeof(datasets));

    return exos_datamodel_connect(datamodel, config_remoteio, datasets, sizeof(datasets), datamodel_event_callback);
}
