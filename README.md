# tappo
Is a safety oriented smart cap that senses liquid level.

It's buildt manly for plastic tanks as are the ones used in the targeted senarios:
* monitor distilled water tanks, used to refill an acquarium, to know when to shut the valve to avoid a tank overflow;
* monitor waste water produced by air conditioning system often stored in plastic tanks.

When the device is inserted, it senses it and starts measuring the distance between the liquid and itself.

It uses the first measure as a reference to compute a value between 0 and 100 representing the "fill percentage" of the tank.

If a critical percentage is reached the device will alert the user.

## Nucleo
Refer to [this file](hardware/NUCLEO_HARDWARE.md) to get more on the nucleo hardware and to [this one](nucleo/NUCLEO.md) to see some code insights.

## Setup MQTT stuff

### Configure the MQTT-SN broker
This is pretty straight forward following [emcute_mqttsn example](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-a-broker) and caring to add at the bottom of the conf file the part relative to the transparent bridge.

```
connection local_bridge_to_mosquitto
  address 127.0.0.1:1883
  topic active out
  topic measures out
  topic led both
  topic buzz both
```

### Bridge between MQTT-SN and MQTT
We will use mosquitto like in [this aws blog post](https://aws.amazon.com/it/blogs/iot/how-to-bridge-mosquitto-mqtt-broker-to-aws-iot/).

To begin we need the following from Amazon:

* root certificate 
* PEM encoded client certificate
* client private key

Now we need to specify in a mosquitto config file the address of the AWT IoT core service, the topics that we want to bridge and their diretctions and copy the AWS certificates in `/etc/mosquitto/certs/`.

## How to run 
Assumptions made in this section:
* we cloned [RIOT](https://github.com/RIOT-OS/RIOT) and [mosquitto.rsmb](https://github.com/eclipse/mosquitto.rsmb) in the main directory of this project;
* we are in the project directory.

### Run the brokers
To run MQTT-SN rsmb broker we issue
```sh
./mosquitto.rsmb/rsmb/src/broker_mqtts rsmb_config.conf
```

Then to run mosquitto with the config file in the current directory we can stop the service and issue:
```sh
mosquitto -c mosquitto_bridge.conf
```

### Nucleo
#### Make the nucleo network work
As in the [emcute_mqttsn example](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-riot-native):

```sh
sudo ./RIOT/dist/tools/tapsetup/tapsetup -d
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
```

#### Run the nucleo firmware
```sh
make BUILD_IN_DOCKER=1 BOARD=nucleo-f401re flash term
```


## AWS
![NetOverview](https://github.com/fedepaj/tappo/blob/assets/network_overview_tappo.jpg)

### IoT Core
Once data from the devices is received from Iot Core data from the 4 different topics is parsed by 4 diffent rules and stored in 4 different tables (I don't know if this is the best approch) based on device id and message timestamp.

### Gateway Api
Once data is asked from the dashboard a lambda function for each endpoint is triggered.
To retrive data we access DynamoDB, to toggle the actuators we publish to the relatives MQTT topics.

## Dashboard
You can find tappo's dashboard [here](https://github.com/fedepaj/tappo_app/).

## Final results

