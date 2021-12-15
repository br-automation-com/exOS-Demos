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
#include "exos_belt.h"
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

obj_handles belt = {};
obj_handles Status = {};
obj_handles Actuator = {};
obj_handles Framework = {};

napi_deferred deferred = NULL;
uv_idle_t cyclic_h;

Belt exos_data = {};
exos_datamodel_handle_t belt_datamodel;
exos_dataset_handle_t Status_dataset;
exos_dataset_handle_t Actuator_dataset;
exos_dataset_handle_t Framework_dataset;

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
        if(0 == strcmp(dataset->name,"Status"))
        {
            if (Status.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Status.onchange_cb);
                napi_call_threadsafe_function(Status.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Status.onchange_cb, napi_tsfn_release);
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published!", dataset->name);
        // fall through

    case EXOS_DATASET_EVENT_DELIVERED:
        if (event_type == EXOS_DATASET_EVENT_DELIVERED) { VERBOSE("dataset %s delivered!", dataset->name); }

        if(0 == strcmp(dataset->name, "Actuator"))
        {
            //BeltActuator *actuator = (BeltActuator *)dataset->data;
        }
        else if(0 == strcmp(dataset->name, "Framework"))
        {
            //bool *framework = (bool *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        VERBOSE("dataset %s connecton changed to: %s", dataset->name, exos_get_state_string(dataset->connection_state));

        if(0 == strcmp(dataset->name, "Status"))
        {
            if (Status.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Status.connectiononchange_cb);
                napi_call_threadsafe_function(Status.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Status.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Actuator"))
        {
            if (Actuator.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Actuator.connectiononchange_cb);
                napi_call_threadsafe_function(Actuator.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Actuator.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Framework"))
        {
            if (Framework.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Framework.connectiononchange_cb);
                napi_call_threadsafe_function(Framework.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Framework.connectiononchange_cb, napi_tsfn_release);
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
        INFO("application Belt changed state to %s", exos_get_state_string(datamodel->connection_state));

        if (belt.connectiononchange_cb != NULL)
        {
            napi_acquire_threadsafe_function(belt.connectiononchange_cb);
            napi_call_threadsafe_function(belt.connectiononchange_cb, exos_get_state_string(datamodel->connection_state), napi_tsfn_blocking);
            napi_release_threadsafe_function(belt.connectiononchange_cb, napi_tsfn_release);
        }

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("Belt operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("Belt application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
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
static void belt_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value napi_true, napi_false, undefined;

    napi_get_undefined(env, &undefined);

    napi_get_boolean(env, true, &napi_true);
    napi_get_boolean(env, false, &napi_false);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &belt.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - belt.value");

    if (napi_ok != napi_get_reference_value(env, belt.ref, &belt.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - belt ");

    switch (belt_datamodel.connection_state)
    {
    case EXOS_STATE_DISCONNECTED:
        if (napi_ok != napi_set_named_property(env, belt.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        if (napi_ok != napi_set_named_property(env, belt.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        break;
    case EXOS_STATE_CONNECTED:
        if (napi_ok != napi_set_named_property(env, belt.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        if (napi_ok != napi_set_named_property(env, belt.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        break;
    case EXOS_STATE_OPERATIONAL:
        if (napi_ok != napi_set_named_property(env, belt.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        if (napi_ok != napi_set_named_property(env, belt.object_value, "isOperational", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        break;
    case EXOS_STATE_ABORTED:
        if (napi_ok != napi_set_named_property(env, belt.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        if (napi_ok != napi_set_named_property(env, belt.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

        break;
    }

    if (napi_ok != napi_set_named_property(env, belt.object_value, "connectionState", belt.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - belt");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - belt");
}

static void belt_onprocessed_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Error calling onProcessed - Belt");
}

static void Status_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Status.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Status.value");

    if (napi_ok != napi_get_reference_value(env, Status.ref, &Status.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Status ");

    if (napi_ok != napi_set_named_property(env, Status.object_value, "connectionState", Status.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Status");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Status");
}

static void Actuator_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Actuator.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Actuator.value");

    if (napi_ok != napi_get_reference_value(env, Actuator.ref, &Actuator.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Actuator ");

    if (napi_ok != napi_set_named_property(env, Actuator.object_value, "connectionState", Actuator.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Actuator");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Actuator");
}

static void Framework_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Framework.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Framework.value");

    if (napi_ok != napi_get_reference_value(env, Framework.ref, &Framework.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Framework ");

    if (napi_ok != napi_set_named_property(env, Framework.object_value, "connectionState", Framework.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Framework");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Framework");
}

// js value callbacks
static void Status_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Status.ref, &Status.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_get_boolean(env, (*((BeltStatus *)ctx->pData)).Running, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "Running", property);
    if (napi_ok != napi_get_boolean(env, (*((BeltStatus *)ctx->pData)).Latched, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "Latched", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((BeltStatus *)ctx->pData)).LatchIndex, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "LatchIndex", property);
    if (napi_ok != napi_get_boolean(env, (*((BeltStatus *)ctx->pData)).Actuator, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "Actuator", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((BeltStatus *)ctx->pData)).ActualSpeed, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "ActualSpeed", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((BeltStatus *)ctx->pData)).ActualPosition, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "ActualPosition", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((BeltStatus *)ctx->pData)).LatchedPosition, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "LatchedPosition", property);
Status.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&belt_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Status.object_value, "nettime", netTime);
        napi_set_named_property(env, Status.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Status.object_value, "value", Status.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

// js callback inits
static napi_value belt_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Belt connection change", belt_connonchange_js_cb, &belt.connectiononchange_cb);
}

static napi_value belt_onprocessed_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Belt onProcessed", belt_onprocessed_js_cb, &belt.onprocessed_cb);
}

static napi_value Status_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Status connection change", Status_connonchange_js_cb, &Status.connectiononchange_cb);
}

static napi_value Actuator_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Actuator connection change", Actuator_connonchange_js_cb, &Actuator.connectiononchange_cb);
}

static napi_value Framework_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Framework connection change", Framework_connonchange_js_cb, &Framework.connectiononchange_cb);
}

static napi_value Status_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Status dataset change", Status_onchange_js_cb, &Status.onchange_cb);
}

// publish methods
static napi_value Actuator_publish_method(napi_env env, napi_callback_info info)
{
    napi_value object0, object1;
    int32_t _value;

    if (napi_ok != napi_get_reference_value(env, Actuator.ref, &Actuator.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, Actuator.object_value, "value", &Actuator.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    object0 = Actuator.value;
    napi_get_named_property(env, object0, "On", &object1);
    if (napi_ok != napi_get_value_int32(env, object1, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Actuator.On = (int32_t)_value;
    napi_get_named_property(env, object0, "Off", &object1);
    if (napi_ok != napi_get_value_int32(env, object1, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Actuator.Off = (int32_t)_value;
    napi_get_named_property(env, object0, "LatchIndex", &object1);
    if (napi_ok != napi_get_value_int32(env, object1, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Actuator.LatchIndex = (int32_t)_value;
    exos_dataset_publish(&Actuator_dataset);
    return NULL;
}

static napi_value Framework_publish_method(napi_env env, napi_callback_info info)
{

    if (napi_ok != napi_get_reference_value(env, Framework.ref, &Framework.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, Framework.object_value, "value", &Framework.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    if (napi_ok != napi_get_value_bool(env, Framework.value, &exos_data.Framework))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }

    exos_dataset_publish(&Framework_dataset);
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.error()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.error()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.warning()");
        return  NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.warning()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.success()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.success()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.info()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.info()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.debug()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.debug()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for belt.log.verbose()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for belt.log.verbose()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
    return NULL;
}

// cleanup/cyclic
static void cleanup_belt(void *env)
{
    uv_idle_stop(&cyclic_h);

    if (EXOS_ERROR_OK != exos_datamodel_delete(&belt_datamodel))
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
    exos_datamodel_process(&belt_datamodel);
    napi_acquire_threadsafe_function(belt.onprocessed_cb);
    napi_call_threadsafe_function(belt.onprocessed_cb, &dummy, napi_tsfn_blocking);
    napi_release_threadsafe_function(belt.onprocessed_cb, napi_tsfn_release);
    exos_log_process(&logger);
}

//read nettime for DataModel
static napi_value get_net_time(napi_env env, napi_callback_info info)
{
    napi_value netTime;

    if (napi_ok == napi_create_int32(env, exos_datamodel_get_nettime(&belt_datamodel), &netTime))
    {
        return netTime;
    }
    else
    {
        return NULL;
    }
}

// init of module, called at "require"
static napi_value init_belt(napi_env env, napi_value exports)
{
    napi_value belt_conn_change, belt_onprocessed, Status_conn_change, Actuator_conn_change, Framework_conn_change;
    napi_value Status_onchange;
    napi_value Actuator_publish, Framework_publish;
    napi_value Status_value, Actuator_value, Framework_value;

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

    if (napi_ok != napi_create_object(env, &belt.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Status.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Actuator.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Framework.value)) 
        return NULL; 

    // build object structures
    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "Running", def_bool);
    napi_set_named_property(env, object0, "Latched", def_bool);
    napi_set_named_property(env, object0, "LatchIndex", def_number);
    napi_set_named_property(env, object0, "Actuator", def_bool);
    napi_set_named_property(env, object0, "ActualSpeed", def_number);
    napi_set_named_property(env, object0, "ActualPosition", def_number);
    napi_set_named_property(env, object0, "LatchedPosition", def_number);
    Status_value = object0;
    napi_create_function(env, NULL, 0, Status_onchange_init, NULL, &Status_onchange);
    napi_set_named_property(env, Status.value, "onChange", Status_onchange);
    napi_set_named_property(env, Status.value, "nettime", undefined);
    napi_set_named_property(env, Status.value, "latency", undefined);
    napi_set_named_property(env, Status.value, "value", Status_value);
    napi_create_function(env, NULL, 0, Status_connonchange_init, NULL, &Status_conn_change);
    napi_set_named_property(env, Status.value, "onConnectionChange", Status_conn_change);
    napi_set_named_property(env, Status.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "On", def_number);
    napi_set_named_property(env, object0, "Off", def_number);
    napi_set_named_property(env, object0, "LatchIndex", def_number);
    Actuator_value = object0;
    napi_create_function(env, NULL, 0, Actuator_publish_method, NULL, &Actuator_publish);
    napi_set_named_property(env, Actuator.value, "publish", Actuator_publish);
    napi_set_named_property(env, Actuator.value, "value", Actuator_value);
    napi_create_function(env, NULL, 0, Actuator_connonchange_init, NULL, &Actuator_conn_change);
    napi_set_named_property(env, Actuator.value, "onConnectionChange", Actuator_conn_change);
    napi_set_named_property(env, Actuator.value, "connectionState", def_string);

Framework_value = def_bool;
    napi_create_function(env, NULL, 0, Framework_publish_method, NULL, &Framework_publish);
    napi_set_named_property(env, Framework.value, "publish", Framework_publish);
    napi_set_named_property(env, Framework.value, "value", Framework_value);
    napi_create_function(env, NULL, 0, Framework_connonchange_init, NULL, &Framework_conn_change);
    napi_set_named_property(env, Framework.value, "onConnectionChange", Framework_conn_change);
    napi_set_named_property(env, Framework.value, "connectionState", def_string);

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
    napi_set_named_property(env, dataModel, "Status", Status.value); 
    napi_set_named_property(env, dataModel, "Actuator", Actuator.value); 
    napi_set_named_property(env, dataModel, "Framework", Framework.value); 
    napi_set_named_property(env, belt.value, "datamodel", dataModel); 
    napi_create_function(env, NULL, 0, belt_connonchange_init, NULL, &belt_conn_change); 
    napi_set_named_property(env, belt.value, "onConnectionChange", belt_conn_change); 
    napi_set_named_property(env, belt.value, "connectionState", def_string);
    napi_set_named_property(env, belt.value, "isConnected", def_bool);
    napi_set_named_property(env, belt.value, "isOperational", def_bool);
    napi_create_function(env, NULL, 0, belt_onprocessed_init, NULL, &belt_onprocessed); 
    napi_set_named_property(env, belt.value, "onProcessed", belt_onprocessed); 
    napi_create_function(env, NULL, 0, get_net_time, NULL, &getNetTime);
    napi_set_named_property(env, belt.value, "nettime", getNetTime);
    napi_set_named_property(env, belt.value, "log", log);
    // export application object
    napi_set_named_property(env, exports, "Belt", belt.value); 

    // save references to object as globals for this C-file
    if (napi_ok != napi_create_reference(env, belt.value, belt.ref_count, &belt.ref)) 
    {
                    
        napi_throw_error(env, "EINVAL", "Can't create belt reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Status.value, Status.ref_count, &Status.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Status reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Actuator.value, Actuator.ref_count, &Actuator.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Actuator reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Framework.value, Framework.ref_count, &Framework.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Framework reference"); 
        return NULL; 
    } 

    // register clean up hook
    if (napi_ok != napi_add_env_cleanup_hook(env, cleanup_belt, env)) 
    {
        napi_throw_error(env, "EINVAL", "Can't register cleanup hook"); 
        return NULL; 
    } 

    // exOS
    // exOS inits
    if (EXOS_ERROR_OK != exos_datamodel_init(&belt_datamodel, "Belt_0", "gBelt_0")) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Belt"); 
    } 
    belt_datamodel.user_context = NULL; 
    belt_datamodel.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Status_dataset, &belt_datamodel, "Status", &exos_data.Status, sizeof(exos_data.Status))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Status"); 
    }
    Status_dataset.user_context = NULL; 
    Status_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Actuator_dataset, &belt_datamodel, "Actuator", &exos_data.Actuator, sizeof(exos_data.Actuator))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Actuator"); 
    }
    Actuator_dataset.user_context = NULL; 
    Actuator_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Framework_dataset, &belt_datamodel, "Framework", &exos_data.Framework, sizeof(exos_data.Framework))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Framework"); 
    }
    Framework_dataset.user_context = NULL; 
    Framework_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_log_init(&logger, "Belt_0"))
    {
        napi_throw_error(env, "EINVAL", "Can't register logger for Belt"); 
    } 

    INFO("Belt starting!")
    // exOS register datamodel
    if (EXOS_ERROR_OK != exos_datamodel_connect_belt(&belt_datamodel, datamodelEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Belt"); 
    } 

    // exOS register datasets
    if (EXOS_ERROR_OK != exos_dataset_connect(&Status_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Status"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Actuator_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Actuator"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Framework_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Framework"); 
    }

    // start up module

    uv_idle_init(uv_default_loop(), &cyclic_h); 
    uv_idle_start(&cyclic_h, cyclic); 

    SUCCESS("Belt started!")
    return exports; 
} 

// hook for Node-API
NAPI_MODULE(NODE_GYP_MODULE_NAME, init_belt);
