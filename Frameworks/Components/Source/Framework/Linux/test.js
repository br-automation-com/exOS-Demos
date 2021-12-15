const ipc = require('node-ipc').default;

let testValue = true;
let testNumeric = 123;
let testY = [10, 11, 10, 11];

function sendUpdate() {
    testValue = !testValue;
    testNumeric ++;
    for(i=0; i < testY.length; i++) {
        testY[i]+=i*0.1;
    }
    if(ipc.of.framework) {
        ipc.of.framework.emit(
            'message',
            {
                update: {
                    name: "test",
                    digital : [
                        {
                            name : "whoe",
                            value: testValue
                        },
                        {
                            name : "foo",
                            value: !testValue
                        }
                    ],
                    numeric : [
                        {
                            name : "encoder",
                            value: testNumeric
                        }
                    ],
                    plot : [
                        {
                            name : "chart",
                            data : [
                                {
                                    x : [ 1, 2, 3, 4],
                                    y : testY,
                                    type : "scatter"
                                }
                            ]
                        }
                    ]
                }          
            }
        )
    }
    setTimeout(sendUpdate, 800);
    
}

ipc.config.id   = 'test';
ipc.config.retry= 1500;

ipc.connectTo(
    'framework',
    function(){
        ipc.of.framework.on(
            'connect',
            function(){
                ipc.log('## connected to framework ##'.rainbow, ipc.config.delay);
                ipc.of.framework.emit(
                    'message',  //any event or message type your server listens for
                    {
                        plugin: {
                            name: "test",
                            digital : [ 
                                {
                                    name : "whoe",
                                    value: true
                                },
                                {
                                    name : "foo",
                                    value: false
                                }
                            ],
                            numeric : [
                                {
                                    name : "encoder",
                                    value: 123
                                }
                            ],
                            plot : [
                                {
                                    name : "chart",
                                    data : [
                                        {
                                            x : [ 1, 2, 3, 4],
                                            y : [10, 11, 10, 11],
                                            type : "scatter"
                                        }
                                    ],
                                    layout : {
                                        title: "chart",
                                        height: 300
                                    }
                                }
                            ]
                        }
                    }
                )

                setTimeout(sendUpdate, 2000);
            }
        );
        ipc.of.framework.on(
            'disconnect',
            function(){
                ipc.log('disconnected from framework'.notice);
            }
        );
        ipc.of.framework.on(
            'message',  //any event or message type your server listens for
            function(data){
                ipc.log('got a message from framework : '.debug, data);
            }
        );
    }
);