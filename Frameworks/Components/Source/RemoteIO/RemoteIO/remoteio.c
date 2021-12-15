#include <RemoteIO.h>

//This is a simplified example of a serialized handling of in- and outputs. 
//The decent implementation would be to create fifos between the function blocks
//and the RemoteIOCyclic.

//This example requires all function blocks to run in the same task-class,
//and has a limit of 50 digital outputs (NUM_REMOTE_DO)

//If more than 50 value changes occur in one scan, or more than 50 DI/AI/DOs
//are registered in the same scan, it will most likely cause a buffer overflow. 
//Therefore, if it should be implemented properly, the dataset->send_buffer
//should be monitored together with emptying the FIFO in RemoteIOCyclic that
//no values are lost on the way.

#define EXOS_ASSERT_LOG &handle->logger
#define EXOS_ASSERT_CALLBACK inst->_state = 255;
#include "exos_log.h"
#include "exos_remoteio.h"
#include <string.h>

#define SUCCESS(_format_, ...) exos_log_success(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&handle->logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&handle->logger, _format_, ##__VA_ARGS__);

//As all DO updates go via the same DigitalOutput structure in the datasetEvent callback,
//we will keep a list of all RemoteDO function blocks, and update the Output value of the function block in the callback
typedef struct 
{
    char name[81];
    void *inst;
} RemoteDOList_t;

//we can have at max 50 RemoteDO function blocks
#define NUM_REMOTE_DO 50

typedef struct
{
    void *self;
    exos_log_handle_t logger;
    RemoteIO data;

    RemoteDOList_t remoteDO[NUM_REMOTE_DO];

    exos_datamodel_handle_t remoteio;

    exos_dataset_handle_t analoginput;
    exos_dataset_handle_t digitalinput;
    exos_dataset_handle_t digitaloutput;
    exos_dataset_handle_t newanaloginput;
    exos_dataset_handle_t newdigitalinput;
    exos_dataset_handle_t newdigitaloutput;
} RemoteIOHandle_t;

