## tappo
What the name syas.

# Network setup

Setup the adrresses:
``` 
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
sudo ip -6 r add <tap0 address> dev tapbr0
```

#Run the message broker

Git clone rsmb;
In a terminal enter the cloned directory then:
```
sudo nano config.conf

```
And paste the config file

```
# add some debug output
trace_output protocol

# listen for MQTT-SN traffic on UDP port 1885
listener 1885 INADDR_ANY mqtts
  ipv6 true

# listen to MQTT connections on tcp port 1886
listener 1886 INADDR_ANY
  ipv6 true

```
Save the file and then issue:
```
./mosquitto.rsmb/rsmb/src/broker_mqtts config.conf
```

# Make
