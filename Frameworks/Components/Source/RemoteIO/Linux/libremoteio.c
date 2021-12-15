//KNOWN ISSUES
/*
NO checks on values are made. NodeJS har as a javascript language only "numbers" that will be created from SINT, INT etc.
This means that when writing from NodeJS to Automation Runtime, you should take care of that the value actually fits into 
the value assigned.

String arrays will most probably not work, as they are basically char[][]...

Strings are encoded as utf8 strings in NodeJS which means that special chars will reduce length of string. And generate funny 
charachters in Automation Runtime.

PLCs WSTRING is not supported.

Enums defined in typ file will parse to DINT (uint32_t). Enums are not supported in JavaScript.

Generally the generates code is not yet fully and understanably error handled. ex. if (napi_ok != .....

The code generated is NOT yet fully formatted to ones normal liking. There are missing indentations.
*/

#define NAPI_VERSION 6
#include <node_api.h>
#include <stdint.h>
#include <exos_api.h>
#include <exos_log.h>
#include "exos_remoteio.h"
#include <uv.h>
#include <unistd.h>
#include <string.h>

#define SUCCESS(_format_, ...) exos_log_success(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&logger, _format_, ##__VA_ARGS__);

#define BUR_NAPI_DEFAULT_BOOL_INIT false
#define BUR_NAPI_DEFAULT_NUM_INIT 0
#define BUR_NAPI_DEFAULT_STRING_INIT ""

static exos_log_handle_t logger;

typedef struct
{
    napi_ref ref;
    uint32_t ref_count;
    napi_threadsafe_function onchange_cb;
    napi_threadsafe_function connectiononchange_cb;
    napi_threadsafe_function onprocessed_cb; //used only for datamodel
    napi_value object_value; //volatile placeholder.
    napi_value value;        //volatile placeholder.
} obj_handles;

typedef struct
{
    size_t size;
    int32_t nettime;
    void *pData;
} callback_context_t;

callback_context_t *create_callback_context(exos_dataset_handle_t *dataset)
{
    callback_context_t *context = malloc(sizeof(callback_context_t) + dataset->size);
    context->nettime = dataset->nettime;
    context->size = dataset->size;
    context->pData = (void *)((unsigned long)context + (unsigned long)sizeof(callback_context_t));
    memcpy(context->pData, dataset->data, dataset->size);
    return context;
}

obj_handles remoteio = {};
obj_handles AnalogInput = {};
obj_handles DigitalInput = {};
obj_handles DigitalOutput = {};
obj_handles NewAnalogInput = {};
obj_handles NewDigitalInput = {};
obj_handles NewDigitalOutput = {};

napi_deferred deferred = NULL;
uv_idle_t cyclic_h;

RemoteIO exos_data = {};
exos_datamodel_handle_t remoteio_datamodel;
exos_dataset_handle_t AnalogInput_dataset;
exos_dataset_handle_t DigitalInput_dataset;
exos_dataset_handle_t DigitalOutput_dataset;
exos_dataset_handle_t NewAnalogInput_dataset;
exos_dataset_handle_t NewDigitalInput_dataset;
exos_dataset_handle_t NewDigitalOutput_dataset;

// error handling (Node.js)
static void throw_fatal_exception_callbacks(napi_env env, const char *defaultCode, const char *defaultMessage)
{
    napi_value err;
    bool is_exception = false;

    napi_is_exception_pending(env, &is_exception);

    if (is_exception)
    {
        napi_get_and_clear_last_exception(env, &err);
        napi_fatal_exception(env, err);
    }
    else
    {
        napi_value code, msg;
        napi_create_string_utf8(env, defaultCode, NAPI_AUTO_LENGTH, &code);
        napi_create_string_utf8(env, defaultMessage, NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, code, msg, &err);
        napi_fatal_exception(env, err);
    }
}

