#include <Sequence.h>

#define EXOS_ASSERT_LOG &handle->logger
#define EXOS_ASSERT_CALLBACK inst->_state = 255;
#include "exos_log.h"
#include "exos_sequence.h"
#include <string.h>

#define SUCCESS(_format_, ...) exos_log_success(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&handle->logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&handle->logger, _format_, ##__VA_ARGS__);

typedef struct
{
    void *self;
    exos_log_handle_t logger;
    Sequence data;

    exos_datamodel_handle_t sequence;

    exos_dataset_handle_t buttons;
    exos_dataset_handle_t start;
    exos_dataset_handle_t active;
} SequenceHandle_t;

static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    struct SequenceCyclic *inst = (struct SequenceCyclic *)dataset->datamodel->user_context;
    SequenceHandle_t *handle = (SequenceHandle_t *)inst->Handle;

    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        //handle each subscription dataset separately
        if(0 == strcmp(dataset->name, "Active"))
        {
            inst->pSequence->Active = *(BOOL *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published to local server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if(0 == strcmp(dataset->name, "Buttons"))
        {
            // SequenceButtons *buttons = (SequenceButtons *)dataset->data;
        }
        else if(0 == strcmp(dataset->name, "Start"))
        {
            // BOOL *start = (BOOL *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_DELIVERED:
        VERBOSE("dataset %s delivered to remote server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if(0 == strcmp(dataset->name, "Buttons"))
        {
            // SequenceButtons *buttons = (SequenceButtons *)dataset->data;
        }
        else if(0 == strcmp(dataset->name, "Start"))
        {
            // BOOL *start = (BOOL *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        INFO("dataset %s changed state to %s", dataset->name, exos_get_state_string(dataset->connection_state));

        switch (dataset->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            break;
        case EXOS_STATE_CONNECTED:
            //call the dataset changed event to update the dataset when connected
            //datasetEvent(dataset,EXOS_DATASET_UPDATED,info);
            break;
        case EXOS_STATE_OPERATIONAL:
            break;
        case EXOS_STATE_ABORTED:
            ERROR("dataset %s error %d (%s) occured", dataset->name, dataset->error, exos_get_error_string(dataset->error));
            break;
        }
        break;
    }

}

static void datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info)
{
    struct SequenceCyclic *inst = (struct SequenceCyclic *)datamodel->user_context;
    SequenceHandle_t *handle = (SequenceHandle_t *)inst->Handle;

    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application changed state to %s", exos_get_state_string(datamodel->connection_state));

        inst->Disconnected = 0;
        inst->Connected = 0;
        inst->Operational = 0;
        inst->Aborted = 0;

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            inst->Disconnected = 1;
            inst->_state = 255;
            break;
        case EXOS_STATE_CONNECTED:
            inst->Connected = 1;
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("Sequence operational!");
            inst->Operational = 1;
            break;
        case EXOS_STATE_ABORTED:
            ERROR("application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            inst->_state = 255;
            inst->Aborted = 1;
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;

    }

}

_BUR_PUBLIC void SequenceInit(struct SequenceInit *inst)
{
    SequenceHandle_t *handle;
    TMP_alloc(sizeof(SequenceHandle_t), (void **)&handle);
    if (NULL == handle)
    {
        inst->Handle = 0;
        return;
    }

    memset(&handle->data, 0, sizeof(handle->data));
    handle->self = handle;

    exos_log_init(&handle->logger, "gSequence_0");

    
    
    exos_datamodel_handle_t *sequence = &handle->sequence;
    exos_dataset_handle_t *buttons = &handle->buttons;
    exos_dataset_handle_t *start = &handle->start;
    exos_dataset_handle_t *active = &handle->active;
    EXOS_ASSERT_OK(exos_datamodel_init(sequence, "Sequence_0", "gSequence_0"));

    EXOS_ASSERT_OK(exos_dataset_init(buttons, sequence, "Buttons", &handle->data.Buttons, sizeof(handle->data.Buttons)));
    EXOS_ASSERT_OK(exos_dataset_init(start, sequence, "Start", &handle->data.Start, sizeof(handle->data.Start)));
    EXOS_ASSERT_OK(exos_dataset_init(active, sequence, "Active", &handle->data.Active, sizeof(handle->data.Active)));
    
    inst->Handle = (UDINT)handle;
}

_BUR_PUBLIC void SequenceCyclic(struct SequenceCyclic *inst)
{
    SequenceHandle_t *handle = (SequenceHandle_t *)inst->Handle;

    inst->Error = false;
    if (NULL == handle || NULL == inst->pSequence)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    Sequence *data = &handle->data;
    exos_datamodel_handle_t *sequence = &handle->sequence;
    //the user context of the datamodel points to the SequenceCyclic instance
    sequence->user_context = inst; //set it cyclically in case the program using the FUB is retransferred
    sequence->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != sequence->datamodel_event_callback && sequence->datamodel_event_callback != datamodelEvent)
    {
        sequence->datamodel_event_callback = datamodelEvent;
        exos_log_delete(&handle->logger);
        exos_log_init(&handle->logger, "gSequence_0");
    }

    exos_dataset_handle_t *buttons = &handle->buttons;
    buttons->user_context = NULL; //user defined
    buttons->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != buttons->dataset_event_callback && buttons->dataset_event_callback != datasetEvent)
    {
        buttons->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *start = &handle->start;
    start->user_context = NULL; //user defined
    start->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != start->dataset_event_callback && start->dataset_event_callback != datasetEvent)
    {
        start->dataset_event_callback = datasetEvent;
    }


    exos_dataset_handle_t *active = &handle->active;
    active->user_context = NULL; //user defined
    active->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != active->dataset_event_callback && active->dataset_event_callback != datasetEvent)
    {
        active->dataset_event_callback = datasetEvent;
    }

    //unregister on disable
    if (inst->_state && !inst->Enable)
    {
        inst->_state = 255;
    }

    switch (inst->_state)
    {
    case 0:
        inst->Disconnected = 1;
        inst->Connected = 0;
        inst->Operational = 0;
        inst->Aborted = 0;

        if (inst->Enable)
        {
            inst->_state = 10;
        }
        break;

    case 10:
        inst->_state = 100;

        SUCCESS("starting Sequence application..");

        //connect the datamodel, then the datasets
        EXOS_ASSERT_OK(exos_datamodel_connect_sequence(sequence, datamodelEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(buttons, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(start, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(active, EXOS_DATASET_SUBSCRIBE, datasetEvent));

        inst->Active = true;
        break;

    case 100:
    case 101:
        if (inst->Start)
        {
            if (inst->_state == 100)
            {
                EXOS_ASSERT_OK(exos_datamodel_set_operational(sequence));
                inst->_state = 101;
            }
        }
        else
        {
            inst->_state = 100;
        }

        EXOS_ASSERT_OK(exos_datamodel_process(sequence));
        //put your cyclic code here!

        //publish the buttons dataset as soon as there are changes
        if (0 != memcmp(&inst->pSequence->Buttons, &data->Buttons, sizeof(data->Buttons)))
        {
            memcpy(&data->Buttons, &inst->pSequence->Buttons, sizeof(data->Buttons));
            exos_dataset_publish(buttons);
        }
        //publish the start dataset as soon as there are changes
        if (inst->pSequence->Start != data->Start)
        {
            data->Start = inst->pSequence->Start;
            exos_dataset_publish(start);
        }

        break;

    case 255:
        //disconnect the datamodel
        EXOS_ASSERT_OK(exos_datamodel_disconnect(sequence));

        inst->Active = false;
        inst->_state = 254;
        //no break

    case 254:
        if (!inst->Enable)
            inst->_state = 0;
        break;
    }

    exos_log_process(&handle->logger);

}

_BUR_PUBLIC void SequenceExit(struct SequenceExit *inst)
{
    SequenceHandle_t *handle = (SequenceHandle_t *)inst->Handle;

    if (NULL == handle)
    {
        ERROR("SequenceExit: NULL handle, cannot delete resources");
        return;
    }
    if ((void *)handle != handle->self)
    {
        ERROR("SequenceExit: invalid handle, cannot delete resources");
        return;
    }

    exos_datamodel_handle_t *sequence = &handle->sequence;

    EXOS_ASSERT_OK(exos_datamodel_delete(sequence));

    //finish with deleting the log
    exos_log_delete(&handle->logger);
    //free the allocated handle
    TMP_free(sizeof(SequenceHandle_t), (void *)handle);
}

