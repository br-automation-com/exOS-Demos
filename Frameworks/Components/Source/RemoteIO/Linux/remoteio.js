/**
 * @callback RemoteIODataModelCallback
 * @returns {function()}
 * 
 * @typedef {Object} RemoteIOAnalogInputDataSetValue
 * @property {string} Name 
 * @property {number} State 
 * 
 * @typedef {Object} RemoteIODigitalInputDataSetValue
 * @property {string} Name 
 * @property {boolean} State 
 * 
 * @typedef {Object} RemoteIODigitalOutputDataSetValue
 * @property {string} Name 
 * @property {boolean} State 
 * 
 * @typedef {Object} AnalogInputDataSet
 * @property {RemoteIOAnalogInputDataSetValue} value 
 * @property {RemoteIODataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} DigitalInputDataSet
 * @property {RemoteIODigitalInputDataSetValue} value 
 * @property {RemoteIODataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} DigitalOutputDataSet
 * @property {RemoteIODigitalOutputDataSetValue} value 
 * @property {function()} publish publish the value
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} NewAnalogInputDataSet
 * @property {string} value 
 * @property {RemoteIODataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} NewDigitalInputDataSet
 * @property {string} value 
 * @property {RemoteIODataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} NewDigitalOutputDataSet
 * @property {string} value 
 * @property {RemoteIODataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} RemoteIODatamodel
 * @property {AnalogInputDataSet} AnalogInput
 * @property {DigitalInputDataSet} DigitalInput
 * @property {DigitalOutputDataSet} DigitalOutput
 * @property {NewAnalogInputDataSet} NewAnalogInput
 * @property {NewDigitalInputDataSet} NewDigitalInput
 * @property {NewDigitalOutputDataSet} NewDigitalOutput
 * 
 * @callback RemoteIODatamodelLogMethod
 * @param {string} message
 * 
 * @typedef {Object} RemoteIODatamodelLog
 * @property {RemoteIODatamodelLogMethod} warning
 * @property {RemoteIODatamodelLogMethod} success
 * @property {RemoteIODatamodelLogMethod} info
 * @property {RemoteIODatamodelLogMethod} debug
 * @property {RemoteIODatamodelLogMethod} verbose
 * 
 * @typedef {Object} RemoteIO
 * @property {function():number} nettime get current nettime
 * @property {RemoteIODataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * @property {boolean} isConnected
 * @property {boolean} isOperational
 * @property {RemoteIODatamodelLog} log
 * @property {RemoteIODatamodel} datamodel
 * 
 */

/**
 * @type {RemoteIO}
 */
let remoteio = require('./l_RemoteIO.node').RemoteIO;

