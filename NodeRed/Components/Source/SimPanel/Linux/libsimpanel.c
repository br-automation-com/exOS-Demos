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
#include "exos_simpanel.h"
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

obj_handles simpanel = {};
obj_handles Switches = {};
obj_handles Buttons = {};
obj_handles LEDs = {};
obj_handles Knobs = {};
obj_handles Display = {};
obj_handles Encoder = {};

napi_deferred deferred = NULL;
uv_idle_t cyclic_h;

SimPanel exos_data = {};
exos_datamodel_handle_t simpanel_datamodel;
exos_dataset_handle_t Switches_dataset;
exos_dataset_handle_t Buttons_dataset;
exos_dataset_handle_t LEDs_dataset;
exos_dataset_handle_t Knobs_dataset;
exos_dataset_handle_t Display_dataset;
exos_dataset_handle_t Encoder_dataset;

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
        if(0 == strcmp(dataset->name,"Switches"))
        {
            if (Switches.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Switches.onchange_cb);
                napi_call_threadsafe_function(Switches.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Switches.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"Buttons"))
        {
            if (Buttons.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Buttons.onchange_cb);
                napi_call_threadsafe_function(Buttons.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Buttons.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"Knobs"))
        {
            if (Knobs.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Knobs.onchange_cb);
                napi_call_threadsafe_function(Knobs.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Knobs.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"Encoder"))
        {
            if (Encoder.onchange_cb != NULL)
            {
                callback_context_t *ctx = create_callback_context(dataset);
                
                napi_acquire_threadsafe_function(Encoder.onchange_cb);
                napi_call_threadsafe_function(Encoder.onchange_cb, ctx, napi_tsfn_blocking);
                napi_release_threadsafe_function(Encoder.onchange_cb, napi_tsfn_release);
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published!", dataset->name);
        // fall through

    case EXOS_DATASET_EVENT_DELIVERED:
        if (event_type == EXOS_DATASET_EVENT_DELIVERED) { VERBOSE("dataset %s delivered!", dataset->name); }

        if(0 == strcmp(dataset->name, "LEDs"))
        {
            //SimPanelLEDs *leds = (SimPanelLEDs *)dataset->data;
        }
        else if(0 == strcmp(dataset->name, "Display"))
        {
            //int16_t *display = (int16_t *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        VERBOSE("dataset %s connecton changed to: %s", dataset->name, exos_get_state_string(dataset->connection_state));

        if(0 == strcmp(dataset->name, "Switches"))
        {
            if (Switches.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Switches.connectiononchange_cb);
                napi_call_threadsafe_function(Switches.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Switches.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Buttons"))
        {
            if (Buttons.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Buttons.connectiononchange_cb);
                napi_call_threadsafe_function(Buttons.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Buttons.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "LEDs"))
        {
            if (LEDs.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(LEDs.connectiononchange_cb);
                napi_call_threadsafe_function(LEDs.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(LEDs.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Knobs"))
        {
            if (Knobs.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Knobs.connectiononchange_cb);
                napi_call_threadsafe_function(Knobs.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Knobs.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Display"))
        {
            if (Display.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Display.connectiononchange_cb);
                napi_call_threadsafe_function(Display.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Display.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Encoder"))
        {
            if (Encoder.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Encoder.connectiononchange_cb);
                napi_call_threadsafe_function(Encoder.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Encoder.connectiononchange_cb, napi_tsfn_release);
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
        INFO("application SimPanel changed state to %s", exos_get_state_string(datamodel->connection_state));

        if (simpanel.connectiononchange_cb != NULL)
        {
            napi_acquire_threadsafe_function(simpanel.connectiononchange_cb);
            napi_call_threadsafe_function(simpanel.connectiononchange_cb, exos_get_state_string(datamodel->connection_state), napi_tsfn_blocking);
            napi_release_threadsafe_function(simpanel.connectiononchange_cb, napi_tsfn_release);
        }

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("SimPanel operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("SimPanel application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
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
static void simpanel_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value napi_true, napi_false, undefined;

    napi_get_undefined(env, &undefined);

    napi_get_boolean(env, true, &napi_true);
    napi_get_boolean(env, false, &napi_false);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &simpanel.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - simpanel.value");

    if (napi_ok != napi_get_reference_value(env, simpanel.ref, &simpanel.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - simpanel ");

    switch (simpanel_datamodel.connection_state)
    {
    case EXOS_STATE_DISCONNECTED:
        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        break;
    case EXOS_STATE_CONNECTED:
        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        break;
    case EXOS_STATE_OPERATIONAL:
        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isOperational", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        break;
    case EXOS_STATE_ABORTED:
        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        if (napi_ok != napi_set_named_property(env, simpanel.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

        break;
    }

    if (napi_ok != napi_set_named_property(env, simpanel.object_value, "connectionState", simpanel.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - simpanel");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - simpanel");
}

static void simpanel_onprocessed_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Error calling onProcessed - SimPanel");
}

static void Switches_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Switches.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Switches.value");

    if (napi_ok != napi_get_reference_value(env, Switches.ref, &Switches.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Switches ");

    if (napi_ok != napi_set_named_property(env, Switches.object_value, "connectionState", Switches.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Switches");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Switches");
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

static void LEDs_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &LEDs.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - LEDs.value");

    if (napi_ok != napi_get_reference_value(env, LEDs.ref, &LEDs.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - LEDs ");

    if (napi_ok != napi_set_named_property(env, LEDs.object_value, "connectionState", LEDs.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - LEDs");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - LEDs");
}

static void Knobs_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Knobs.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Knobs.value");

    if (napi_ok != napi_get_reference_value(env, Knobs.ref, &Knobs.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Knobs ");

    if (napi_ok != napi_set_named_property(env, Knobs.object_value, "connectionState", Knobs.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Knobs");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Knobs");
}

static void Display_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Display.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Display.value");

    if (napi_ok != napi_get_reference_value(env, Display.ref, &Display.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Display ");

    if (napi_ok != napi_set_named_property(env, Display.object_value, "connectionState", Display.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Display");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Display");
}

static void Encoder_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Encoder.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Encoder.value");

    if (napi_ok != napi_get_reference_value(env, Encoder.ref, &Encoder.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Encoder ");

    if (napi_ok != napi_set_named_property(env, Encoder.object_value, "connectionState", Encoder.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Encoder");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Encoder");
}

// js value callbacks
static void Switches_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Switches.ref, &Switches.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelSwitches *)ctx->pData)).DI1, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI1", property);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelSwitches *)ctx->pData)).DI2, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI2", property);
Switches.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&simpanel_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Switches.object_value, "nettime", netTime);
        napi_set_named_property(env, Switches.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Switches.object_value, "value", Switches.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

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
    if (napi_ok != napi_get_boolean(env, (*((SimPanelButtons *)ctx->pData)).DI3, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI3", property);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelButtons *)ctx->pData)).DI4, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI4", property);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelButtons *)ctx->pData)).DI5, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI5", property);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelButtons *)ctx->pData)).DI6, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "DI6", property);
    if (napi_ok != napi_get_boolean(env, (*((SimPanelButtons *)ctx->pData)).Encoder, &property))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

    napi_set_named_property(env, object0, "Encoder", property);
Buttons.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&simpanel_datamodel) - ctx->nettime;
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

static void Knobs_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Knobs.ref, &Knobs.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((SimPanelKnobs *)ctx->pData)).P1, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "P1", property);
    if (napi_ok != napi_create_int32(env, (int32_t)(*((SimPanelKnobs *)ctx->pData)).P2, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
    napi_set_named_property(env, object0, "P2", property);
Knobs.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&simpanel_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Knobs.object_value, "nettime", netTime);
        napi_set_named_property(env, Knobs.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Knobs.object_value, "value", Knobs.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

static void Encoder_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *cb_context)
{
    callback_context_t *ctx = (callback_context_t *)cb_context;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, Encoder.ref, &Encoder.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_create_uint32(env, (uint32_t)(*((uint16_t *)ctx->pData)), &Encoder.value))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit unsigned integer");
    }
        int32_t _latency = exos_datamodel_get_nettime(&simpanel_datamodel) - ctx->nettime;
        napi_create_int32(env, ctx->nettime, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, Encoder.object_value, "nettime", netTime);
        napi_set_named_property(env, Encoder.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, Encoder.object_value, "value", Encoder.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    
    free(ctx);
}

// js callback inits
static napi_value simpanel_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "SimPanel connection change", simpanel_connonchange_js_cb, &simpanel.connectiononchange_cb);
}

static napi_value simpanel_onprocessed_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "SimPanel onProcessed", simpanel_onprocessed_js_cb, &simpanel.onprocessed_cb);
}

static napi_value Switches_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Switches connection change", Switches_connonchange_js_cb, &Switches.connectiononchange_cb);
}

static napi_value Buttons_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Buttons connection change", Buttons_connonchange_js_cb, &Buttons.connectiononchange_cb);
}

static napi_value LEDs_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "LEDs connection change", LEDs_connonchange_js_cb, &LEDs.connectiononchange_cb);
}

static napi_value Knobs_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Knobs connection change", Knobs_connonchange_js_cb, &Knobs.connectiononchange_cb);
}

static napi_value Display_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Display connection change", Display_connonchange_js_cb, &Display.connectiononchange_cb);
}

static napi_value Encoder_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Encoder connection change", Encoder_connonchange_js_cb, &Encoder.connectiononchange_cb);
}

static napi_value Switches_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Switches dataset change", Switches_onchange_js_cb, &Switches.onchange_cb);
}

static napi_value Buttons_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Buttons dataset change", Buttons_onchange_js_cb, &Buttons.onchange_cb);
}

static napi_value Knobs_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Knobs dataset change", Knobs_onchange_js_cb, &Knobs.onchange_cb);
}

static napi_value Encoder_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Encoder dataset change", Encoder_onchange_js_cb, &Encoder.onchange_cb);
}

// publish methods
static napi_value LEDs_publish_method(napi_env env, napi_callback_info info)
{
    napi_value object0, object1, object2;

    if (napi_ok != napi_get_reference_value(env, LEDs.ref, &LEDs.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, LEDs.object_value, "value", &LEDs.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    object0 = LEDs.value;
    napi_get_named_property(env, object0, "DI1", &object1);
    if (napi_ok != napi_get_value_bool(env, object1, &exos_data.LEDs.DI1))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "DI2", &object1);
    if (napi_ok != napi_get_value_bool(env, object1, &exos_data.LEDs.DI2))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "DI3", &object1);
    napi_get_named_property(env, object1, "Green", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI3.Green))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Red", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI3.Red))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Yellow", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI3.Yellow))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "DI4", &object1);
    napi_get_named_property(env, object1, "Green", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI4.Green))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Red", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI4.Red))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Yellow", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI4.Yellow))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "DI5", &object1);
    napi_get_named_property(env, object1, "Green", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI5.Green))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Red", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI5.Red))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Yellow", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI5.Yellow))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "DI6", &object1);
    napi_get_named_property(env, object1, "Green", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI6.Green))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Red", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI6.Red))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object1, "Yellow", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.LEDs.DI6.Yellow))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    exos_dataset_publish(&LEDs_dataset);
    return NULL;
}

