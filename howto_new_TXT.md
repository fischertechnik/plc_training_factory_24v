# HowTo Setup TXT Controller
This How-To describes the steps how to setup a new TXT controller of the fischertechnik model *Training Factory Industry 4.0 24V*.

> If you have any questions, please contact: fischertechnik-technik@fischer.de

## 1. Replace TXT Controller
  - Remove connections
  - Remove old TXT controller
  - Install new TXT controller
  - Restore connections (see picture)
  
![txt_new](doc/TXT_new.png "TXT new")

## 2. TXT Settings
Change TXT settings:
  - **Role**: Online: Cloud-Client, Offline: MQTT Broker
  - **Security settings**: Enable WEB Server and SSH Daemon
  - **Network settings**: disable Bluetooth, activate WLAN Client, setup DHCP (see analogous to [network WLAN settings](https://github.com/fischertechnik/txt_training_factory/blob/master/doc/Network_Config.md)
  
Update Client address reservation on Nano Router:
  - **Nano Router**: open 192.168.0.252 in WEB browser user:admin, password:admin, edit MAC in address reservation

## 3. Deploy C Programs
Use TXT [WEB server](https://github.com/fischertechnik/txt_training_factory/blob/master/doc/WEBServer.md) to copy files from PC to the TXT controller
  - Online: Copy C program "TxtGatewayPLC.cloud" to the folder "**Cloud**"
  - Offline: Copy C program "TxtGatewayOfflinePLC" to the folder "**C-Program**"
Set *AutoLoad* for the program

## 4. Power Off and On
Switch off and on the TXT controller in the training model.

## 5. Start and Check
Start the factory.

# Hints
- The version of the TXT firmware can be found on the TXT controller in the menue `Settings -> info`
- The version of the ROBO Pro can be found in menue `Help -> About...`