// exOS callbacks
static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        if(0 == strcmp(dataset->name,"AnalogInput"))
        {
            if (AnalogInput.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(AnalogInput.onchange_cb);
                napi_call_threadsafe_function(AnalogInput.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(AnalogInput.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"DigitalInput"))
        {
            if (DigitalInput.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(DigitalInput.onchange_cb);
                napi_call_threadsafe_function(DigitalInput.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(DigitalInput.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"NewAnalogInput"))
        {
            if (NewAnalogInput.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(NewAnalogInput.onchange_cb);
                napi_call_threadsafe_function(NewAnalogInput.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(NewAnalogInput.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"NewDigitalInput"))
        {
            if (NewDigitalInput.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(NewDigitalInput.onchange_cb);
                napi_call_threadsafe_function(NewDigitalInput.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(NewDigitalInput.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"NewDigitalOutput"))
        {
            if (NewDigitalOutput.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(NewDigitalOutput.onchange_cb);
                napi_call_threadsafe_function(NewDigitalOutput.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(NewDigitalOutput.onchange_cb, napi_tsfn_release);
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published!", dataset->name);
        // fall through

    case EXOS_DATASET_EVENT_DELIVERED:
        if (event_type == EXOS_DATASET_EVENT_DELIVERED) { VERBOSE("dataset %s delivered!", dataset->name); }

        if(0 == strcmp(dataset->name, "DigitalOutput"))
        {
            //RemoteIODigital *digitaloutput = (RemoteIODigital *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        VERBOSE("dataset %s connecton changed to: %s", dataset->name, exos_get_state_string(dataset->connection_state));

        if(0 == strcmp(dataset->name, "AnalogInput"))
        {
            if (AnalogInput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(AnalogInput.connectiononchange_cb);
                napi_call_threadsafe_function(AnalogInput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(AnalogInput.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "DigitalInput"))
        {
            if (DigitalInput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(DigitalInput.connectiononchange_cb);
                napi_call_threadsafe_function(DigitalInput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(DigitalInput.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "DigitalOutput"))
        {
            if (DigitalOutput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(DigitalOutput.connectiononchange_cb);
                napi_call_threadsafe_function(DigitalOutput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(DigitalOutput.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "NewAnalogInput"))
        {
            if (NewAnalogInput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(NewAnalogInput.connectiononchange_cb);
                napi_call_threadsafe_function(NewAnalogInput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(NewAnalogInput.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "NewDigitalInput"))
        {
            if (NewDigitalInput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(NewDigitalInput.connectiononchange_cb);
                napi_call_threadsafe_function(NewDigitalInput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(NewDigitalInput.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "NewDigitalOutput"))
        {
            if (NewDigitalOutput.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(NewDigitalOutput.connectiononchange_cb);
                napi_call_threadsafe_function(NewDigitalOutput.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(NewDigitalOutput.connectiononchange_cb, napi_tsfn_release);
            }
        }

        switch (dataset->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
        case EXOS_STATE_OPERATIONAL:
        case EXOS_STATE_ABORTED:
            break;
        }
        break;
    default:
        break;

    }
}

static void datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application RemoteIO changed state to %s", exos_get_state_string(datamodel->connection_state));

        if (remoteio.connectiononchange_cb != NULL)
        {
            napi_acquire_threadsafe_function(remoteio.connectiononchange_cb);
            napi_call_threadsafe_function(remoteio.connectiononchange_cb, exos_get_state_string(datamodel->connection_state), napi_tsfn_blocking);
            napi_release_threadsafe_function(remoteio.connectiononchange_cb, napi_tsfn_release);
        }

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("RemoteIO operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("RemoteIO application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;

    }
}

// napi callback setup main function
static napi_value init_napi_onchange(napi_env env, napi_callback_info info, const char *identifier, napi_threadsafe_function_call_js call_js_cb, napi_threadsafe_function *result)
{
    size_t argc = 1;
    napi_value argv[1];

    if (napi_ok != napi_get_cb_info(env, info, &argc, argv, NULL, NULL))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_get_cb_info failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments");
        return NULL;
    }

    napi_value work_name;
    if (napi_ok != napi_create_string_utf8(env, identifier, NAPI_AUTO_LENGTH, &work_name))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_create_string_utf8 failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    napi_valuetype cb_typ;
    if (napi_ok != napi_typeof(env, argv[0], &cb_typ))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_typeof failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    if (cb_typ == napi_function)
    {
        if (napi_ok != napi_create_threadsafe_function(env, argv[0], NULL, work_name, 0, 1, NULL, NULL, NULL, call_js_cb, result))
        {
            const napi_extended_error_info *info;
            napi_get_last_error_info(env, &info);
            napi_throw_error(env, NULL, info->error_message);
            return NULL;
        }
    }
    return NULL;
}

// js object callbacks
static void remoteio_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value napi_true, napi_false, undefined;

    napi_get_undefined(env, &undefined);

    napi_get_boolean(env, true, &napi_true);
    napi_get_boolean(env, false, &napi_false);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &remoteio.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - remoteio.value");

    if (napi_ok != napi_get_reference_value(env, remoteio.ref, &remoteio.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - remoteio ");

    switch (remoteio_datamodel.connection_state)
    {
    case EXOS_STATE_DISCONNECTED:
        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        break;
    case EXOS_STATE_CONNECTED:
        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        break;
    case EXOS_STATE_OPERATIONAL:
        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isOperational", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        break;
    case EXOS_STATE_ABORTED:
        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        if (napi_ok != napi_set_named_property(env, remoteio.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

        break;
    }

    if (napi_ok != napi_set_named_property(env, remoteio.object_value, "connectionState", remoteio.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - remoteio");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - remoteio");
}

static void remoteio_onprocessed_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Error calling onProcessed - RemoteIO");
}

static void AnalogInput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &AnalogInput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - AnalogInput.value");

    if (napi_ok != napi_get_reference_value(env, AnalogInput.ref, &AnalogInput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - AnalogInput ");

    if (napi_ok != napi_set_named_property(env, AnalogInput.object_value, "connectionState", AnalogInput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - AnalogInput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - AnalogInput");
}

static void DigitalInput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &DigitalInput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - DigitalInput.value");

    if (napi_ok != napi_get_reference_value(env, DigitalInput.ref, &DigitalInput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - DigitalInput ");

    if (napi_ok != napi_set_named_property(env, DigitalInput.object_value, "connectionState", DigitalInput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - DigitalInput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - DigitalInput");
}

static void DigitalOutput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &DigitalOutput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - DigitalOutput.value");

    if (napi_ok != napi_get_reference_value(env, DigitalOutput.ref, &DigitalOutput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - DigitalOutput ");

    if (napi_ok != napi_set_named_property(env, DigitalOutput.object_value, "connectionState", DigitalOutput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - DigitalOutput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - DigitalOutput");
}

static void NewAnalogInput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &NewAnalogInput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - NewAnalogInput.value");

    if (napi_ok != napi_get_reference_value(env, NewAnalogInput.ref, &NewAnalogInput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - NewAnalogInput ");

    if (napi_ok != napi_set_named_property(env, NewAnalogInput.object_value, "connectionState", NewAnalogInput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - NewAnalogInput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - NewAnalogInput");
}

static void NewDigitalInput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &NewDigitalInput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - NewDigitalInput.value");

    if (napi_ok != napi_get_reference_value(env, NewDigitalInput.ref, &NewDigitalInput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - NewDigitalInput ");

    if (napi_ok != napi_set_named_property(env, NewDigitalInput.object_value, "connectionState", NewDigitalInput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - NewDigitalInput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - NewDigitalInput");
}

static void NewDigitalOutput_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &NewDigitalOutput.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - NewDigitalOutput.value");

    if (napi_ok != napi_get_reference_value(env, NewDigitalOutput.ref, &NewDigitalOutput.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - NewDigitalOutput ");

    if (napi_ok != napi_set_named_property(env, NewDigitalOutput.object_value, "connectionState", NewDigitalOutput.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - NewDigitalOutput");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - NewDigitalOutput");
}

// js value callbacks
static void AnalogInput_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, AnalogInput.ref, &AnalogInput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_create_string_utf8(env, (*((RemoteIOAnalog *)ctx->pData)).Name, strlen((*((RemoteIOAnalog *)ctx->pData)).Name), &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable char* to utf8 string");
    }

    napi_set_named_property(env, object0, "Name", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((RemoteIOAnalog *)ctx->pData)).State, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "State", property);
AnalogInput.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&remoteio_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, AnalogInput.object_value, "nettime", netTime);
        napi_set_named_property(env, AnalogInput.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, AnalogInput.object_value, "value", AnalogInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void DigitalInput_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, DigitalInput.ref, &DigitalInput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_create_string_utf8(env, (*((RemoteIODigital *)ctx->pData)).Name, strlen((*((RemoteIODigital *)ctx->pData)).Name), &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable char* to utf8 string");
    }

    napi_set_named_property(env, object0, "Name", property);
    if (napi_ok != napi_get_boolean(env, (*((RemoteIODigital *)ctx->pData)).State, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "State", property);
DigitalInput.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&remoteio_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, DigitalInput.object_value, "nettime", netTime);
        napi_set_named_property(env, DigitalInput.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, DigitalInput.object_value, "value", DigitalInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void NewAnalogInput_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, NewAnalogInput.ref, &NewAnalogInput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_create_string_utf8(env, ((char *)ctx->pData), strlen(((char *)ctx->pData)), &NewAnalogInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable char* to utf8 string");
    }

        int32_t _latency = exos_datamodel_get_nettime(&remoteio_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, NewAnalogInput.object_value, "nettime", netTime);
        napi_set_named_property(env, NewAnalogInput.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, NewAnalogInput.object_value, "value", NewAnalogInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void NewDigitalInput_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, NewDigitalInput.ref, &NewDigitalInput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_create_string_utf8(env, ((char *)ctx->pData), strlen(((char *)ctx->pData)), &NewDigitalInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable char* to utf8 string");
    }

        int32_t _latency = exos_datamodel_get_nettime(&remoteio_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, NewDigitalInput.object_value, "nettime", netTime);
        napi_set_named_property(env, NewDigitalInput.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, NewDigitalInput.object_value, "value", NewDigitalInput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void NewDigitalOutput_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, NewDigitalOutput.ref, &NewDigitalOutput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_create_string_utf8(env, ((char *)ctx->pData), strlen(((char *)ctx->pData)), &NewDigitalOutput.value))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable char* to utf8 string");
    }

        int32_t _latency = exos_datamodel_get_nettime(&remoteio_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, NewDigitalOutput.object_value, "nettime", netTime);
        napi_set_named_property(env, NewDigitalOutput.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, NewDigitalOutput.object_value, "value", NewDigitalOutput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

// js callback inits
static napi_value remoteio_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "RemoteIO connection change", remoteio_connonchange_js_cb, &remoteio.connectiononchange_cb);
}

static napi_value remoteio_onprocessed_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "RemoteIO onProcessed", remoteio_onprocessed_js_cb, &remoteio.onprocessed_cb);
}

static napi_value AnalogInput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "AnalogInput connection change", AnalogInput_connonchange_js_cb, &AnalogInput.connectiononchange_cb);
}

static napi_value DigitalInput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "DigitalInput connection change", DigitalInput_connonchange_js_cb, &DigitalInput.connectiononchange_cb);
}

static napi_value DigitalOutput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "DigitalOutput connection change", DigitalOutput_connonchange_js_cb, &DigitalOutput.connectiononchange_cb);
}

static napi_value NewAnalogInput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewAnalogInput connection change", NewAnalogInput_connonchange_js_cb, &NewAnalogInput.connectiononchange_cb);
}

static napi_value NewDigitalInput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewDigitalInput connection change", NewDigitalInput_connonchange_js_cb, &NewDigitalInput.connectiononchange_cb);
}

static napi_value NewDigitalOutput_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewDigitalOutput connection change", NewDigitalOutput_connonchange_js_cb, &NewDigitalOutput.connectiononchange_cb);
}

static napi_value AnalogInput_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "AnalogInput dataset change", AnalogInput_onchange_js_cb, &AnalogInput.onchange_cb);
}

static napi_value DigitalInput_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "DigitalInput dataset change", DigitalInput_onchange_js_cb, &DigitalInput.onchange_cb);
}

static napi_value NewAnalogInput_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewAnalogInput dataset change", NewAnalogInput_onchange_js_cb, &NewAnalogInput.onchange_cb);
}

static napi_value NewDigitalInput_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewDigitalInput dataset change", NewDigitalInput_onchange_js_cb, &NewDigitalInput.onchange_cb);
}

static napi_value NewDigitalOutput_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "NewDigitalOutput dataset change", NewDigitalOutput_onchange_js_cb, &NewDigitalOutput.onchange_cb);
}

// publish methods
static napi_value DigitalOutput_publish_method(napi_env env, napi_callback_info info)
{
    napi_value object0, object1;
    size_t _r;

    if (napi_ok != napi_get_reference_value(env, DigitalOutput.ref, &DigitalOutput.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, DigitalOutput.object_value, "value", &DigitalOutput.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    object0 = DigitalOutput.value;
    napi_get_named_property(env, object0, "Name", &object1);
    if (napi_ok != napi_get_value_string_utf8(env, object1, (char *)&exos_data.DigitalOutput.Name, sizeof(exos_data.DigitalOutput.Name), &_r))
    {
        napi_throw_error(env, "EINVAL", "Expected string");
        return NULL;
    }
    napi_get_named_property(env, object0, "State", &object1);
    if (napi_ok != napi_get_value_bool(env, object1, &exos_data.DigitalOutput.State))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    exos_dataset_publish(&DigitalOutput_dataset);
    return NULL;
}

//logging functions
static napi_value log_error(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.error()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.error()");
        return NULL;
    }

    exos_log_error(&logger, log_entry);
    return NULL;
}

static napi_value log_warning(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.warning()");
        return  NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.warning()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_success(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.success()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.success()");
        return NULL;
    }

    exos_log_success(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_info(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.info()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.info()");
        return NULL;
    }

    exos_log_info(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_debug(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.debug()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.debug()");
        return NULL;
    }

    exos_log_debug(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_verbose(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for remoteio.log.verbose()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for remoteio.log.verbose()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
    return NULL;
}

// cleanup/cyclic
static void cleanup_remoteio(void *env)
{
    uv_idle_stop(&cyclic_h);

    if (EXOS_ERROR_OK != exos_datamodel_delete(&remoteio_datamodel))
    {
        napi_throw_error(env, "EINVAL", "Can't delete datamodel");
    }

    if (EXOS_ERROR_OK != exos_log_delete(&logger))
    {
        napi_throw_error(env, "EINVAL", "Can't delete logger");
    }
}

static void cyclic(uv_idle_t * handle) 
{
    int dummy = 0;
    exos_datamodel_process(&remoteio_datamodel);
    napi_acquire_threadsafe_function(remoteio.onprocessed_cb);
    napi_call_threadsafe_function(remoteio.onprocessed_cb, &dummy, napi_tsfn_blocking);
    napi_release_threadsafe_function(remoteio.onprocessed_cb, napi_tsfn_release);
    exos_log_process(&logger);
}

//read nettime for DataModel
static napi_value get_net_time(napi_env env, napi_callback_info info)
{
    napi_value netTime;

    if (napi_ok == napi_create_int32(env, exos_datamodel_get_nettime(&remoteio_datamodel), &netTime))
    {
        return netTime;
    }
    else
    {
        return NULL;
    }
}

// init of module, called at "require"
static napi_value init_remoteio(napi_env env, napi_value exports)
{
    napi_value remoteio_conn_change, remoteio_onprocessed, AnalogInput_conn_change, DigitalInput_conn_change, DigitalOutput_conn_change, NewAnalogInput_conn_change, NewDigitalInput_conn_change, NewDigitalOutput_conn_change;
    napi_value AnalogInput_onchange, DigitalInput_onchange, NewAnalogInput_onchange, NewDigitalInput_onchange, NewDigitalOutput_onchange;
    napi_value DigitalOutput_publish;
    napi_value AnalogInput_value, DigitalInput_value, DigitalOutput_value, NewAnalogInput_value, NewDigitalInput_value, NewDigitalOutput_value;

    napi_value dataModel, getNetTime, undefined, def_bool, def_number, def_string;
    napi_value log, logError, logWarning, logSuccess, logInfo, logDebug, logVerbose;
    napi_value object0;

    napi_get_boolean(env, BUR_NAPI_DEFAULT_BOOL_INIT, &def_bool); 
    napi_create_int32(env, BUR_NAPI_DEFAULT_NUM_INIT, &def_number); 
    napi_create_string_utf8(env, BUR_NAPI_DEFAULT_STRING_INIT, strlen(BUR_NAPI_DEFAULT_STRING_INIT), &def_string);
    napi_get_undefined(env, &undefined); 

    // create base objects
    if (napi_ok != napi_create_object(env, &dataModel)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &log)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &remoteio.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &AnalogInput.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &DigitalInput.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &DigitalOutput.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &NewAnalogInput.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &NewDigitalInput.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &NewDigitalOutput.value)) 
        return NULL; 

    // build object structures
    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "Name", def_string);
    napi_set_named_property(env, object0, "State", def_number);
    AnalogInput_value = object0;
    napi_create_function(env, NULL, 0, AnalogInput_onchange_init, NULL, &AnalogInput_onchange);
    napi_set_named_property(env, AnalogInput.value, "onChange", AnalogInput_onchange);
    napi_set_named_property(env, AnalogInput.value, "nettime", undefined);
    napi_set_named_property(env, AnalogInput.value, "latency", undefined);
    napi_set_named_property(env, AnalogInput.value, "value", AnalogInput_value);
    napi_create_function(env, NULL, 0, AnalogInput_connonchange_init, NULL, &AnalogInput_conn_change);
    napi_set_named_property(env, AnalogInput.value, "onConnectionChange", AnalogInput_conn_change);
    napi_set_named_property(env, AnalogInput.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "Name", def_string);
    napi_set_named_property(env, object0, "State", def_bool);
    DigitalInput_value = object0;
    napi_create_function(env, NULL, 0, DigitalInput_onchange_init, NULL, &DigitalInput_onchange);
    napi_set_named_property(env, DigitalInput.value, "onChange", DigitalInput_onchange);
    napi_set_named_property(env, DigitalInput.value, "nettime", undefined);
    napi_set_named_property(env, DigitalInput.value, "latency", undefined);
    napi_set_named_property(env, DigitalInput.value, "value", DigitalInput_value);
    napi_create_function(env, NULL, 0, DigitalInput_connonchange_init, NULL, &DigitalInput_conn_change);
    napi_set_named_property(env, DigitalInput.value, "onConnectionChange", DigitalInput_conn_change);
    napi_set_named_property(env, DigitalInput.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "Name", def_string);
    napi_set_named_property(env, object0, "State", def_bool);
    DigitalOutput_value = object0;
    napi_create_function(env, NULL, 0, DigitalOutput_publish_method, NULL, &DigitalOutput_publish);
    napi_set_named_property(env, DigitalOutput.value, "publish", DigitalOutput_publish);
    napi_set_named_property(env, DigitalOutput.value, "value", DigitalOutput_value);
    napi_create_function(env, NULL, 0, DigitalOutput_connonchange_init, NULL, &DigitalOutput_conn_change);
    napi_set_named_property(env, DigitalOutput.value, "onConnectionChange", DigitalOutput_conn_change);
    napi_set_named_property(env, DigitalOutput.value, "connectionState", def_string);

NewAnalogInput_value = def_string;
    napi_create_function(env, NULL, 0, NewAnalogInput_onchange_init, NULL, &NewAnalogInput_onchange);
    napi_set_named_property(env, NewAnalogInput.value, "onChange", NewAnalogInput_onchange);
    napi_set_named_property(env, NewAnalogInput.value, "nettime", undefined);
    napi_set_named_property(env, NewAnalogInput.value, "latency", undefined);
    napi_set_named_property(env, NewAnalogInput.value, "value", NewAnalogInput_value);
    napi_create_function(env, NULL, 0, NewAnalogInput_connonchange_init, NULL, &NewAnalogInput_conn_change);
    napi_set_named_property(env, NewAnalogInput.value, "onConnectionChange", NewAnalogInput_conn_change);
    napi_set_named_property(env, NewAnalogInput.value, "connectionState", def_string);

NewDigitalInput_value = def_string;
    napi_create_function(env, NULL, 0, NewDigitalInput_onchange_init, NULL, &NewDigitalInput_onchange);
    napi_set_named_property(env, NewDigitalInput.value, "onChange", NewDigitalInput_onchange);
    napi_set_named_property(env, NewDigitalInput.value, "nettime", undefined);
    napi_set_named_property(env, NewDigitalInput.value, "latency", undefined);
    napi_set_named_property(env, NewDigitalInput.value, "value", NewDigitalInput_value);
    napi_create_function(env, NULL, 0, NewDigitalInput_connonchange_init, NULL, &NewDigitalInput_conn_change);
    napi_set_named_property(env, NewDigitalInput.value, "onConnectionChange", NewDigitalInput_conn_change);
    napi_set_named_property(env, NewDigitalInput.value, "connectionState", def_string);

NewDigitalOutput_value = def_string;
    napi_create_function(env, NULL, 0, NewDigitalOutput_onchange_init, NULL, &NewDigitalOutput_onchange);
    napi_set_named_property(env, NewDigitalOutput.value, "onChange", NewDigitalOutput_onchange);
    napi_set_named_property(env, NewDigitalOutput.value, "nettime", undefined);
    napi_set_named_property(env, NewDigitalOutput.value, "latency", undefined);
    napi_set_named_property(env, NewDigitalOutput.value, "value", NewDigitalOutput_value);
    napi_create_function(env, NULL, 0, NewDigitalOutput_connonchange_init, NULL, &NewDigitalOutput_conn_change);
    napi_set_named_property(env, NewDigitalOutput.value, "onConnectionChange", NewDigitalOutput_conn_change);
    napi_set_named_property(env, NewDigitalOutput.value, "connectionState", def_string);

    //connect logging functions
    napi_create_function(env, NULL, 0, log_error, NULL, &logError);
    napi_set_named_property(env, log, "error", logError);
    napi_create_function(env, NULL, 0, log_warning, NULL, &logWarning);
    napi_set_named_property(env, log, "warning", logWarning);
    napi_create_function(env, NULL, 0, log_success, NULL, &logSuccess);
    napi_set_named_property(env, log, "success", logSuccess);
    napi_create_function(env, NULL, 0, log_info, NULL, &logInfo);
    napi_set_named_property(env, log, "info", logInfo);
    napi_create_function(env, NULL, 0, log_debug, NULL, &logDebug);
    napi_set_named_property(env, log, "debug", logDebug);
    napi_create_function(env, NULL, 0, log_verbose, NULL, &logVerbose);
    napi_set_named_property(env, log, "verbose", logVerbose);

    // bind dataset objects to datamodel object
    napi_set_named_property(env, dataModel, "AnalogInput", AnalogInput.value); 
    napi_set_named_property(env, dataModel, "DigitalInput", DigitalInput.value); 
    napi_set_named_property(env, dataModel, "DigitalOutput", DigitalOutput.value); 
    napi_set_named_property(env, dataModel, "NewAnalogInput", NewAnalogInput.value); 
    napi_set_named_property(env, dataModel, "NewDigitalInput", NewDigitalInput.value); 
    napi_set_named_property(env, dataModel, "NewDigitalOutput", NewDigitalOutput.value); 
    napi_set_named_property(env, remoteio.value, "datamodel", dataModel); 
    napi_create_function(env, NULL, 0, remoteio_connonchange_init, NULL, &remoteio_conn_change); 
    napi_set_named_property(env, remoteio.value, "onConnectionChange", remoteio_conn_change); 
    napi_set_named_property(env, remoteio.value, "connectionState", def_string);
    napi_set_named_property(env, remoteio.value, "isConnected", def_bool);
    napi_set_named_property(env, remoteio.value, "isOperational", def_bool);
    napi_create_function(env, NULL, 0, remoteio_onprocessed_init, NULL, &remoteio_onprocessed); 
    napi_set_named_property(env, remoteio.value, "onProcessed", remoteio_onprocessed); 
    napi_create_function(env, NULL, 0, get_net_time, NULL, &getNetTime);
    napi_set_named_property(env, remoteio.value, "nettime", getNetTime);
    napi_set_named_property(env, remoteio.value, "log", log);
    // export application object
    napi_set_named_property(env, exports, "RemoteIO", remoteio.value); 

    // save references to object as globals for this C-file
    if (napi_ok != napi_create_reference(env, remoteio.value, remoteio.ref_count, &remoteio.ref)) 
    {
                    
        napi_throw_error(env, "EINVAL", "Can't create remoteio reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, AnalogInput.value, AnalogInput.ref_count, &AnalogInput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create AnalogInput reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, DigitalInput.value, DigitalInput.ref_count, &DigitalInput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create DigitalInput reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, DigitalOutput.value, DigitalOutput.ref_count, &DigitalOutput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create DigitalOutput reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, NewAnalogInput.value, NewAnalogInput.ref_count, &NewAnalogInput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create NewAnalogInput reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, NewDigitalInput.value, NewDigitalInput.ref_count, &NewDigitalInput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create NewDigitalInput reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, NewDigitalOutput.value, NewDigitalOutput.ref_count, &NewDigitalOutput.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create NewDigitalOutput reference"); 
        return NULL; 
    } 

    // register clean up hook
    if (napi_ok != napi_add_env_cleanup_hook(env, cleanup_remoteio, env)) 
    {
        napi_throw_error(env, "EINVAL", "Can't register cleanup hook"); 
        return NULL; 
    } 

    // exOS
    // exOS inits
    if (EXOS_ERROR_OK != exos_datamodel_init(&remoteio_datamodel, "RemoteIO_0", "gRemoteIO_0")) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize RemoteIO"); 
    } 
    remoteio_datamodel.user_context = NULL; 
    remoteio_datamodel.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&AnalogInput_dataset, &remoteio_datamodel, "AnalogInput", &exos_data.AnalogInput, sizeof(exos_data.AnalogInput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize AnalogInput"); 
    }
    AnalogInput_dataset.user_context = NULL; 
    AnalogInput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&DigitalInput_dataset, &remoteio_datamodel, "DigitalInput", &exos_data.DigitalInput, sizeof(exos_data.DigitalInput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize DigitalInput"); 
    }
    DigitalInput_dataset.user_context = NULL; 
    DigitalInput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&DigitalOutput_dataset, &remoteio_datamodel, "DigitalOutput", &exos_data.DigitalOutput, sizeof(exos_data.DigitalOutput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize DigitalOutput"); 
    }
    DigitalOutput_dataset.user_context = NULL; 
    DigitalOutput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&NewAnalogInput_dataset, &remoteio_datamodel, "NewAnalogInput", &exos_data.NewAnalogInput, sizeof(exos_data.NewAnalogInput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize NewAnalogInput"); 
    }
    NewAnalogInput_dataset.user_context = NULL; 
    NewAnalogInput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&NewDigitalInput_dataset, &remoteio_datamodel, "NewDigitalInput", &exos_data.NewDigitalInput, sizeof(exos_data.NewDigitalInput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize NewDigitalInput"); 
    }
    NewDigitalInput_dataset.user_context = NULL; 
    NewDigitalInput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&NewDigitalOutput_dataset, &remoteio_datamodel, "NewDigitalOutput", &exos_data.NewDigitalOutput, sizeof(exos_data.NewDigitalOutput))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize NewDigitalOutput"); 
    }
    NewDigitalOutput_dataset.user_context = NULL; 
    NewDigitalOutput_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_log_init(&logger, "RemoteIO_0"))
    {
        napi_throw_error(env, "EINVAL", "Can't register logger for RemoteIO"); 
    } 

    INFO("RemoteIO starting!")
    // exOS register datamodel
    if (EXOS_ERROR_OK != exos_datamodel_connect_remoteio(&remoteio_datamodel, datamodelEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect RemoteIO"); 
    } 

    // exOS register datasets
    if (EXOS_ERROR_OK != exos_dataset_connect(&AnalogInput_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect AnalogInput"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&DigitalInput_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect DigitalInput"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&DigitalOutput_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect DigitalOutput"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&NewAnalogInput_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect NewAnalogInput"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&NewDigitalInput_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect NewDigitalInput"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&NewDigitalOutput_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect NewDigitalOutput"); 
    }

    // start up module

    uv_idle_init(uv_default_loop(), &cyclic_h); 
    uv_idle_start(&cyclic_h, cyclic); 

    SUCCESS("RemoteIO started!")
    return exports; 
} 

// hook for Node-API
NAPI_MODULE(NODE_GYP_MODULE_NAME, init_remoteio);
