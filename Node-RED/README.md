## IOT Gateway (Raspberry Pi 4)

### Hardware
- [Raspberry Pi 4 Model B](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/specifications/)
- [Strom Pi 3](https://joy-it.net/en/products/RB-StromPi3)

The SD card image for Raspberry Pi 4 can be downloaded [here](https://github.com/fischertechnik/plc_training_factory_24v/releases/download/V04/2021-06-25-lite-IOTpi2.zip)

Please use e. g. the tool *Win32DiskImager* to write the Raspberry Pi 4 image to a >=4GB micro SD card. This micro SD card image is ready to use. The flows contain also a local dashboard for *Training Factory Industry 4.0 24V*.

### Login data SSH console Raspberry Pi 4
- **user:** pi
- **password:** ft-IOTpi2

## Node-RED

[Node-RED](https://nodered.org/) is a powerful tool for connecting various hardware and APIs. In this project Node-RED is used to convert MQTT <--> OPC/UA.

Here you can find the [flows](flows_IOTpi2.json) for the Node-RED environment (see [HowTo](https://nodered.org/docs/user-guide/editor/workspace/import-export)).

Node-RED dependencies:
  * [node-red-dashboard](https://flows.nodered.org/node/node-red-dashboard)
