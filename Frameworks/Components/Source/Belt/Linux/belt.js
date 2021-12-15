/**
 * @callback BeltDataModelCallback
 * @returns {function()}
 * 
 * @typedef {Object} BeltStatusDataSetValue
 * @property {boolean} Running 
 * @property {boolean} Latched 
 * @property {boolean} Actuator 
 * @property {number} ActualSpeed 
 * @property {number} ActualPosition 
 * @property {number} LatchedPosition 
 * @property {number} LatchIndex
 * 
 * @typedef {Object} BeltActuatorDataSetValue
 * @property {number} On 
 * @property {number} Off 
 * @property {number} LatchIndex
 * 
 * @typedef {Object} StatusDataSet
 * @property {BeltStatusDataSetValue} value Current Belt Values 
 * @property {BeltDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {BeltDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} ActuatorDataSet
 * @property {BeltActuatorDataSetValue} value Actuator set values from Linux 
 * @property {function()} publish publish the value
 * @property {BeltDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} FrameworkDataSet
 * @property {boolean} value Linux Application is connected to Framework 
 * @property {function()} publish publish the value
 * @property {BeltDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} BeltDatamodel
 * @property {StatusDataSet} Status
 * @property {ActuatorDataSet} Actuator
 * @property {FrameworkDataSet} Framework
 * 
 * @callback BeltDatamodelLogMethod
 * @param {string} message
 * 
 * @typedef {Object} BeltDatamodelLog
 * @property {BeltDatamodelLogMethod} warning
 * @property {BeltDatamodelLogMethod} success
 * @property {BeltDatamodelLogMethod} info
 * @property {BeltDatamodelLogMethod} debug
 * @property {BeltDatamodelLogMethod} verbose
 * 
 * @typedef {Object} Belt
 * @property {function():number} nettime get current nettime
 * @property {BeltDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * @property {boolean} isConnected
 * @property {boolean} isOperational
 * @property {BeltDatamodelLog} log
 * @property {BeltDatamodel} datamodel
 * 
 */

/**
 * @type {Belt}
 */
let belt = require('./l_Belt.node').Belt;

/* datamodel features:

main methods:
    belt.nettime() : (int32_t) get current nettime

state change events:
    belt.onConnectionChange(() => {
        belt.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted" 
    })

boolean values:
    belt.isConnected
    belt.isOperational

logging methods:
    belt.log.error(string)
    belt.log.warning(string)
    belt.log.success(string)
    belt.log.info(string)
    belt.log.debug(string)
    belt.log.verbose(string)

dataset Status:
    belt.datamodel.Status.value : (BeltStatus)  actual dataset values
    belt.datamodel.Status.onChange(() => {
        belt.datamodel.Status.value ...
        belt.datamodel.Status.nettime : (int32_t) nettime @ time of publish
        belt.datamodel.Status.latency : (int32_t) time in us between publish and arrival
    })
    belt.datamodel.Status.onConnectionChange(() => {
        belt.datamodel.Status.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Actuator:
    belt.datamodel.Actuator.value : (BeltActuator)  actual dataset values
    belt.datamodel.Actuator.publish()
    belt.datamodel.Actuator.onConnectionChange(() => {
        belt.datamodel.Actuator.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Framework:
    belt.datamodel.Framework.value : (bool)  actual dataset value
    belt.datamodel.Framework.publish()
    belt.datamodel.Framework.onConnectionChange(() => {
        belt.datamodel.Framework.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });
*/

const ipc = require('node-ipc').default;

let velocityX = [];
let velocityY = [];
let latchedY = [];
let actuatorY = [];
let positionY = [];

for(let i= 0; i< 100; i++) {
    velocityX[i] = i*10;
    velocityY[i] = 0;
    latchedY[i] = 0;
    actuatorY[i] = 0;
    positionY[i] = 0;
}

ipc.config.id   = 'Belt';
ipc.config.retry= 1500;



