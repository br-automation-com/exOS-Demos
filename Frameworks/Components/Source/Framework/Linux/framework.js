const express = require('express');
const bodyParser = require('body-parser');
const ipc = require('node-ipc').default;
const port = 2000;

var stream = null;
let id = 0;

//list of all regstered plugins
let pluginRegData = [];

ipc.config.id   = 'framework';
ipc.config.retry= 1500;

ipc.serve(
    function(){
        ipc.server.on(
            'message',
            function(data,socket){

                if(data.plugin && data.plugin.name) {
                    pluginRegData.push({plugin: data.plugin, socket:socket});
                }

                if (stream) {
                    let message = JSON.stringify(data);
                    stream.write(`id: ${id++}\n`);
                    stream.write(`type: message\n`);
                    stream.write(`data: ${message}\n\n`);
                }
            }
        );
        ipc.server.on(
            'socket.disconnected',
            function(socket, destroyedSocketID) {
                console.log('client ' + destroyedSocketID + ' has disconnected!');
                let i = pluginRegData.length;
               
                while(i--) {
                    if(pluginRegData[i].socket == socket) {
                        pluginRegData.splice(i,1);
                    }
                }
                if (stream) {
                    let message = JSON.stringify({ reload: true});
                    stream.write(`id: ${id++}\n`);
                    stream.write(`type: message\n`);
                    stream.write(`data: ${message}\n\n`);
                }
            }
        );
    }
);

ipc.server.start();

const www = express();
www.use(bodyParser.urlencoded({ extended: true }));
www.use(bodyParser.json());

www.get('/inputStream', (req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive',
    });
    res.write('\n');
    stream = res;

    //each time the page reloads, the plugin registrations need to be resent
    for(let regData of pluginRegData) {
        let message = JSON.stringify(regData);
        stream.write(`id: ${id++}\n`);
        stream.write(`type: message\n`);
        stream.write(`data: ${message}\n\n`);
    }
})

www.get('/', (req, res) => {
    res.sendFile(__dirname + '/www/index.html');
});

www.use(express.static(__dirname+ '/www'));

www.listen(port, () => console.log(`server running on port ${port}!`))