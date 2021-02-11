# HowTo Setup TXT Controller
This How-To describes the steps how to setup a new TXT controller of the fischertechnik model *Training Factory Industry 4.0 24V*.

> If you have any questions, please contact: fischertechnik-technik@fischer.de

## 1. TXT Settings
Change TXT settings:
  - **Role**: Online: Cloud-Client, Offline: MQTT Broker
  - **Security settings**: Enable WEB Server and SSH Daemon
  - **Network settings**: disable Bluetooth, activate WLAN Client, setup [network WLAN settings](Network_Config.md) for the TXT
  
Update Client reservation on Nano Router:
  - **Nano Router**: open 192.168.0.252 in WEB browser user:admin, password:admin, edit MAC in address reservation

## 2. Deploy C Programs
Use TXT [WEB server](WEBServer.md) to copy files from PC to the TXT controller
  - Online: Copy C program "TxtGatewayOfflinePLC" to the folder "Cloud"
  - Offline: Copy C program "TxtGatewayPLC.cloud" to the folder "C-Program"
Set *AutoLoad* for the program

## 3. Power Off and On
Switch off and on the TXT controller in the training model.

## 4. Start and Check
Start the factory.

# Hints
- The version of the TXT firmware can be found on the TXT controller in the menue `Settings -> info`
- The version of the ROBO Pro can be found in menue `Help -> About...`
