## Raspberry Pi 4

**user:** pi

**password:** ft-IOTpi2

## Node-RED

[Node-RED](https://nodered.org/) is a powerful tool for connecting various hardware and APIs. In this project Node-RED is used to convert MQTT <--> OPC/UA.

Please use e. g. the tool *Win32DiskImager* to write the [Raspberry Pi 4 Image V00](https://github.com/fischertechnik/plc_training_factory_24v/releases/download/V00/2020-07-13-lite-IOTpi2.zip) to a >=4GB micro SD card. This micro SD card image is ready to use. The flows contain also a local dashboard for *Training Factory Industry 4.0 24V*.

Here you can find the [flows](flows_IOTpi2.json) for the Node-RED environment (see [HowTo](https://nodered.org/docs/user-guide/editor/workspace/import-export)).

Node-RED dependencies:
  * [node-red-dashboard](https://flows.nodered.org/node/node-red-dashboard)

