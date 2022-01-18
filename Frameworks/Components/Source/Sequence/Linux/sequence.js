/**
 * @callback SequenceDataModelCallback
 * @returns {function()}
 * 
 * @typedef {Object} SequenceButtonsDataSetValue
 * @property {boolean} ButtonLeft 
 * @property {boolean} ButtonRight 
 * 
 * @typedef {Object} SequenceLampsDataSetValue
 * @property {boolean} ButtonLeft 
 * @property {boolean} ButtonRight 
 * 
 * @typedef {Object} ButtonsDataSet
 * @property {SequenceButtonsDataSetValue} value 
 * @property {SequenceDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SequenceDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} StartDataSet
 * @property {boolean} value 
 * @property {SequenceDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SequenceDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} LampsDataSet
 * @property {SequenceLampsDataSetValue} value 
 * @property {function()} publish publish the value
 * @property {SequenceDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} ActiveDataSet
 * @property {boolean} value 
 * @property {function()} publish publish the value
 * @property {SequenceDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} SequenceDatamodel
 * @property {ButtonsDataSet} Buttons
 * @property {StartDataSet} Start
 * @property {LampsDataSet} Lamps
 * @property {ActiveDataSet} Active
 * 
 * @callback SequenceDatamodelLogMethod
 * @param {string} message
 * 
 * @typedef {Object} SequenceDatamodelLog
 * @property {SequenceDatamodelLogMethod} warning
 * @property {SequenceDatamodelLogMethod} success
 * @property {SequenceDatamodelLogMethod} info
 * @property {SequenceDatamodelLogMethod} debug
 * @property {SequenceDatamodelLogMethod} verbose
 * 
 * @typedef {Object} Sequence
 * @property {function():number} nettime get current nettime
 * @property {SequenceDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * @property {boolean} isConnected
 * @property {boolean} isOperational
 * @property {SequenceDatamodelLog} log
 * @property {SequenceDatamodel} datamodel
 * 
 */

/**
 * @type {Sequence}
 */
let sequence = require('./l_Sequence.node').Sequence;

/* datamodel features:

main methods:
    sequence.nettime() : (int32_t) get current nettime

state change events:
    sequence.onConnectionChange(() => {
        sequence.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted" 
    })

boolean values:
    sequence.isConnected
    sequence.isOperational

logging methods:
    sequence.log.error(string)
    sequence.log.warning(string)
    sequence.log.success(string)
    sequence.log.info(string)
    sequence.log.debug(string)
    sequence.log.verbose(string)

dataset Buttons:
    sequence.datamodel.Buttons.value : (SequenceButtons)  actual dataset values
    sequence.datamodel.Buttons.onChange(() => {
        sequence.datamodel.Buttons.value ...
        sequence.datamodel.Buttons.nettime : (int32_t) nettime @ time of publish
        sequence.datamodel.Buttons.latency : (int32_t) time in us between publish and arrival
    })
    sequence.datamodel.Buttons.onConnectionChange(() => {
        sequence.datamodel.Buttons.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Start:
    sequence.datamodel.Start.value : (bool)  actual dataset value
    sequence.datamodel.Start.onChange(() => {
        sequence.datamodel.Start.value ...
        sequence.datamodel.Start.nettime : (int32_t) nettime @ time of publish
        sequence.datamodel.Start.latency : (int32_t) time in us between publish and arrival
    })
    sequence.datamodel.Start.onConnectionChange(() => {
        sequence.datamodel.Start.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });


dataset Active:
    sequence.datamodel.Active.value : (bool)  actual dataset value
    sequence.datamodel.Active.publish()
    sequence.datamodel.Active.onConnectionChange(() => {
        sequence.datamodel.Active.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });
*/

const ipc = require('node-ipc').default;


ipc.config.id   = 'Sequence';
ipc.config.retry= 1500;

var sequenceStep = 0;
const SEQUENCE_MAX = 100

function runSequence() {
    
    sequence.datamodel.Active.value = true;
    sequence.datamodel.Active.publish();
    sequenceStep +=2;
    ipc.of.framework.emit(
        'message',
        {
            update: {
                name: "Sequence",
                plot : [
                    {
                        name : "Sequence Progress",
                        data : [
                            {
                                values : [SEQUENCE_MAX-sequenceStep, sequenceStep],
                                labels : ['Remaining', 'Finished'],
                                type : "pie",
                                name : "Sequence Progress",
                            }
                        ],
                        layout : {
                            title: "Sequence Progress",
                            height: 500
                        }
                    }
                ]
            }
        });
    if(sequenceStep < SEQUENCE_MAX) {
        setTimeout(runSequence, 100);
    }
    else {
        sequenceStep = 0;
        sequence.datamodel.Active.value = false;
        sequence.datamodel.Active.publish();
    }
}

//connection state changes
sequence.onConnectionChange(() => {
    switch (sequence.connectionState) {
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
                                    name: "Sequence",
                                    digital : [ 
                                        {
                                            name : "Left Button",
                                            value: false
                                        },
                                        {
                                            name : "Right Button",
                                            value: false
                                        }
                                    ],
                                    plot : [
                                        {
                                            name : "Sequence Progress",
                                            data : [
                                                {
                                                    values : [SEQUENCE_MAX-sequenceStep, sequenceStep],
                                                    labels : ['Remaining', 'Finished'],
                                                    type : "pie",
                                                    name : "Sequence Progress",
                                                }
                                            ],
                                            layout : {
                                                title: "Sequence Progress",
                                                height: 500
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
        break;
    case "Operational":
        break;
    case "Disconnected":
        //disconnect from the framework
        if(ipc.of.framework) {
            ipc.disconnect('framework');
        }
        sequenceStep = 0;
        break;
    case "Aborted":
        break;
    }
});
sequence.datamodel.Buttons.onConnectionChange(() => {
    // switch (sequence.datamodel.Buttons.connectionState) ...
});
sequence.datamodel.Start.onConnectionChange(() => {
    // switch (sequence.datamodel.Start.connectionState) ...
});
sequence.datamodel.Active.onConnectionChange(() => {
    // switch (sequence.datamodel.Active.connectionState) ...
});

//value change events
sequence.datamodel.Buttons.onChange(() => {
    ipc.of.framework.emit(
        'message',
        {
            update: {
                name: "Sequence",
                digital : [ 
                    {
                        name : "Left Button",
                        value: sequence.datamodel.Buttons.value.ButtonLeft
                    },
                    {
                        name : "Right Button",
                        value: sequence.datamodel.Buttons.value.ButtonRight
                    }
                ]
            }
        });
});
sequence.datamodel.Start.onChange(() => {
    if(sequence.datamodel.Start.value == true) {
        runSequence();
    }
});

//Cyclic call triggered from the Component Server
sequence.onProcessed(() => {
    //Publish values
    //if (sequence.isConnected) {
        //sequence.datamodel.Active.value = ..
        //sequence.datamodel.Active.publish();
    //}
});

