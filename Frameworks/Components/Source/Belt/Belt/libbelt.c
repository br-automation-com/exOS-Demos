#include <string.h>
#define EXOS_ASSERT_LOG &logger
#include "exos_log.h"
#include "libbelt.h"

#define SUCCESS(_format_, ...) exos_log_success(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&logger, _format_, ##__VA_ARGS__);

static exos_log_handle_t logger;

typedef struct libBeltHandle
{
    libBelt_t ext_belt;
    exos_datamodel_handle_t belt;

    exos_dataset_handle_t status;
    exos_dataset_handle_t actuator;
    exos_dataset_handle_t framework;
} libBeltHandle_t;

static libBeltHandle_t h_Belt;

static void libBelt_datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        //handle each subscription dataset separately
        if (0 == strcmp(dataset->name, "Actuator"))
        {
            //update the nettime
            h_Belt.ext_belt.Actuator.nettime = dataset->nettime;

            //trigger the callback if assigned
            if (NULL != h_Belt.ext_belt.Actuator.on_change)
            {
                h_Belt.ext_belt.Actuator.on_change();
            }
        }
        else if (0 == strcmp(dataset->name, "Framework"))
        {
            //update the nettime
            h_Belt.ext_belt.Framework.nettime = dataset->nettime;

            //trigger the callback if assigned
            if (NULL != h_Belt.ext_belt.Framework.on_change)
            {
                h_Belt.ext_belt.Framework.on_change();
            }
        }
        break;

    default:
        break;
    }
}

static void libBelt_datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application changed state to %s", exos_get_state_string(datamodel->connection_state));

        h_Belt.ext_belt.is_connected = false;
        h_Belt.ext_belt.is_operational = false;
        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            if (NULL != h_Belt.ext_belt.on_disconnected)
            {
                h_Belt.ext_belt.on_disconnected();
            }
            break;
        case EXOS_STATE_CONNECTED:
            h_Belt.ext_belt.is_connected = true;
            if (NULL != h_Belt.ext_belt.on_connected)
            {
                h_Belt.ext_belt.on_connected();
            }
            break;
        case EXOS_STATE_OPERATIONAL:
            h_Belt.ext_belt.is_connected = true;
            h_Belt.ext_belt.is_operational = true;
            if (NULL != h_Belt.ext_belt.on_operational)
            {
                h_Belt.ext_belt.on_operational();
            }
            SUCCESS("Belt operational!");
            break;
        case EXOS_STATE_ABORTED:
            if (NULL != h_Belt.ext_belt.on_disconnected)
            {
                h_Belt.ext_belt.on_disconnected();
            }
            ERROR("application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;

    }
}

static void libBelt_publish_status(void)
{
    exos_dataset_publish(&h_Belt.status);
}

static void libBelt_connect(void)
{
    //connect the datamodel
    EXOS_ASSERT_OK(exos_datamodel_connect_belt(&(h_Belt.belt), libBelt_datamodelEvent));
    
    //connect datasets
    EXOS_ASSERT_OK(exos_dataset_connect(&(h_Belt.status), EXOS_DATASET_PUBLISH, libBelt_datasetEvent));
    EXOS_ASSERT_OK(exos_dataset_connect(&(h_Belt.actuator), EXOS_DATASET_SUBSCRIBE, libBelt_datasetEvent));
    EXOS_ASSERT_OK(exos_dataset_connect(&(h_Belt.framework), EXOS_DATASET_SUBSCRIBE, libBelt_datasetEvent));
}
static void libBelt_disconnect(void)
{
    h_Belt.ext_belt.is_connected = false;
    h_Belt.ext_belt.is_operational = false;

    EXOS_ASSERT_OK(exos_datamodel_disconnect(&(h_Belt.belt)));
}

static void libBelt_set_operational(void)
{
    EXOS_ASSERT_OK(exos_datamodel_set_operational(&(h_Belt.belt)));
}

