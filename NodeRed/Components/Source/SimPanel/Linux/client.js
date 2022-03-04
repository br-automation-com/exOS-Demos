const mqtt = require('mqtt')
let simpanel = require('./simpanel').simpanel;

const client  = mqtt.connect('mqtt://localhost')

function send() {
    //send everything out, even though it might not be used
    let obj = {
        Switches: simpanel.datamodel.Switches.value,
        Buttons: simpanel.datamodel.Buttons.value,
        LEDs: simpanel.datamodel.LEDs.value,
        Knobs: simpanel.datamodel.Knobs.value,
        Display: simpanel.datamodel.Display.value,
        Encoder: simpanel.datamodel.Encoder.value
    }
    let msg = JSON.stringify(obj);
    client.publish('simpanel/current', msg);
}

client.on('connect', function () {
    client.subscribe('simpanel/updated');
})

client.on('message', function (topic, message) {
    //we only subscribe to one topic, which contains the complete (maybe partly modified) object we send out
    let obj = JSON.parse(message);
    simpanel.datamodel.Display.value = obj.Display || 0;
    simpanel.datamodel.Display.publish();
});

simpanel.onConnectionChange(() => {
    switch (simpanel.connectionState) {
        case "Connected":
            break;
        case "Operational":
            send();          
            break;
        case "Disconnected":
            break;
        case "Aborted":
            break;
    }
});

simpanel.datamodel.Switches.onChange(() => {
    //light up the switches according to their states
    simpanel.datamodel.LEDs.value.DI1 = simpanel.datamodel.Switches.value.DI1;
    simpanel.datamodel.LEDs.value.DI2 = simpanel.datamodel.Switches.value.DI2;
    simpanel.datamodel.LEDs.publish();
});

simpanel.datamodel.Buttons.onChange(() => {
    simpanel.datamodel.LEDs.value.DI3.Green = simpanel.datamodel.Buttons.value.DI3;
    simpanel.datamodel.LEDs.publish();
    send();
});
simpanel.datamodel.Knobs.onChange(() => {
    
});
simpanel.datamodel.Encoder.onChange(() => {
    send();
});

//Cyclic call triggered from the Component Server
simpanel.onProcessed(() => {
    //Publish values
    //if (simpanel.isConnected) {
        //simpanel.datamodel.LEDs.value = ..
        //simpanel.datamodel.LEDs.publish();
        //simpanel.datamodel.Display.value = ..
        //simpanel.datamodel.Display.publish();
    //}
});