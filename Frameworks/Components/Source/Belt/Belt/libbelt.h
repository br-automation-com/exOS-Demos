#ifndef _LIBBELT_H_
#define _LIBBELT_H_

#include "exos_belt.h"

typedef void (*libBelt_event_cb)(void);
typedef void (*libBelt_method_fn)(void);
typedef int32_t (*libBelt_get_nettime_fn)(void);
typedef void (*libBelt_log_fn)(char *log_entry);

typedef struct libBeltStatus
{
    libBelt_method_fn publish;
    BeltStatus value;
} libBeltStatus_t;

typedef struct libBeltActuator
{
    libBelt_event_cb on_change;
    int32_t nettime;
    BeltActuator value;
} libBeltActuator_t;

typedef struct libBeltFramework
{
    libBelt_event_cb on_change;
    int32_t nettime;
    bool value;
} libBeltFramework_t;

typedef struct libBelt_log
{
    libBelt_log_fn error;
    libBelt_log_fn warning;
    libBelt_log_fn success;
    libBelt_log_fn info;
    libBelt_log_fn debug;
    libBelt_log_fn verbose;
} libBelt_log_t;

typedef struct libBelt
{
    libBelt_method_fn connect;
    libBelt_method_fn disconnect;
    libBelt_method_fn process;
    libBelt_method_fn set_operational;
    libBelt_method_fn dispose;
    libBelt_get_nettime_fn get_nettime;
    libBelt_log_t log;
    libBelt_event_cb on_connected;
    libBelt_event_cb on_disconnected;
    libBelt_event_cb on_operational;
    bool is_connected;
    bool is_operational;
    libBeltStatus_t Status;
    libBeltActuator_t Actuator;
    libBeltFramework_t Framework;
} libBelt_t;

#ifdef __cplusplus
extern "C" {
#endif
libBelt_t *libBelt_init(void);
#ifdef __cplusplus
}
#endif
#endif // _LIBBELT_H_
