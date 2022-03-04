/**
 * @callback SimPanelDataModelCallback
 * @returns {function()}
 * 
 * @typedef {Object} SimPanelSwitchesDataSetValue
 * @property {boolean} DI1 
 * @property {boolean} DI2 
 * 
 * @typedef {Object} SimPanelButtonsDataSetValue
 * @property {boolean} DI3 
 * @property {boolean} DI4 
 * @property {boolean} DI5 
 * @property {boolean} DI6 
 * @property {boolean} Encoder 
 * 
 * @typedef {Object} SimPanelLEDsDI3DataSetValue
 * @property {boolean} Green 
 * @property {boolean} Red 
 * @property {boolean} Yellow 
 * 
 * @typedef {Object} SimPanelLEDsDI4DataSetValue
 * @property {boolean} Green 
 * @property {boolean} Red 
 * @property {boolean} Yellow 
 * 
 * @typedef {Object} SimPanelLEDsDI5DataSetValue
 * @property {boolean} Green 
 * @property {boolean} Red 
 * @property {boolean} Yellow 
 * 
 * @typedef {Object} SimPanelLEDsDI6DataSetValue
 * @property {boolean} Green 
 * @property {boolean} Red 
 * @property {boolean} Yellow 
 * 
 * @typedef {Object} SimPanelLEDsDataSetValue
 * @property {boolean} DI1 
 * @property {boolean} DI2 
 * @property {SimPanelLEDsDI3DataSetValue} DI3 
 * @property {SimPanelLEDsDI4DataSetValue} DI4 
 * @property {SimPanelLEDsDI5DataSetValue} DI5 
 * @property {SimPanelLEDsDI6DataSetValue} DI6 
 * 
 * @typedef {Object} SimPanelKnobsDataSetValue
 * @property {number} P1 
 * @property {number} P2 
 * 
 * @typedef {Object} SwitchesDataSet
 * @property {SimPanelSwitchesDataSetValue} value 
 * @property {SimPanelDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} ButtonsDataSet
 * @property {SimPanelButtonsDataSetValue} value 
 * @property {SimPanelDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} LEDsDataSet
 * @property {SimPanelLEDsDataSetValue} value 
 * @property {function()} publish publish the value
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} KnobsDataSet
 * @property {SimPanelKnobsDataSetValue} value 
 * @property {SimPanelDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} DisplayDataSet
 * @property {number} value 
 * @property {function()} publish publish the value
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} EncoderDataSet
 * @property {number} value 
 * @property {SimPanelDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} SimPanelDatamodel
 * @property {SwitchesDataSet} Switches
 * @property {ButtonsDataSet} Buttons
 * @property {LEDsDataSet} LEDs
 * @property {KnobsDataSet} Knobs
 * @property {DisplayDataSet} Display
 * @property {EncoderDataSet} Encoder
 * 
 * @callback SimPanelDatamodelLogMethod
 * @param {string} message
 * 
 * @typedef {Object} SimPanelDatamodelLog
 * @property {SimPanelDatamodelLogMethod} warning
 * @property {SimPanelDatamodelLogMethod} success
 * @property {SimPanelDatamodelLogMethod} info
 * @property {SimPanelDatamodelLogMethod} debug
 * @property {SimPanelDatamodelLogMethod} verbose
 * 
 * @typedef {Object} SimPanel
 * @property {function():number} nettime get current nettime
 * @property {SimPanelDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * @property {boolean} isConnected
 * @property {boolean} isOperational
 * @property {SimPanelDatamodelLog} log
 * @property {SimPanelDatamodel} datamodel
 * 
 */

/**
 * @type {SimPanel}
 */
let simpanel = require('./l_SimPanel.node').SimPanel;

/* datamodel features:

main methods:
    simpanel.nettime() : (int32_t) get current nettime

state change events:
    simpanel.onConnectionChange(() => {
        simpanel.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted" 
    })

boolean values:
    simpanel.isConnected
    simpanel.isOperational

logging methods:
    simpanel.log.error(string)
    simpanel.log.warning(string)
    simpanel.log.success(string)
    simpanel.log.info(string)
    simpanel.log.debug(string)
    simpanel.log.verbose(string)

dataset Switches:
    simpanel.datamodel.Switches.value : (SimPanelSwitches)  actual dataset values
    simpanel.datamodel.Switches.onChange(() => {
        simpanel.datamodel.Switches.value ...
        simpanel.datamodel.Switches.nettime : (int32_t) nettime @ time of publish
        simpanel.datamodel.Switches.latency : (int32_t) time in us between publish and arrival
    })
    simpanel.datamodel.Switches.onConnectionChange(() => {
        simpanel.datamodel.Switches.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Buttons:
    simpanel.datamodel.Buttons.value : (SimPanelButtons)  actual dataset values
    simpanel.datamodel.Buttons.onChange(() => {
        simpanel.datamodel.Buttons.value ...
        simpanel.datamodel.Buttons.nettime : (int32_t) nettime @ time of publish
        simpanel.datamodel.Buttons.latency : (int32_t) time in us between publish and arrival
    })
    simpanel.datamodel.Buttons.onConnectionChange(() => {
        simpanel.datamodel.Buttons.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset LEDs:
    simpanel.datamodel.LEDs.value : (SimPanelLEDs)  actual dataset values
    simpanel.datamodel.LEDs.publish()
    simpanel.datamodel.LEDs.onConnectionChange(() => {
        simpanel.datamodel.LEDs.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Knobs:
    simpanel.datamodel.Knobs.value : (SimPanelKnobs)  actual dataset values
    simpanel.datamodel.Knobs.onChange(() => {
        simpanel.datamodel.Knobs.value ...
        simpanel.datamodel.Knobs.nettime : (int32_t) nettime @ time of publish
        simpanel.datamodel.Knobs.latency : (int32_t) time in us between publish and arrival
    })
    simpanel.datamodel.Knobs.onConnectionChange(() => {
        simpanel.datamodel.Knobs.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Display:
    simpanel.datamodel.Display.value : (int16_t)  actual dataset value
    simpanel.datamodel.Display.publish()
    simpanel.datamodel.Display.onConnectionChange(() => {
        simpanel.datamodel.Display.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Encoder:
    simpanel.datamodel.Encoder.value : (uint16_t)  actual dataset value
    simpanel.datamodel.Encoder.onChange(() => {
        simpanel.datamodel.Encoder.value ...
        simpanel.datamodel.Encoder.nettime : (int32_t) nettime @ time of publish
        simpanel.datamodel.Encoder.latency : (int32_t) time in us between publish and arrival
    })
    simpanel.datamodel.Encoder.onConnectionChange(() => {
        simpanel.datamodel.Encoder.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });
*/

module.exports = { simpanel }