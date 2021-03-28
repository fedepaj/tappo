# tappo
What the name syas.

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
To make this work basically we need to follow [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-riot-native), so:

```sh 
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
```

## Run the MQTT-SN broker

This is pretty straight forward if we follow [emcute_mqttsn](https://github.com/RIOT-OS/RIOT/tree/master/examples/emcute_mqttsn#setting-up-a-broker).

## Make