static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    struct RemoteIOCyclic *inst = (struct RemoteIOCyclic *)dataset->datamodel->user_context;
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;
    int i;

    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        //handle each subscription dataset separately
        if(0 == strcmp(dataset->name, "DigitalOutput"))
        {
            struct RemoteIODigital *output = (struct RemoteIODigital *)dataset->data;

            //go through the list of registered RemoteDO function blocks to see if we find a matching name
            for(i = 0; i < NUM_REMOTE_DO; i++)
            {
                if(0 == strcmp(output->Name, handle->remoteDO[i].name)) 
                {
                    if(NULL != handle->remoteDO[i].inst)
                    {
                        //OK! we found a matching name, and the Inst pointer is set (to the RemoteDO function block)
                        //-> update the Output value of the function block
                        struct RemoteDO *doinst = (struct RemoteDO *)handle->remoteDO[i].inst;
                        doinst->Output = output->State;
                    }
                    break;
                }
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published to local server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        break;

    case EXOS_DATASET_EVENT_DELIVERED:
        VERBOSE("dataset %s delivered to remote server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        INFO("dataset %s changed state to %s", dataset->name, exos_get_state_string(dataset->connection_state));

        switch (dataset->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            break;
        case EXOS_STATE_CONNECTED:
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
    struct RemoteIOCyclic *inst = (struct RemoteIOCyclic *)datamodel->user_context;
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;

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
            SUCCESS("RemoteIO operational!");
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

_BUR_PUBLIC void RemoteIOInit(struct RemoteIOInit *inst)
{
    RemoteIOHandle_t *handle;
    TMP_alloc(sizeof(RemoteIOHandle_t), (void **)&handle);
    if (NULL == handle)
    {
        inst->Handle = 0;
        return;
    }

    memset(&handle->data, 0, sizeof(handle->data));
    memset(&handle->remoteDO, 0, sizeof(handle->remoteDO));

    handle->self = handle;

    exos_log_init(&handle->logger, "gRemoteIO_0");

    
    
    exos_datamodel_handle_t *remoteio = &handle->remoteio;
    exos_dataset_handle_t *analoginput = &handle->analoginput;
    exos_dataset_handle_t *digitalinput = &handle->digitalinput;
    exos_dataset_handle_t *digitaloutput = &handle->digitaloutput;
    exos_dataset_handle_t *newanaloginput = &handle->newanaloginput;
    exos_dataset_handle_t *newdigitalinput = &handle->newdigitalinput;
    exos_dataset_handle_t *newdigitaloutput = &handle->newdigitaloutput;
    EXOS_ASSERT_OK(exos_datamodel_init(remoteio, "RemoteIO_0", "gRemoteIO_0"));

    EXOS_ASSERT_OK(exos_dataset_init(analoginput, remoteio, "AnalogInput", &handle->data.AnalogInput, sizeof(handle->data.AnalogInput)));
    EXOS_ASSERT_OK(exos_dataset_init(digitalinput, remoteio, "DigitalInput", &handle->data.DigitalInput, sizeof(handle->data.DigitalInput)));
    EXOS_ASSERT_OK(exos_dataset_init(digitaloutput, remoteio, "DigitalOutput", &handle->data.DigitalOutput, sizeof(handle->data.DigitalOutput)));
    EXOS_ASSERT_OK(exos_dataset_init(newanaloginput, remoteio, "NewAnalogInput", &handle->data.NewAnalogInput, sizeof(handle->data.NewAnalogInput)));
    EXOS_ASSERT_OK(exos_dataset_init(newdigitalinput, remoteio, "NewDigitalInput", &handle->data.NewDigitalInput, sizeof(handle->data.NewDigitalInput)));
    EXOS_ASSERT_OK(exos_dataset_init(newdigitaloutput, remoteio, "NewDigitalOutput", &handle->data.NewDigitalOutput, sizeof(handle->data.NewDigitalOutput)));
    
    inst->Handle = (UDINT)handle;
}

_BUR_PUBLIC void RemoteDO(struct RemoteDO *inst)
{
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;
    int i;
    inst->Ready = false;
    inst->Error = false;
    if (NULL == handle)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    RemoteIO *data = &handle->data;
    exos_dataset_handle_t *newdigitaloutput = &handle->newdigitaloutput;

    switch(inst->_state) {
        case 0:
            if(newdigitaloutput->connection_state != EXOS_STATE_CONNECTED && newdigitaloutput->connection_state != EXOS_STATE_OPERATIONAL)
                break;

            //the RemoteDO function block gets its value changes via the serialized DigitalOutput dataset callback
            //we therefore add this function block to an internal array and update the Output value of this function block in the callback
            for(i = 0; i < NUM_REMOTE_DO; i++)
            {
                if(handle->remoteDO[i].inst == NULL)
                {
                    strcpy(handle->remoteDO[i].name, inst->Name);
                    handle->remoteDO[i].inst = inst;
                    break;
                }
            }
            //save the current inst pointer to react on online changes
            inst->_inst = (UDINT)inst;

            strcpy(data->NewDigitalOutput, inst->Name);
            exos_dataset_publish(newdigitaloutput);
            inst->_state ++;
        case 1:
            //go back to state 0 if disconnected
            if(newdigitaloutput->connection_state != EXOS_STATE_CONNECTED && newdigitaloutput->connection_state != EXOS_STATE_OPERATIONAL)
            {
                inst->_state = 0;
                break;
            }
            inst->Ready = true;

            //if the inst pointer has changed (copy mode online change), we need to update the inst pointer in the callback list
            if(inst->_inst != (UDINT)inst)
            {
                for(i = 0; i < NUM_REMOTE_DO; i++)
                {
                    //update all (hopefully only one) occurences of the old pointer
                    if((UDINT)handle->remoteDO[i].inst == inst->_inst)
                    {
                        handle->remoteDO[i].inst = inst;
                    }
                }
                inst->_inst = (UDINT)inst;
            }
            break;
    }
}

_BUR_PUBLIC void RemoteDI(struct RemoteDI *inst)
{
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;

    inst->Ready = false;
    inst->Error = false;
    if (NULL == handle)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    RemoteIO *data = &handle->data;
    exos_dataset_handle_t *newdigitalinput = &handle->newdigitalinput;
    exos_dataset_handle_t *digitalinput = &handle->digitalinput;

    switch(inst->_state) {
        case 0:
            if(newdigitalinput->connection_state != EXOS_STATE_CONNECTED && newdigitalinput->connection_state != EXOS_STATE_OPERATIONAL)
                break;
            if(digitalinput->connection_state != EXOS_STATE_CONNECTED && digitalinput->connection_state != EXOS_STATE_OPERATIONAL)
                break;
            
            //register the digital input via the serialized NewDigitalInput dataset
            strcpy(data->NewDigitalInput, inst->Name);
            exos_dataset_publish(newdigitalinput);
            inst->_state ++;
        case 1:
            //go back to state 0 if disconnected
            if(digitalinput->connection_state != EXOS_STATE_CONNECTED && digitalinput->connection_state != EXOS_STATE_OPERATIONAL)
            {
                inst->_state = 0;
                break;
            }
            inst->Ready = true;
            //publish changes if they occur
            if(inst->Input != inst->_input)
            {
                inst->_input = inst->Input;
                data->DigitalInput.State = inst->Input;
                strcpy(data->DigitalInput.Name, inst->Name);
                exos_dataset_publish(digitalinput);
            }
            break;
    }
}

_BUR_PUBLIC void RemoteAI(struct RemoteAI *inst)
{
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;

    inst->Ready = false;
    inst->Error = false;
    if (NULL == handle)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    RemoteIO *data = &handle->data;
    exos_dataset_handle_t *newanaloginput = &handle->newanaloginput;
    exos_dataset_handle_t *analoginput = &handle->analoginput;

    switch(inst->_state) {
        case 0:
            if(newanaloginput->connection_state != EXOS_STATE_CONNECTED && newanaloginput->connection_state != EXOS_STATE_OPERATIONAL)
                break;
            if(analoginput->connection_state != EXOS_STATE_CONNECTED && analoginput->connection_state != EXOS_STATE_OPERATIONAL)
                break;
            
            //register the analig input via the serialized NewAnalogInput dataset
            strcpy(data->NewAnalogInput, inst->Name);
            exos_dataset_publish(newanaloginput);
            inst->_state ++;
        case 1:
            //go back to state 0 if disconnected
            if(analoginput->connection_state != EXOS_STATE_CONNECTED && analoginput->connection_state != EXOS_STATE_OPERATIONAL)
            {
                inst->_state = 0;
                break;
            }

            inst->Ready = true;
            //publish changes
            if(inst->Input != inst->_input)
            {
                inst->_input = inst->Input;
                data->AnalogInput.State = inst->Input;
                strcpy(data->AnalogInput.Name, inst->Name);
                exos_dataset_publish(analoginput);
            }
            break;
    }
}

_BUR_PUBLIC void RemoteIOCyclic(struct RemoteIOCyclic *inst)
{
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;

    inst->Error = false;
    if (NULL == handle)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    exos_datamodel_handle_t *remoteio = &handle->remoteio;
    //the user context of the datamodel points to the RemoteIOCyclic instance
    remoteio->user_context = inst; //set it cyclically in case the program using the FUB is retransferred
    remoteio->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != remoteio->datamodel_event_callback && remoteio->datamodel_event_callback != datamodelEvent)
    {
        remoteio->datamodel_event_callback = datamodelEvent;
        exos_log_delete(&handle->logger);
        exos_log_init(&handle->logger, "gRemoteIO_0");
    }

    exos_dataset_handle_t *analoginput = &handle->analoginput;
    analoginput->user_context = NULL; //user defined
    analoginput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != analoginput->dataset_event_callback && analoginput->dataset_event_callback != datasetEvent)
    {
        analoginput->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *digitalinput = &handle->digitalinput;
    digitalinput->user_context = NULL; //user defined
    digitalinput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != digitalinput->dataset_event_callback && digitalinput->dataset_event_callback != datasetEvent)
    {
        digitalinput->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *digitaloutput = &handle->digitaloutput;
    digitaloutput->user_context = NULL; //user defined
    digitaloutput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != digitaloutput->dataset_event_callback && digitaloutput->dataset_event_callback != datasetEvent)
    {
        digitaloutput->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *newanaloginput = &handle->newanaloginput;
    newanaloginput->user_context = NULL; //user defined
    newanaloginput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != newanaloginput->dataset_event_callback && newanaloginput->dataset_event_callback != datasetEvent)
    {
        newanaloginput->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *newdigitalinput = &handle->newdigitalinput;
    newdigitalinput->user_context = NULL; //user defined
    newdigitalinput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != newdigitalinput->dataset_event_callback && newdigitalinput->dataset_event_callback != datasetEvent)
    {
        newdigitalinput->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *newdigitaloutput = &handle->newdigitaloutput;
    newdigitaloutput->user_context = NULL; //user defined
    newdigitaloutput->user_tag = 0; //user defined
    //handle online download of the library
    if(NULL != newdigitaloutput->dataset_event_callback && newdigitaloutput->dataset_event_callback != datasetEvent)
    {
        newdigitaloutput->dataset_event_callback = datasetEvent;
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

        SUCCESS("starting RemoteIO application..");

        //connect the datamodel, then the datasets
        EXOS_ASSERT_OK(exos_datamodel_connect_remoteio(remoteio, datamodelEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(analoginput, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(digitalinput, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(digitaloutput, EXOS_DATASET_SUBSCRIBE, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(newanaloginput, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(newdigitalinput, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(newdigitaloutput, EXOS_DATASET_PUBLISH, datasetEvent));

        inst->Active = true;
        break;

    case 100:
    case 101:
        if (inst->Start)
        {
            if (inst->_state == 100 && remoteio->connection_state == EXOS_STATE_CONNECTED)
            {
                EXOS_ASSERT_OK(exos_datamodel_set_operational(remoteio));
                inst->_state = 101;
            }
        }
        else
        {
            inst->_state = 100;
        }

        EXOS_ASSERT_OK(exos_datamodel_process(remoteio));

        break;

    case 255:
        //disconnect the datamodel
        EXOS_ASSERT_OK(exos_datamodel_disconnect(remoteio));

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

_BUR_PUBLIC void RemoteIOExit(struct RemoteIOExit *inst)
{
    RemoteIOHandle_t *handle = (RemoteIOHandle_t *)inst->Handle;

    if (NULL == handle)
    {
        ERROR("RemoteIOExit: NULL handle, cannot delete resources");
        return;
    }
    if ((void *)handle != handle->self)
    {
        ERROR("RemoteIOExit: invalid handle, cannot delete resources");
        return;
    }

    exos_datamodel_handle_t *remoteio = &handle->remoteio;

    EXOS_ASSERT_OK(exos_datamodel_delete(remoteio));

    //finish with deleting the log
    exos_log_delete(&handle->logger);
    //free the allocated handle
    TMP_free(sizeof(RemoteIOHandle_t), (void *)handle);
}

