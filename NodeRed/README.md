# Using exOS with node-red

This application showcases how you can use exOS to deploy a complete node-red environment including an application that starts up on the Linux target, and have it exchange data with AR via a second exOS component.

The component that exchanges data between Linux and AR (SimPanel) represents the I/Os of a simulation panel (4SIM.10-1), and controls a nodejs application in Linux using mqtt to interact with node-red.

![][img_layout]

In the Components folder, Binary and Source components are found together with an I/O-mapping file for SimPanel demo.

## Prerequisites

The demo requires the following setup

- Node v.16 - use [nodesource][link_nodesource] to install the correct version
- MQTT broker installed on the target system (e.g. mosquitto) listening on port 1883
- Port 1880 opened from Linux 

    - This is needed if the windows host should be able to interact with the node-red application

    - In case the Linux system uses a firewall, open the port with
        
            ufw allow 1880


The node-red application takes in the entire SimPanel structure and modifies the parts of the structure before sending it back to the Linux side of the SimPanel exOS component.

![][img_noderedapp]

[img_noderedapp]: images/NodeRedApp.png
[img_layout]: images/NodeRed.png
[link_nodesource]: https://github.com/nodesource/distributions/blob/master/README.md