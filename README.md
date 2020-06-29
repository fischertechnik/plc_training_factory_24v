> If you have any questions, please contact fischertechnik-technik@fischer.de

# fischertechnik Training Factory Industry 4.0 24V (en)
This project contains the PLC programs, IOT Gateway SD card image and the TXT Gateway C program for the fischertechnik [**Training Factory Industry 4.0 24V**](https://www.fischertechnik.de/en/products/simulating/training-models/554868-sim-training-factory-industry-4-0-24v-simulation).

## Overview
The factory consists of the following stations:
* **SSC**: Sensor Station with Camera
* **HBW**: High-Bay Warehouse
* **VGR**: Vacuum Gripper Robot
* **DPS**: Delivery and Pickup Station
* **MPO**: Multi-Processing Station with Oven
* **SLD**: Sorting Line with Color Detection

## Programs
The default user PLC programs implement the standard demo scenario with the [www.fischertechnik-cloud.com](https://www.fischertechnik-cloud.com). These programs you will find in the [TxtGatewayPLC](https://github.com/fischertechnik/plc_training_factory_24v/tree/master/TxtGatewayPLC) folder:
* **PLC_S7_1500** (PLC S7 1500 project, TIA Portal v16)
* **PLC_S7_1500_sources** (PLC sources as SCL Structured Control Language)
* **PLC_S7_1500_exercises** (PLC S7 1500 exercises, TIA Portal v16)
* **TxtGatewayPLC.cloud** (C program for TXT controller)

## Network
The next picture shows the [network](doc/Network_Config.md) overview with the TXT controllers.
![Overview Network](doc/Overview_Network.PNG "Overview Network")

## MQTT Interface
The [MQTT Interface](TxtSmartFactoryLib/doc/MqttInterface.md) describes the topics and the payload of the MQTT clients and the configuration of the mosquitto MQTT bridge. 

## Node-RED
An example for  Training Factory Industry 4.0 24V and [Node-RED](https://nodered.org/) you can find in [Node-RED folder](Node-RED/README.md). 
