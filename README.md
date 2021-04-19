# tappo
Is a safety oriented smart cap tat senses liquid level.

It's build manly for plastic tanks as are the ones used in the targeted senarios:
- To help my father, that needs to fill distilled water tanks to refill the acquarium, know when to shut the valve to avoid a tank overflow.
- To help monitor waste water produced by air conditioning system stored in plastic tanks.

When the device is inserted, it senses it and starts measuring the distance between the liquid and itself.
It uses the first measure as a reference to compute a value between 0 and 100 representing the "fill percentage" of the tank.
If a critical percentage is reached the device will alert the user.

## Nucleo
Refer to [this file]() to get more on how the hardware is builted and to [this one]() to see some code insights.

## Setup MQTT stuff

### Configure the MQTT-SN broker
This is pretty straight forward following [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-a-broker) and caring to add at the bottom of the conf file the part relative to the transparent bridge.

```
connection local_bridge_to_mosquitto
  address 127.0.0.1:1883
  topic active out
  topic measures out
  topic led both
  topic buzz both

```

### Bridge between MQTT-SN and MQTT
We will use mosquitto like [here](https://aws.amazon.com/it/blogs/iot/how-to-bridge-mosquitto-mqtt-broker-to-aws-iot/).
To begin we need the following from Amazon:

* root certificate 
* PEM encoded client certificate
* client private key

Now we need to specify in a mosquitto config file the adress of the AWT IoT core service and the topics, and their specific diretctions, that we want to bridge and copy the AWS certificates in `/etc/mosquitto/certs/`.

## How to run
Assumptions made in the following section:
* we cloned [RIOT](https://github.com/RIOT-OS/RIOT) and [mosquitto.rsmb](https://github.com/eclipse/mosquitto.rsmb) in the main directory of this project;
* we are in the project directory.
* 
### Make the nucleo network work
To make this work basically following [emcute_mqttsn example](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-riot-native), so:

```sh
sudo ./RIOT/dist/tools/tapsetup/tapsetup -d
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
```

### Run the brokers
To run MQTT-SN rsmb broker we issue
```sh
./mosquitto.rsmb/rsmb/src/broker_mqtts rsmb_config.conf
```

Then to run mosquitto with the config file in the current directory we can stop the service and issue:
```sh
mosquitto -c mosquitto_bridge.conf
```

### Run the firmware
```sh
make BUILD_IN_DOCKER=1 BOARD=nucleo-f401re flash term
```

## AWS

### IoT Core

### Gateway Api

## Dashboard

## Final results
