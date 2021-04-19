# tappo
Is a safety oriented smart cap tat senses liquid level.

It's build manly for plastic tanks as are the ones used for axample in the following senarios:
- My father, who need to refill acquarium's wather, and need to shut the distilled water valve before the tank is full.
- Waste water produced by air conditioning system is stored in plastic tanks that need to be periodically emptyed.

When the device is inserted, it senses it and starts measuring the distance between the liquid and itself returning the fill percentage of the container based on the fist measure that it makes.
When a percentage near 100% is reached the device will alert the user via it's actuators.

## Hardware
To sense the insertion a switch is used, an ultrasonic sensor is used to measure the liquid quantity and to alert the user a led and a buzzer are used. 
To see how the hardware is put togheder please refer to [this file]().

### Nucleo board

## On linux
### Make the nucleo network work
To make this work basically following [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-riot-native), so:

```sh 
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
```

### Run the MQTT-SN broker

This is pretty straight forward following [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-a-broker) and caring to add at the bottom of the conf file the part relative to the transparent bridge.
```
connection local_bridge_to_mosquitto
  address 127.0.0.1:1883
  topic active out
  topic measures out
  topic commands in
```

### Bridge between MQTT-SN and MQTT
We will use mosquitto like [here](https://aws.amazon.com/it/blogs/iot/how-to-bridge-mosquitto-mqtt-broker-to-aws-iot/).
To begin we need the following from Amazon:
* root certificate 
* PEM encoded client certificate
* client private key
Now we need to specify in a mosquitto config file the adress of the AWT IoT core service and the topics, and their specific diretctions, that we want to bridge.
To then run mosquitto with the config file in the current directory, after copying the AWS certificates `/etc/mosquitto/certs/`, as stated in the link above, we can also stop the service and issue the command with flag `-c` followed by the name of the file. 

## AWS

### IoT Core

### Gateway Api

## Dashboard

## Final results