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
#include "exos_sequence.h"
#include <uv.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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

obj_handles sequence = {};
obj_handles Buttons = {};
obj_handles Start = {};
obj_handles Active = {};

napi_deferred deferred = NULL;
uv_idle_t cyclic_h;

Sequence exos_data = {};
exos_datamodel_handle_t sequence_datamodel;
exos_dataset_handle_t Buttons_dataset;
exos_dataset_handle_t Start_dataset;
exos_dataset_handle_t Active_dataset;

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
        if(0 == strcmp(dataset->name,"Buttons"))
        {
            if (Buttons.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Buttons.onchange_cb);
                napi_call_threadsafe_function(Buttons.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Buttons.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"Start"))
        {
            if (Start.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Start.onchange_cb);
                napi_call_threadsafe_function(Start.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Start.onchange_cb, napi_tsfn_release);
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published!", dataset->name);
        // fall through

    case EXOS_DATASET_EVENT_DELIVERED:
        if (event_type == EXOS_DATASET_EVENT_DELIVERED) { VERBOSE("dataset %s delivered!", dataset->name); }

        if(0 == strcmp(dataset->name, "Active"))
        {
            //bool *active = (bool *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        VERBOSE("dataset %s connecton changed to: %s", dataset->name, exos_get_state_string(dataset->connection_state));

        if(0 == strcmp(dataset->name, "Buttons"))
        {
            if (Buttons.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Buttons.connectiononchange_cb);
                napi_call_threadsafe_function(Buttons.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Buttons.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Start"))
        {
            if (Start.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Start.connectiononchange_cb);
                napi_call_threadsafe_function(Start.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Start.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Active"))
        {
            if (Active.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Active.connectiononchange_cb);
                napi_call_threadsafe_function(Active.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Active.connectiononchange_cb, napi_tsfn_release);
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
        INFO("application Sequence changed state to %s", exos_get_state_string(datamodel->connection_state));

        if (sequence.connectiononchange_cb != NULL)
        {
            napi_acquire_threadsafe_function(sequence.connectiononchange_cb);
            napi_call_threadsafe_function(sequence.connectiononchange_cb, exos_get_state_string(datamodel->connection_state), napi_tsfn_blocking);
            napi_release_threadsafe_function(sequence.connectiononchange_cb, napi_tsfn_release);
        }

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("Sequence operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("Sequence application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
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
static void sequence_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value napi_true, napi_false, undefined;

    napi_get_undefined(env, &undefined);

    napi_get_boolean(env, true, &napi_true);
    napi_get_boolean(env, false, &napi_false);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &sequence.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - sequence.value");

    if (napi_ok != napi_get_reference_value(env, sequence.ref, &sequence.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - sequence ");

    switch (sequence_datamodel.connection_state)
    {
    case EXOS_STATE_DISCONNECTED:
        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        break;
    case EXOS_STATE_CONNECTED:
        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        break;
    case EXOS_STATE_OPERATIONAL:
        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isOperational", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        break;
    case EXOS_STATE_ABORTED:
        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        if (napi_ok != napi_set_named_property(env, sequence.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

        break;
    }

    if (napi_ok != napi_set_named_property(env, sequence.object_value, "connectionState", sequence.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - sequence");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - sequence");
}

static void sequence_onprocessed_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Error calling onProcessed - Sequence");
}

static void Buttons_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Buttons.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Buttons.value");

    if (napi_ok != napi_get_reference_value(env, Buttons.ref, &Buttons.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Buttons ");

    if (napi_ok != napi_set_named_property(env, Buttons.object_value, "connectionState", Buttons.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Buttons");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Buttons");
}

static void Start_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Start.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Start.value");

    if (napi_ok != napi_get_reference_value(env, Start.ref, &Start.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Start ");

    if (napi_ok != napi_set_named_property(env, Start.object_value, "connectionState", Start.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Start");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Start");
}

static void Active_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Active.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Active.value");

    if (napi_ok != napi_get_reference_value(env, Active.ref, &Active.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Active ");

    if (napi_ok != napi_set_named_property(env, Active.object_value, "connectionState", Active.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Active");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Active");
}

// js value callbacks
static void Buttons_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Buttons.ref, &Buttons.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_get_boolean(env, (*((SequenceButtons *)ctx->pData)).ButtonLeft, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "ButtonLeft", property);
    if (napi_ok != napi_get_boolean(env, (*((SequenceButtons *)ctx->pData)).ButtonRight, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "ButtonRight", property);
Buttons.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&sequence_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Buttons.object_value, "nettime", netTime);
        napi_set_named_property(env, Buttons.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Buttons.object_value, "value", Buttons.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void Start_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Start.ref, &Start.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_get_boolean(env, (*((bool *)ctx->pData)), &Start.value))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

        int32_t _latency = exos_datamodel_get_nettime(&sequence_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Start.object_value, "nettime", netTime);
        napi_set_named_property(env, Start.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Start.object_value, "value", Start.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

// js callback inits
static napi_value sequence_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Sequence connection change", sequence_connonchange_js_cb, &sequence.connectiononchange_cb);
}

static napi_value sequence_onprocessed_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Sequence onProcessed", sequence_onprocessed_js_cb, &sequence.onprocessed_cb);
}

static napi_value Buttons_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Buttons connection change", Buttons_connonchange_js_cb, &Buttons.connectiononchange_cb);
}

static napi_value Start_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Start connection change", Start_connonchange_js_cb, &Start.connectiononchange_cb);
}

static napi_value Active_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Active connection change", Active_connonchange_js_cb, &Active.connectiononchange_cb);
}

static napi_value Buttons_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Buttons dataset change", Buttons_onchange_js_cb, &Buttons.onchange_cb);
}

static napi_value Start_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Start dataset change", Start_onchange_js_cb, &Start.onchange_cb);
}

// publish methods
static napi_value Active_publish_method(napi_env env, napi_callback_info info)
{

    if (napi_ok != napi_get_reference_value(env, Active.ref, &Active.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, Active.object_value, "value", &Active.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    if (napi_ok != napi_get_value_bool(env, Active.value, &exos_data.Active))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }

    exos_dataset_publish(&Active_dataset);
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.error()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.error()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.warning()");
        return  NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.warning()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.success()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.success()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.info()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.info()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.debug()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.debug()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for sequence.log.verbose()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for sequence.log.verbose()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
    return NULL;
}

// cleanup/cyclic
static void cleanup_sequence(void *env)
{
    uv_idle_stop(&cyclic_h);

    if (EXOS_ERROR_OK != exos_datamodel_delete(&sequence_datamodel))
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
    exos_datamodel_process(&sequence_datamodel);
    napi_acquire_threadsafe_function(sequence.onprocessed_cb);
    napi_call_threadsafe_function(sequence.onprocessed_cb, &dummy, napi_tsfn_blocking);
    napi_release_threadsafe_function(sequence.onprocessed_cb, napi_tsfn_release);
    exos_log_process(&logger);
}

//read nettime for DataModel
static napi_value get_net_time(napi_env env, napi_callback_info info)
{
    napi_value netTime;

    if (napi_ok == napi_create_int32(env, exos_datamodel_get_nettime(&sequence_datamodel), &netTime))
    {
        return netTime;
    }
    else
    {
        return NULL;
    }
}

// init of module, called at "require"
static napi_value init_sequence(napi_env env, napi_value exports)
{
    napi_value sequence_conn_change, sequence_onprocessed, Buttons_conn_change, Start_conn_change, Active_conn_change;
    napi_value Buttons_onchange, Start_onchange;
    napi_value Active_publish;
    napi_value Buttons_value, Start_value, Active_value;

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

    if (napi_ok != napi_create_object(env, &sequence.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Buttons.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Start.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Active.value)) 
        return NULL; 

    // build object structures
    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "ButtonLeft", def_bool);
    napi_set_named_property(env, object0, "ButtonRight", def_bool);
    Buttons_value = object0;
    napi_create_function(env, NULL, 0, Buttons_onchange_init, NULL, &Buttons_onchange);
    napi_set_named_property(env, Buttons.value, "onChange", Buttons_onchange);
    napi_set_named_property(env, Buttons.value, "nettime", undefined);
    napi_set_named_property(env, Buttons.value, "latency", undefined);
    napi_set_named_property(env, Buttons.value, "value", Buttons_value);
    napi_create_function(env, NULL, 0, Buttons_connonchange_init, NULL, &Buttons_conn_change);
    napi_set_named_property(env, Buttons.value, "onConnectionChange", Buttons_conn_change);
    napi_set_named_property(env, Buttons.value, "connectionState", def_string);

Start_value = def_bool;
    napi_create_function(env, NULL, 0, Start_onchange_init, NULL, &Start_onchange);
    napi_set_named_property(env, Start.value, "onChange", Start_onchange);
    napi_set_named_property(env, Start.value, "nettime", undefined);
    napi_set_named_property(env, Start.value, "latency", undefined);
    napi_set_named_property(env, Start.value, "value", Start_value);
    napi_create_function(env, NULL, 0, Start_connonchange_init, NULL, &Start_conn_change);
    napi_set_named_property(env, Start.value, "onConnectionChange", Start_conn_change);
    napi_set_named_property(env, Start.value, "connectionState", def_string);

Active_value = def_bool;
    napi_create_function(env, NULL, 0, Active_publish_method, NULL, &Active_publish);
    napi_set_named_property(env, Active.value, "publish", Active_publish);
    napi_set_named_property(env, Active.value, "value", Active_value);
    napi_create_function(env, NULL, 0, Active_connonchange_init, NULL, &Active_conn_change);
    napi_set_named_property(env, Active.value, "onConnectionChange", Active_conn_change);
    napi_set_named_property(env, Active.value, "connectionState", def_string);

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
    napi_set_named_property(env, dataModel, "Buttons", Buttons.value); 
    napi_set_named_property(env, dataModel, "Start", Start.value); 
    napi_set_named_property(env, dataModel, "Active", Active.value); 
    napi_set_named_property(env, sequence.value, "datamodel", dataModel); 
    napi_create_function(env, NULL, 0, sequence_connonchange_init, NULL, &sequence_conn_change); 
    napi_set_named_property(env, sequence.value, "onConnectionChange", sequence_conn_change); 
    napi_set_named_property(env, sequence.value, "connectionState", def_string);
    napi_set_named_property(env, sequence.value, "isConnected", def_bool);
    napi_set_named_property(env, sequence.value, "isOperational", def_bool);
    napi_create_function(env, NULL, 0, sequence_onprocessed_init, NULL, &sequence_onprocessed); 
    napi_set_named_property(env, sequence.value, "onProcessed", sequence_onprocessed); 
    napi_create_function(env, NULL, 0, get_net_time, NULL, &getNetTime);
    napi_set_named_property(env, sequence.value, "nettime", getNetTime);
    napi_set_named_property(env, sequence.value, "log", log);
    // export application object
    napi_set_named_property(env, exports, "Sequence", sequence.value); 

    // save references to object as globals for this C-file
    if (napi_ok != napi_create_reference(env, sequence.value, sequence.ref_count, &sequence.ref)) 
    {
                    
        napi_throw_error(env, "EINVAL", "Can't create sequence reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Buttons.value, Buttons.ref_count, &Buttons.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Buttons reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Start.value, Start.ref_count, &Start.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Start reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Active.value, Active.ref_count, &Active.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Active reference"); 
        return NULL; 
    } 

    // register clean up hook
    if (napi_ok != napi_add_env_cleanup_hook(env, cleanup_sequence, env)) 
    {
        napi_throw_error(env, "EINVAL", "Can't register cleanup hook"); 
        return NULL; 
    } 

    // exOS
    // exOS inits
    if (EXOS_ERROR_OK != exos_datamodel_init(&sequence_datamodel, "Sequence_0", "gSequence_0")) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Sequence"); 
    } 
    sequence_datamodel.user_context = NULL; 
    sequence_datamodel.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Buttons_dataset, &sequence_datamodel, "Buttons", &exos_data.Buttons, sizeof(exos_data.Buttons))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Buttons"); 
    }
    Buttons_dataset.user_context = NULL; 
    Buttons_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Start_dataset, &sequence_datamodel, "Start", &exos_data.Start, sizeof(exos_data.Start))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Start"); 
    }
    Start_dataset.user_context = NULL; 
    Start_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Active_dataset, &sequence_datamodel, "Active", &exos_data.Active, sizeof(exos_data.Active))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Active"); 
    }
    Active_dataset.user_context = NULL; 
    Active_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_log_init(&logger, "Sequence_0"))
    {
        napi_throw_error(env, "EINVAL", "Can't register logger for Sequence"); 
    } 

    INFO("Sequence starting!")
    // exOS register datamodel
    if (EXOS_ERROR_OK != exos_datamodel_connect_sequence(&sequence_datamodel, datamodelEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Sequence"); 
    } 

    // exOS register datasets
    if (EXOS_ERROR_OK != exos_dataset_connect(&Buttons_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Buttons"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Start_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Start"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Active_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Active"); 
    }

    // start up module

    uv_idle_init(uv_default_loop(), &cyclic_h); 
    uv_idle_start(&cyclic_h, cyclic); 

    SUCCESS("Sequence started!")
    return exports; 
} 

// hook for Node-API
NAPI_MODULE(NODE_GYP_MODULE_NAME, init_sequence);