static void libBelt_process(void)
{
    EXOS_ASSERT_OK(exos_datamodel_process(&(h_Belt.belt)));
    exos_log_process(&logger);
}

static void libBelt_dispose(void)
{
    h_Belt.ext_belt.is_connected = false;
    h_Belt.ext_belt.is_operational = false;

    EXOS_ASSERT_OK(exos_datamodel_delete(&(h_Belt.belt)));
    exos_log_delete(&logger);
}

static int32_t libBelt_get_nettime(void)
{
    return exos_datamodel_get_nettime(&(h_Belt.belt));
}

static void libBelt_log_error(char* log_entry)
{
    exos_log_error(&logger, log_entry);
}

static void libBelt_log_warning(char* log_entry)
{
    exos_log_warning(&logger, EXOS_LOG_TYPE_USER, log_entry);
}

static void libBelt_log_success(char* log_entry)
{
    exos_log_success(&logger, EXOS_LOG_TYPE_USER, log_entry);
}

static void libBelt_log_info(char* log_entry)
{
    exos_log_info(&logger, EXOS_LOG_TYPE_USER, log_entry);
}

static void libBelt_log_debug(char* log_entry)
{
    exos_log_debug(&logger, EXOS_LOG_TYPE_USER, log_entry);
}

static void libBelt_log_verbose(char* log_entry)
{
    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
}

libBelt_t *libBelt_init(void)
{
    memset(&h_Belt, 0, sizeof(h_Belt));

    h_Belt.ext_belt.Status.publish = libBelt_publish_status;
    
    h_Belt.ext_belt.connect = libBelt_connect;
    h_Belt.ext_belt.disconnect = libBelt_disconnect;
    h_Belt.ext_belt.process = libBelt_process;
    h_Belt.ext_belt.set_operational = libBelt_set_operational;
    h_Belt.ext_belt.dispose = libBelt_dispose;
    h_Belt.ext_belt.get_nettime = libBelt_get_nettime;
    h_Belt.ext_belt.log.error = libBelt_log_error;
    h_Belt.ext_belt.log.warning = libBelt_log_warning;
    h_Belt.ext_belt.log.success = libBelt_log_success;
    h_Belt.ext_belt.log.info = libBelt_log_info;
    h_Belt.ext_belt.log.debug = libBelt_log_debug;
    h_Belt.ext_belt.log.verbose = libBelt_log_verbose;
    
    exos_log_init(&logger, "gBelt_0");

    SUCCESS("starting gBelt_0 application..");

    EXOS_ASSERT_OK(exos_datamodel_init(&h_Belt.belt, "Belt_0", "gBelt_0"));

    //set the user_context to access custom data in the callbacks
    h_Belt.belt.user_context = NULL; //not used
    h_Belt.belt.user_tag = 0; //not used

    EXOS_ASSERT_OK(exos_dataset_init(&h_Belt.status, &h_Belt.belt, "Status", &h_Belt.ext_belt.Status.value, sizeof(h_Belt.ext_belt.Status.value)));
    h_Belt.status.user_context = NULL; //not used
    h_Belt.status.user_tag = 0; //not used

    EXOS_ASSERT_OK(exos_dataset_init(&h_Belt.actuator, &h_Belt.belt, "Actuator", &h_Belt.ext_belt.Actuator.value, sizeof(h_Belt.ext_belt.Actuator.value)));
    h_Belt.actuator.user_context = NULL; //not used
    h_Belt.actuator.user_tag = 0; //not used

    EXOS_ASSERT_OK(exos_dataset_init(&h_Belt.framework, &h_Belt.belt, "Framework", &h_Belt.ext_belt.Framework.value, sizeof(h_Belt.ext_belt.Framework.value)));
    h_Belt.framework.user_context = NULL; //not used
    h_Belt.framework.user_tag = 0; //not used

    return &(h_Belt.ext_belt);
}
