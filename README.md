# tappo
Is a safety oriented smart lid for plastic tanks.

The device is placed on the neck of the tank and measures fill percentage to inform the user when it has reached the desired level and warn the user the user tor stop the flow.

## Ultrasonic range finder sr04
When the lid is on Is used to determine the distance between the water and the neck of the tank. 

The device gets a reference masure of the distance between the sensor and the level of the bottom of the tank or the level of already present fluid.

The difference between the reference vaule and the subsequent distance's samples defines the fill percentage that will be sended via the `measures` topic to the broker.

## The switch
The role of the switch is to decect when the lid has been placed on the neck of the tank and could be any sort of switch.

The switch chosen is [this one](http://smparts.com/product_info.php?cPath=2_602&products_id=6689),a lever switch with a little wheel on top of it.
Is placed in the circuit like [this](https://killerrobotics.files.wordpress.com/2015/09/lever-switch_bb.png).

The switch it raises an interruppt that publishes the activeness of the lid to the `active` topic and let the device stop measuring the fill percentage.

## Case
Is the one that holds the sensors together and makes it a perfect lid for a 43mm 30L plastic tank that will be filled with 15mm tubing.

## Piezo buzzer
Informs the user that the level of the water has reached the desired level with an acoustic signal.

## Led
Is surely a visual cue but could really be switched by a relay.

## Network setup (NUCLEO side)

For me networking didn't work out of the box so I've added two functions
looking at the ones in RIOT's [sc_gnrc_netif.c](https://github.com/RIOT-OS/RIOT/blob/2453b68249d09ff680f3ea85343e26851d00da67/sys/shell/commands/sc_gnrc_netif.c) to mimic the behavior of the `ifconfig` command in [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn) (that was the only way to connect to broker).

First we define:
```c
#define _IPV6_DEFAULT_PREFIX_LEN        (64U)
```
Then we have:
```c
static uint8_t get_prefix_len(char *addr)
{
    int prefix_len = ipv6_addr_split_int(addr, '/', _IPV6_DEFAULT_PREFIX_LEN);

    if (prefix_len < 1) {
        prefix_len = _IPV6_DEFAULT_PREFIX_LEN;
    }

    return prefix_len;
}



static int netif_add(char *iface_name,char *addr_str)
{

    netif_t *iface = netif_get_by_name(iface_name);
        if (!iface) {
            puts("error: invalid interface given");
            return 1;
        }
    enum {
        _UNICAST = 0,
        _ANYCAST
    } type = _UNICAST;
    
    ipv6_addr_t addr;
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID;
    uint8_t prefix_len;


    prefix_len = get_prefix_len(addr_str);

    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("error: unable to parse IPv6 address.");
        return 1;
    }

    if (ipv6_addr_is_multicast(&addr)) {
        if (netif_set_opt(iface, NETOPT_IPV6_GROUP, 0, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to join IPv6 multicast group\n");
            return 1;
        }
    }
    else {
        if (type == _ANYCAST) {
            flags |= GNRC_NETIF_IPV6_ADDRS_FLAGS_ANYCAST;
        }
        flags |= (prefix_len << 8U);
        if (netif_set_opt(iface, NETOPT_IPV6_ADDR, flags, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to add IPv6 address\n");
            return 1;
        }
    }

    printf("success: added %s/%d to interface ", addr_str, prefix_len);
    printf("\n");

    return 0;

}
```
Finally we can call `netif_add` in `main` as:
```c
netif_add("4","fec0:affe::99");
```
Last but not least we need to add the following in `Makefile.ethos.conf` to let RIOT add a third entry tho the network interface:
```make
CFLAGS += -DCONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF=3
```
and in `Makefile`:
```make
SERVER_ADDR = fec0:affe::1
```
## Networking (Linux side)
To make this work basically following [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-riot-native), so:

```sh 
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
```

## Run the MQTT-SN broker

This is pretty straight forward following [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-a-broker) and caring to add at the bottom of the conf file the part relative to the transparent bridge.
```
connection local_bridge_to_mosquitto
  address 127.0.0.1:1883
  topic active out
  topic measures out
  topic commands in
```

## Bridge between MQTT-SN and MQTT
We will use mosquitto like [here](https://aws.amazon.com/it/blogs/iot/how-to-bridge-mosquitto-mqtt-broker-to-aws-iot/).
To begin we need the following from Amazon:
* root certificate 
* PEM encoded client certificate
* client private key
Now we need to specify in a mosquitto config file the adress of the AWT IoT core service and the topics, and their specific diretctions, that we want to bridge.
To then run mosquitto with the config file in the current directory, after copying the AWS certificates `/etc/mosquitto/certs/`, as stated in the link above, we can also stop the service and issue the command with flag `-c` followed by the name of the file. 