static napi_value Display_publish_method(napi_env env, napi_callback_info info)
{
    int32_t _value;

    if (napi_ok != napi_get_reference_value(env, Display.ref, &Display.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, Display.object_value, "value", &Display.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    if (napi_ok != napi_get_value_int32(env, Display.value, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Display = (int16_t)_value;

    exos_dataset_publish(&Display_dataset);
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.error()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.error()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.warning()");
        return  NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.warning()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.success()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.success()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.info()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.info()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.debug()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.debug()");
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
        napi_throw_error(env, "EINVAL", "Too few arguments for simpanel.log.verbose()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for simpanel.log.verbose()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
    return NULL;
}

// cleanup/cyclic
static void cleanup_simpanel(void *env)
{
    uv_idle_stop(&cyclic_h);

    if (EXOS_ERROR_OK != exos_datamodel_delete(&simpanel_datamodel))
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
    exos_datamodel_process(&simpanel_datamodel);
    napi_acquire_threadsafe_function(simpanel.onprocessed_cb);
    napi_call_threadsafe_function(simpanel.onprocessed_cb, &dummy, napi_tsfn_blocking);
    napi_release_threadsafe_function(simpanel.onprocessed_cb, napi_tsfn_release);
    exos_log_process(&logger);
}

//read nettime for DataModel
static napi_value get_net_time(napi_env env, napi_callback_info info)
{
    napi_value netTime;

    if (napi_ok == napi_create_int32(env, exos_datamodel_get_nettime(&simpanel_datamodel), &netTime))
    {
        return netTime;
    }
    else
    {
        return NULL;
    }
}

// init of module, called at "require"
static napi_value init_simpanel(napi_env env, napi_value exports)
{
    napi_value simpanel_conn_change, simpanel_onprocessed, Switches_conn_change, Buttons_conn_change, LEDs_conn_change, Knobs_conn_change, Display_conn_change, Encoder_conn_change;
    napi_value Switches_onchange, Buttons_onchange, Knobs_onchange, Encoder_onchange;
    napi_value LEDs_publish, Display_publish;
    napi_value Switches_value, Buttons_value, LEDs_value, Knobs_value, Display_value, Encoder_value;

    napi_value dataModel, getNetTime, undefined, def_bool, def_number, def_string;
    napi_value log, logError, logWarning, logSuccess, logInfo, logDebug, logVerbose;
    napi_value object0, object1;

    napi_get_boolean(env, BUR_NAPI_DEFAULT_BOOL_INIT, &def_bool); 
    napi_create_int32(env, BUR_NAPI_DEFAULT_NUM_INIT, &def_number); 
    napi_create_string_utf8(env, BUR_NAPI_DEFAULT_STRING_INIT, strlen(BUR_NAPI_DEFAULT_STRING_INIT), &def_string);
    napi_get_undefined(env, &undefined); 

    // create base objects
    if (napi_ok != napi_create_object(env, &dataModel)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &log)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &simpanel.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Switches.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Buttons.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &LEDs.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Knobs.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Display.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Encoder.value)) 
        return NULL; 

    // build object structures
    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "DI1", def_bool);
    napi_set_named_property(env, object0, "DI2", def_bool);
    Switches_value = object0;
    napi_create_function(env, NULL, 0, Switches_onchange_init, NULL, &Switches_onchange);
    napi_set_named_property(env, Switches.value, "onChange", Switches_onchange);
    napi_set_named_property(env, Switches.value, "nettime", undefined);
    napi_set_named_property(env, Switches.value, "latency", undefined);
    napi_set_named_property(env, Switches.value, "value", Switches_value);
    napi_create_function(env, NULL, 0, Switches_connonchange_init, NULL, &Switches_conn_change);
    napi_set_named_property(env, Switches.value, "onConnectionChange", Switches_conn_change);
    napi_set_named_property(env, Switches.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "DI3", def_bool);
    napi_set_named_property(env, object0, "DI4", def_bool);
    napi_set_named_property(env, object0, "DI5", def_bool);
    napi_set_named_property(env, object0, "DI6", def_bool);
    napi_set_named_property(env, object0, "Encoder", def_bool);
    Buttons_value = object0;
    napi_create_function(env, NULL, 0, Buttons_onchange_init, NULL, &Buttons_onchange);
    napi_set_named_property(env, Buttons.value, "onChange", Buttons_onchange);
    napi_set_named_property(env, Buttons.value, "nettime", undefined);
    napi_set_named_property(env, Buttons.value, "latency", undefined);
    napi_set_named_property(env, Buttons.value, "value", Buttons_value);
    napi_create_function(env, NULL, 0, Buttons_connonchange_init, NULL, &Buttons_conn_change);
    napi_set_named_property(env, Buttons.value, "onConnectionChange", Buttons_conn_change);
    napi_set_named_property(env, Buttons.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "DI1", def_bool);
    napi_set_named_property(env, object0, "DI2", def_bool);
        napi_create_object(env, &object1);
    napi_set_named_property(env, object1, "Green", def_bool);
    napi_set_named_property(env, object1, "Red", def_bool);
    napi_set_named_property(env, object1, "Yellow", def_bool);
    napi_set_named_property(env, object0, "DI3", object1);
        napi_create_object(env, &object1);
    napi_set_named_property(env, object1, "Green", def_bool);
    napi_set_named_property(env, object1, "Red", def_bool);
    napi_set_named_property(env, object1, "Yellow", def_bool);
    napi_set_named_property(env, object0, "DI4", object1);
        napi_create_object(env, &object1);
    napi_set_named_property(env, object1, "Green", def_bool);
    napi_set_named_property(env, object1, "Red", def_bool);
    napi_set_named_property(env, object1, "Yellow", def_bool);
    napi_set_named_property(env, object0, "DI5", object1);
        napi_create_object(env, &object1);
    napi_set_named_property(env, object1, "Green", def_bool);
    napi_set_named_property(env, object1, "Red", def_bool);
    napi_set_named_property(env, object1, "Yellow", def_bool);
    napi_set_named_property(env, object0, "DI6", object1);
    LEDs_value = object0;
    napi_create_function(env, NULL, 0, LEDs_publish_method, NULL, &LEDs_publish);
    napi_set_named_property(env, LEDs.value, "publish", LEDs_publish);
    napi_set_named_property(env, LEDs.value, "value", LEDs_value);
    napi_create_function(env, NULL, 0, LEDs_connonchange_init, NULL, &LEDs_conn_change);
    napi_set_named_property(env, LEDs.value, "onConnectionChange", LEDs_conn_change);
    napi_set_named_property(env, LEDs.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "P1", def_number);
    napi_set_named_property(env, object0, "P2", def_number);
    Knobs_value = object0;
    napi_create_function(env, NULL, 0, Knobs_onchange_init, NULL, &Knobs_onchange);
    napi_set_named_property(env, Knobs.value, "onChange", Knobs_onchange);
    napi_set_named_property(env, Knobs.value, "nettime", undefined);
    napi_set_named_property(env, Knobs.value, "latency", undefined);
    napi_set_named_property(env, Knobs.value, "value", Knobs_value);
    napi_create_function(env, NULL, 0, Knobs_connonchange_init, NULL, &Knobs_conn_change);
    napi_set_named_property(env, Knobs.value, "onConnectionChange", Knobs_conn_change);
    napi_set_named_property(env, Knobs.value, "connectionState", def_string);

Display_value = def_number;
    napi_create_function(env, NULL, 0, Display_publish_method, NULL, &Display_publish);
    napi_set_named_property(env, Display.value, "publish", Display_publish);
    napi_set_named_property(env, Display.value, "value", Display_value);
    napi_create_function(env, NULL, 0, Display_connonchange_init, NULL, &Display_conn_change);
    napi_set_named_property(env, Display.value, "onConnectionChange", Display_conn_change);
    napi_set_named_property(env, Display.value, "connectionState", def_string);

Encoder_value = def_number;
    napi_create_function(env, NULL, 0, Encoder_onchange_init, NULL, &Encoder_onchange);
    napi_set_named_property(env, Encoder.value, "onChange", Encoder_onchange);
    napi_set_named_property(env, Encoder.value, "nettime", undefined);
    napi_set_named_property(env, Encoder.value, "latency", undefined);
    napi_set_named_property(env, Encoder.value, "value", Encoder_value);
    napi_create_function(env, NULL, 0, Encoder_connonchange_init, NULL, &Encoder_conn_change);
    napi_set_named_property(env, Encoder.value, "onConnectionChange", Encoder_conn_change);
    napi_set_named_property(env, Encoder.value, "connectionState", def_string);

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
    napi_set_named_property(env, dataModel, "Switches", Switches.value); 
    napi_set_named_property(env, dataModel, "Buttons", Buttons.value); 
    napi_set_named_property(env, dataModel, "LEDs", LEDs.value); 
    napi_set_named_property(env, dataModel, "Knobs", Knobs.value); 
    napi_set_named_property(env, dataModel, "Display", Display.value); 
    napi_set_named_property(env, dataModel, "Encoder", Encoder.value); 
    napi_set_named_property(env, simpanel.value, "datamodel", dataModel); 
    napi_create_function(env, NULL, 0, simpanel_connonchange_init, NULL, &simpanel_conn_change); 
    napi_set_named_property(env, simpanel.value, "onConnectionChange", simpanel_conn_change); 
    napi_set_named_property(env, simpanel.value, "connectionState", def_string);
    napi_set_named_property(env, simpanel.value, "isConnected", def_bool);
    napi_set_named_property(env, simpanel.value, "isOperational", def_bool);
    napi_create_function(env, NULL, 0, simpanel_onprocessed_init, NULL, &simpanel_onprocessed); 
    napi_set_named_property(env, simpanel.value, "onProcessed", simpanel_onprocessed); 
    napi_create_function(env, NULL, 0, get_net_time, NULL, &getNetTime);
    napi_set_named_property(env, simpanel.value, "nettime", getNetTime);
    napi_set_named_property(env, simpanel.value, "log", log);
    // export application object
    napi_set_named_property(env, exports, "SimPanel", simpanel.value); 

    // save references to object as globals for this C-file
    if (napi_ok != napi_create_reference(env, simpanel.value, simpanel.ref_count, &simpanel.ref)) 
    {
                    
        napi_throw_error(env, "EINVAL", "Can't create simpanel reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Switches.value, Switches.ref_count, &Switches.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Switches reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Buttons.value, Buttons.ref_count, &Buttons.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Buttons reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, LEDs.value, LEDs.ref_count, &LEDs.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create LEDs reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Knobs.value, Knobs.ref_count, &Knobs.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Knobs reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Display.value, Display.ref_count, &Display.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Display reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Encoder.value, Encoder.ref_count, &Encoder.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Encoder reference"); 
        return NULL; 
    } 

    // register clean up hook
    if (napi_ok != napi_add_env_cleanup_hook(env, cleanup_simpanel, env)) 
    {
        napi_throw_error(env, "EINVAL", "Can't register cleanup hook"); 
        return NULL; 
    } 

    // exOS
    // exOS inits
    if (EXOS_ERROR_OK != exos_datamodel_init(&simpanel_datamodel, "SimPanel_0", "gSimPanel_0")) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize SimPanel"); 
    } 
    simpanel_datamodel.user_context = NULL; 
    simpanel_datamodel.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Switches_dataset, &simpanel_datamodel, "Switches", &exos_data.Switches, sizeof(exos_data.Switches))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Switches"); 
    }
    Switches_dataset.user_context = NULL; 
    Switches_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Buttons_dataset, &simpanel_datamodel, "Buttons", &exos_data.Buttons, sizeof(exos_data.Buttons))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Buttons"); 
    }
    Buttons_dataset.user_context = NULL; 
    Buttons_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&LEDs_dataset, &simpanel_datamodel, "LEDs", &exos_data.LEDs, sizeof(exos_data.LEDs))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize LEDs"); 
    }
    LEDs_dataset.user_context = NULL; 
    LEDs_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Knobs_dataset, &simpanel_datamodel, "Knobs", &exos_data.Knobs, sizeof(exos_data.Knobs))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Knobs"); 
    }
    Knobs_dataset.user_context = NULL; 
    Knobs_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Display_dataset, &simpanel_datamodel, "Display", &exos_data.Display, sizeof(exos_data.Display))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Display"); 
    }
    Display_dataset.user_context = NULL; 
    Display_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Encoder_dataset, &simpanel_datamodel, "Encoder", &exos_data.Encoder, sizeof(exos_data.Encoder))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Encoder"); 
    }
    Encoder_dataset.user_context = NULL; 
    Encoder_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_log_init(&logger, "SimPanel_0"))
    {
        napi_throw_error(env, "EINVAL", "Can't register logger for SimPanel"); 
    } 

    INFO("SimPanel starting!")
    // exOS register datamodel
    if (EXOS_ERROR_OK != exos_datamodel_connect_simpanel(&simpanel_datamodel, datamodelEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect SimPanel"); 
    } 

    // exOS register datasets
    if (EXOS_ERROR_OK != exos_dataset_connect(&Switches_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Switches"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Buttons_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Buttons"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&LEDs_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect LEDs"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Knobs_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Knobs"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Display_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Display"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Encoder_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Encoder"); 
    }

    // start up module

    uv_idle_init(uv_default_loop(), &cyclic_h); 
    uv_idle_start(&cyclic_h, cyclic); 

    SUCCESS("SimPanel started!")
    return exports; 
} 

// hook for Node-API
NAPI_MODULE(NODE_GYP_MODULE_NAME, init_simpanel);
