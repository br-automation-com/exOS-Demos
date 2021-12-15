#include <string.h>
#include <stdbool.h>
#include "libbelt.h"

/* libBelt_t datamodel features:

main methods:
    belt->connect()
    belt->disconnect()
    belt->process()
    belt->set_operational()
    belt->dispose()
    belt->get_nettime() : (int32_t) get current nettime

void(void) user callbacks:
    belt->on_connected
    belt->on_disconnected
    belt->on_operational

boolean values:
    belt->is_connected
    belt->is_operational

logging methods:
    belt->log.error(char *)
    belt->log.warning(char *)
    belt->log.success(char *)
    belt->log.info(char *)
    belt->log.debug(char *)
    belt->log.verbose(char *)

dataset Status:
    belt->Status.publish()
    belt->Status.value : (BeltStatus)  actual dataset values

dataset Actuator:
    belt->Actuator.on_change : void(void) user callback function
    belt->Actuator.nettime : (int32_t) nettime @ time of publish
    belt->Actuator.value : (BeltActuator)  actual dataset values

dataset Framework:
    belt->Framework.on_change : void(void) user callback function
    belt->Framework.nettime : (int32_t) nettime @ time of publish
    belt->Framework.value : (bool)  actual dataset value
*/

static libBelt_t *belt;
static struct BeltCyclic *cyclic_inst;

static void on_connected_belt(void)
{
}

static void on_change_actuator(void)
{
    memcpy(&(cyclic_inst->pBelt->Actuator), &(belt->Actuator.value), sizeof(cyclic_inst->pBelt->Actuator));
    
    // Your code here...
}
static void on_change_framework(void)
{
    cyclic_inst->pBelt->Framework = belt->Framework.value;
    
    // Your code here...
}
_BUR_PUBLIC void BeltCyclic(struct BeltCyclic *inst)
{
    // check if function block has been created before
    if(cyclic_inst != NULL)
    {
        // return error if more than one function blocks have been created
        if(inst != cyclic_inst)
        {
            inst->Operational = false;
            inst->Connected = false;
            inst->Error = true;
            return;
        }
    }
    cyclic_inst = inst;
    // initialize library
    if((libBelt_t *)inst->_Handle == NULL || (libBelt_t *)inst->_Handle != belt)
    {
        //retrieve the belt structure
        belt = libBelt_init();

        //setup callbacks
        belt->on_connected = on_connected_belt;
        // belt->on_disconnected = .. ;
        // belt->on_operational = .. ;
        belt->Actuator.on_change = on_change_actuator;
        belt->Framework.on_change = on_change_framework;

        inst->_Handle = (UDINT)belt;
    }
    // return error if reference to structure is not set on function block
    if(inst->pBelt == NULL)
    {
        inst->Operational = false;
        inst->Connected = false;
        inst->Error = true;
        return;
    }
    if (inst->Enable && !inst->_Enable)
    {
        //connect to the server
        belt->connect();
    }
    if (!inst->Enable && inst->_Enable)
    {
        //disconnect from server
        cyclic_inst = NULL;
        belt->disconnect();
    }
    inst->_Enable = inst->Enable;

    if(inst->Start && !inst->_Start && belt->is_connected)
    {
        belt->set_operational();
        inst->_Start = inst->Start;
    }
    if(!inst->Start)
    {
        inst->_Start = false;
    }

    //trigger callbacks
    belt->process();

    if (belt->is_connected)
    {
        if (memcmp(&(belt->Status.value), &(inst->pBelt->Status), sizeof(inst->pBelt->Status)))
        {
            memcpy(&(belt->Status.value), &(inst->pBelt->Status), sizeof(belt->Status.value));
            belt->Status.publish();
        }
    
        // Your code here...
    }
    inst->Connected = belt->is_connected;
    inst->Operational = belt->is_operational;
}

UINT _EXIT ProgramExit(unsigned long phase)
{
    //shutdown
    belt->dispose();
    cyclic_inst = NULL;
    return 0;
}