//connection state changes
belt.onConnectionChange(() => {
    switch (belt.connectionState) {
    case "Connected":
        ipc.connectTo(
            'framework',
            function(){
                ipc.of.framework.on(
                    'connect',
                    function(){
                        ipc.log('## connected to framework ##'.rainbow, ipc.config.delay);
                        ipc.of.framework.emit(
                            'message',
                            {
                                plugin: {
                                    name: "Belt",
                                    digital : [ 
                                        {
                                            name : "Running",
                                            value: true
                                        }
                                    ],
                                    numeric : [
                                        {
                                            name : "Actual Position",
                                            value: 0
                                        },
                                        {
                                            name : "Actual Speed",
                                            value: 0
                                        },
                                        {
                                            name : "Latched Position",
                                            value: 0
                                        },
                                    ],
                                    plot : [
                                        {
                                            name : "Plot Position",
                                            data : [
                                                {
                                                    x : velocityX,
                                                    y : positionY,
                                                    type : "scatter",
                                                    name : "Actual Position"
                                                }
                                            ],
                                            layout : {
                                                title: "Actual Position",
                                                height: 300
                                            }
                                        },
                                        {
                                            name : "Plot Actuator",
                                            data : [
                                                {
                                                    x : velocityX,
                                                    y : actuatorY,
                                                    type : "scatter",
                                                    name : "Actuator"
                                                },
                                                {
                                                    x : velocityX,
                                                    y : latchedY,
                                                    type : "scatter",
                                                    name: "Latched"
                                                }
                                            ],
                                            layout : {
                                                title: "Belt Actuator",
                                                height: 300
                                            }
                                        }
                                    ]
                                }
                            }
                        );

                        belt.datamodel.Framework.value = true;
                        belt.datamodel.Framework.publish();
                    }
                );
                ipc.of.framework.on(
                    'disconnect',
                    function(){
                        ipc.log('disconnected from framework'.notice);
                        belt.datamodel.Framework.value = false;
                        belt.datamodel.Framework.publish();
                    }
                );
                ipc.of.framework.on(
                    'message',
                    function(data){
                        ipc.log('got a message from framework : '.debug, data);
                    }
                );
            }
        );
        break;
    case "Operational":
        break;
    case "Disconnected":
        if(ipc.of.framework) {
            ipc.disconnect('framework');
        }
        break;
    case "Aborted":
        break;
    }
});

//value change events
belt.datamodel.Status.onChange(() => {
    if(belt.datamodel.Status.value.Latched) 
    {
        someComplicatedOnOffAlgorithm(belt.datamodel.Status.value, belt.datamodel.Actuator.value);
        belt.datamodel.Actuator.publish();
    }

    velocityY.shift();
    velocityY.push(belt.datamodel.Status.value.ActualSpeed);
    
    actuatorY.shift();
    actuatorY.push(belt.datamodel.Status.value.Actuator);

    latchedY.shift();
    latchedY.push(belt.datamodel.Status.value.Latched);

    positionY.shift();
    positionY.push(belt.datamodel.Status.value.ActualPosition);

});

const REFRESH_INTERVAL = 20;
let refresh = 0;

//Cyclic call triggered from the Component Server
belt.onProcessed(() => {
    if (belt.isConnected) {
        refresh ++;
        if(refresh >= REFRESH_INTERVAL) {
            if(ipc.of.framework) {
                ipc.of.framework.emit(
                    'message',
                    {
                        update: {
                            name: "Belt",
                            digital : [ 
                                {
                                    name : "Running",
                                    value: belt.datamodel.Status.value.Running
                                }
                            ],
                            numeric : [
                                {
                                    name : "Actual Position",
                                    value: belt.datamodel.Status.value.ActualPosition
                                },
                                {
                                    name : "Actual Speed",
                                    value: belt.datamodel.Status.value.ActualSpeed
                                },
                                {
                                    name : "Latched Position",
                                    value: belt.datamodel.Status.value.LatchedPosition
                                },
                            ],
                            plot : [
                                {
                                    name : "Plot Position",
                                    data : [
                                        {
                                            x : velocityX,
                                            y : positionY,
                                            type : "scatter",
                                            name : "Actual Position"
                                        }
                                    ],
                                    layout : {
                                        title: "Actual Position",
                                        height: 300
                                    }
                                },
                                {
                                    name : "Plot Actuator",
                                    data : [
                                        {
                                            x : velocityX,
                                            y : actuatorY,
                                            type : "scatter",
                                            name : "Actuator"
                                        },
                                        {
                                            x : velocityX,
                                            y : latchedY,
                                            type : "scatter",
                                            name: "Latched"
                                        }
                                    ],
                                    layout : {
                                        title: "Belt Actuator",
                                        height: 300
                                    }
                                }
                            ]
                        }
                    }
                );
            }
        }
    }
});


/**
 * @param {BeltStatusDataSetValue} status 
 * @param {BeltActuatorDataSetValue} actuator
 */
function someComplicatedOnOffAlgorithm(status, actuator)
{
    //the position increases with ActualSpeed every scan, so its a one second pulse after one second (at 50% speed)
    actuator.On = (150 + status.LatchedPosition) % 1000;
    actuator.Off = (250 + status.LatchedPosition) % 1000;
    actuator.LatchIndex ++;
}