/* datamodel features:

main methods:
    remoteio.nettime() : (int32_t) get current nettime

state change events:
    remoteio.onConnectionChange(() => {
        remoteio.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted" 
    })

boolean values:
    remoteio.isConnected
    remoteio.isOperational

logging methods:
    remoteio.log.error(string)
    remoteio.log.warning(string)
    remoteio.log.success(string)
    remoteio.log.info(string)
    remoteio.log.debug(string)
    remoteio.log.verbose(string)

dataset AnalogInput:
    remoteio.datamodel.AnalogInput.value : (RemoteIOAnalog)  actual dataset values
    remoteio.datamodel.AnalogInput.onChange(() => {
        remoteio.datamodel.AnalogInput.value ...
        remoteio.datamodel.AnalogInput.nettime : (int32_t) nettime @ time of publish
        remoteio.datamodel.AnalogInput.latency : (int32_t) time in us between publish and arrival
    })
    remoteio.datamodel.AnalogInput.onConnectionChange(() => {
        remoteio.datamodel.AnalogInput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset DigitalInput:
    remoteio.datamodel.DigitalInput.value : (RemoteIODigital)  actual dataset values
    remoteio.datamodel.DigitalInput.onChange(() => {
        remoteio.datamodel.DigitalInput.value ...
        remoteio.datamodel.DigitalInput.nettime : (int32_t) nettime @ time of publish
        remoteio.datamodel.DigitalInput.latency : (int32_t) time in us between publish and arrival
    })
    remoteio.datamodel.DigitalInput.onConnectionChange(() => {
        remoteio.datamodel.DigitalInput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset DigitalOutput:
    remoteio.datamodel.DigitalOutput.value : (RemoteIODigital)  actual dataset values
    remoteio.datamodel.DigitalOutput.publish()
    remoteio.datamodel.DigitalOutput.onConnectionChange(() => {
        remoteio.datamodel.DigitalOutput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset NewAnalogInput:
    remoteio.datamodel.NewAnalogInput.value : (char[81)  actual dataset value
    remoteio.datamodel.NewAnalogInput.onChange(() => {
        remoteio.datamodel.NewAnalogInput.value ...
        remoteio.datamodel.NewAnalogInput.nettime : (int32_t) nettime @ time of publish
        remoteio.datamodel.NewAnalogInput.latency : (int32_t) time in us between publish and arrival
    })
    remoteio.datamodel.NewAnalogInput.onConnectionChange(() => {
        remoteio.datamodel.NewAnalogInput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset NewDigitalInput:
    remoteio.datamodel.NewDigitalInput.value : (char[81)  actual dataset value
    remoteio.datamodel.NewDigitalInput.onChange(() => {
        remoteio.datamodel.NewDigitalInput.value ...
        remoteio.datamodel.NewDigitalInput.nettime : (int32_t) nettime @ time of publish
        remoteio.datamodel.NewDigitalInput.latency : (int32_t) time in us between publish and arrival
    })
    remoteio.datamodel.NewDigitalInput.onConnectionChange(() => {
        remoteio.datamodel.NewDigitalInput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset NewDigitalOutput:
    remoteio.datamodel.NewDigitalOutput.value : (char[81)  actual dataset value
    remoteio.datamodel.NewDigitalOutput.onChange(() => {
        remoteio.datamodel.NewDigitalOutput.value ...
        remoteio.datamodel.NewDigitalOutput.nettime : (int32_t) nettime @ time of publish
        remoteio.datamodel.NewDigitalOutput.latency : (int32_t) time in us between publish and arrival
    })
    remoteio.datamodel.NewDigitalOutput.onConnectionChange(() => {
        remoteio.datamodel.NewDigitalOutput.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });
*/

const ipc = require('node-ipc').default;

let analogInputs = [];
let digitalInputs = [];
let digitalOutputs = [];
let latencyPlot = {x :[], y: []};

//connection state changes
remoteio.onConnectionChange(() => {
    switch (remoteio.connectionState) {
    case "Connected":
        break;
    case "Operational":
        registerFramework();
        break;
    case "Disconnected":
        //disconnect from the framework
        if(ipc.of.framework) {
            ipc.disconnect('framework');
        }
        //clear all local lists
        analogInputs = [];
        digitalInputs = [];
        digitalOutputs = [];
        break;
    case "Aborted":
        break;
    }
});

//value change events
remoteio.datamodel.AnalogInput.onChange(() => {
    //store the current value in the analoginput list
    for(let i=0; i<analogInputs.length; i++) {
        if(analogInputs[i].name == remoteio.datamodel.AnalogInput.value.Name) {
            analogInputs[i].value = remoteio.datamodel.AnalogInput.value.State;
        }
    }

    //update the latency
    for(let i=0; i<latencyPlot.x.length; i++) {
        if(latencyPlot.x[i] == remoteio.datamodel.AnalogInput.value.Name) {
            latencyPlot.y[i] = remoteio.datamodel.AnalogInput.latency;
        }
    }

    //update the analog input values in the framework
    if(ipc.of.framework && analogInputs.length > 0) {
        ipc.of.framework.emit(
            'message',
            {
                update: {
                    name: "RemoteIO",
                    numeric : analogInputs,
                    plot : [
                        {
                            name : "Latency",
                            data : [
                                {
                                    x : latencyPlot.x,
                                    y : latencyPlot.y,
                                    type : "bar"
                                }
                            ],
                            layout : {
                                title: "Latency (us)",
                                height: 300
                            }
                        }
                    ]
                }
            }
        );
    }
});
remoteio.datamodel.DigitalInput.onChange(() => {
    //store the current value in the digitalinput list
    for(let i=0; i<digitalInputs.length; i++) {
        if(digitalInputs[i].name == remoteio.datamodel.DigitalInput.value.Name) {
            digitalInputs[i].value = remoteio.datamodel.DigitalInput.value.State;
        }
    }

    //update the latency
    for(let i=0; i<latencyPlot.x.length; i++) {
        if(latencyPlot.x[i] == remoteio.datamodel.DigitalInput.value.Name) {
            latencyPlot.y[i] = remoteio.datamodel.DigitalInput.latency;
        }
    }

    //if we find a digital output with the same name as the input, we reflect the value back
    for(let i=0; i<digitalOutputs.length; i++) {
        if(digitalOutputs[i].name == remoteio.datamodel.DigitalInput.value.Name) {
            digitalOutputs[i].value = remoteio.datamodel.DigitalInput.value.State;

            remoteio.datamodel.DigitalOutput.value.Name = digitalOutputs[i].name;
            remoteio.datamodel.DigitalOutput.value.State = digitalOutputs[i].value;
            remoteio.datamodel.DigitalOutput.publish();
        }
    }

    //update the digital input values in the framework
    if(ipc.of.framework) {
        ipc.of.framework.emit(
            'message',
            {
                update: {
                    name: "RemoteIO",
                    digital : digitalInputs,
                    plot : [
                        {
                            name : "Latency",
                            data : [
                                {
                                    x : latencyPlot.x,
                                    y : latencyPlot.y,
                                    type : "bar"
                                }
                            ],
                            layout : {
                                title: "Latency (us)",
                                height: 300
                            }
                        }
                    ]
                }
            }
        );
    }
});
remoteio.datamodel.NewAnalogInput.onChange(() => {
    //add the analog input to the analog inputs list
    remoteio.log.success(`added AI ${remoteio.datamodel.NewAnalogInput.value}`);
    analogInputs.push({
        name : remoteio.datamodel.NewAnalogInput.value,
        value : 0
    });
    latencyPlot.x.push(remoteio.datamodel.NewAnalogInput.value);
    latencyPlot.y.push(0);
    //if this is done during operation, register in the framework
    if(remoteio.isOperational) {
        registerFramework();
    }
});
remoteio.datamodel.NewDigitalInput.onChange(() => {
    //add the digital input to the digital inputs list
    remoteio.log.success(`added DI ${remoteio.datamodel.NewDigitalInput.value}`);
    digitalInputs.push({
        name : remoteio.datamodel.NewDigitalInput.value,
        value : false
    })
    latencyPlot.x.push(remoteio.datamodel.NewDigitalInput.value);
    latencyPlot.y.push(0);
    //if this is done during operation, register in the framework
    if(remoteio.isOperational) {
        registerFramework();
    }
});
remoteio.datamodel.NewDigitalOutput.onChange(() => {
    //add the digital output to the digital outputs list
    remoteio.log.success(`added DO ${remoteio.datamodel.NewDigitalOutput.value}`);
    
    digitalOutputs.push({
        name : remoteio.datamodel.NewDigitalOutput.value,
        value : false
    })
    //if this is done during operation, register in the framework
    if(remoteio.isOperational) {
        registerFramework();
    }
});

remoteio.onProcessed(() => {

});

function registerFramework() {

    if(ipc.of.framework) {
        ipc.disconnect('framework');
    }

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
                                name: "RemoteIO",
                                digital : digitalInputs,
                                numeric : analogInputs,
                                plot : [
                                    {
                                        name : "Latency",
                                        data : [
                                            {
                                                x : latencyPlot.x,
                                                y : latencyPlot.y,
                                                type : "bar"
                                            }
                                        ],
                                        layout : {
                                            title: "Latency (us)",
                                            height: 300
                                        }
                                    }
                                ]
                            }
                        }
                    );
                }
            );
            ipc.of.framework.on(
                'disconnect',
                function(){
                    ipc.log('disconnected from framework'.notice);
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
}