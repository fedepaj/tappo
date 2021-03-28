#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "shell.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#include "xtimer.h"
#include "srf04.h"
#include "srf04_params.h"

#ifndef EMCUTE_ID
#define EMCUTE_ID           ("gertrud")  //Putting long names currently may cause buffer overflow
#endif
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define _IPV6_DEFAULT_PREFIX_LEN        (64U)

#define NUMOFSUBS           (16U)
#define TOPIC_MAXLEN        (64U)

static char stack[THREAD_STACKSIZE_DEFAULT];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

gpio_t trigger_pin = GPIO_PIN(PORT_A, 10); //D2
gpio_t echo_pin = GPIO_PIN(PORT_B, 3); //D3
gpio_t alert_pin = GPIO_PIN(PORT_B, 5);
gpio_t led_pin = GPIO_PIN(PORT_B,4);
gpio_t operative_pin = GPIO_PIN(PORT_A, 2);

int perc=0;
int onTop=0;
int isReferenceMeasure=0;
int margin=50; //Default margin
static void *emcute_thread(void *arg)
{
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    /* should never be reached */
}

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

static void on_pub(const emcute_topic_t *topic, void *data, size_t len){
    char *in = (char *)data;
    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    for (size_t i = 0; i < len; i++) {
        printf("%c", in[i]);
    }
    puts("");
}

static int pub(char* topic, const char* data, int qos){
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;

    switch(qos){
        case 1:
            flags |= EMCUTE_QOS_1;
            break;
        case 2:
            flags |= EMCUTE_QOS_2;
            break;
        default:
            flags |= EMCUTE_QOS_0;
            break;

    }

    t.name = MQTT_TOPIC;
    if(emcute_reg(&t) != EMCUTE_OK){
        puts("PUB ERROR: Unable to obtain Topic ID");
        return 1;
    }
    if(emcute_pub(&t, data, strlen(data), flags) != EMCUTE_OK){
        printf("PUB ERROR: unable to publish data to topic '%s [%i]'\n", t.name, (int)t.id);
        return 1;
    }

    printf("PUB SUCCESS: Published %s on topic %s\n", data, topic);
    return 0;
}

int measure_distance(srf04_t *dev){
    if(srf04_read(dev) == SRF04_ERR_INVALID){
        printf("No valid measurement is available!\n");
        return -1;
    }
    int distance = srf04_get_distance(dev);
    //printf("SRF04 distance: %d mm\n", distance);
    return distance;
}

int setup_mqtt(void)
{
    /* initialize our subscription buffers */
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    /* start the emcute thread */
    thread_create(stack, sizeof(stack), EMCUTE_PRIO, 0, emcute_thread, NULL, "emcute");
    //Adding address to network interface
    netif_add("4","fec0:affe::99");
    // connect to MQTT-SN broker
    printf("Connecting to MQTT-SN broker %s port %d.\n", SERVER_ADDR, SERVER_PORT);

    sock_udp_ep_t gw = {
        .family = AF_INET6,
        .port = SERVER_PORT
    };
    
    char *message = "connected";
    size_t len = strlen(message);

    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, SERVER_ADDR) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }

    if (emcute_con(&gw, true, MQTT_TOPIC, message, len, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", SERVER_ADDR, (int)gw.port);
        return 1;
    }

    printf("Successfully connected to gateway at [%s]:%i\n", SERVER_ADDR, (int)gw.port);

    // setup subscription to topic
    
    unsigned flags = EMCUTE_QOS_0;
    subscriptions[0].cb = on_pub;
    strcpy(topics[0], MQTT_TOPIC_IN);
    subscriptions[0].topic.name =MQTT_TOPIC_IN;

    if (emcute_sub(&subscriptions[0], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", MQTT_TOPIC_IN);
        return 1;
    }

    printf("Now subscribed to %s\n", MQTT_TOPIC_IN);
    
    return 0;
}

void switch_cb(void *arg){
    puts(arg);
    char str[40];
    int val = gpio_read(operative_pin);
    if(val){
        onTop=1;
        isReferenceMeasure=1;
        sprintf(str, "{\"id\":\"%s\",\"active\":\"1\"}}",EMCUTE_ID);
        pub(MQTT_TOPIC, str, 0);
    } else {
        onTop=0;
        perc=0;
        sprintf(str, "{\"id\":\"%s\",\"active\":\"0\"}}",EMCUTE_ID);
        pub(MQTT_TOPIC, str, 0);
    }

}

void make_sound(void){
    gpio_set(alert_pin);
    xtimer_usleep(200);
    gpio_clear(alert_pin);
    xtimer_usleep(200);
}

int getNextWakeTime(void){
    return 3;
}

int main(void){
    int distance=0;
    int reference=0;
    char str[40];
    printf("Setting up MQTT-SN.\n");
    setup_mqtt();
    srf04_t dev;
    srf04_params_t my_params;
    my_params.trigger = trigger_pin;
    my_params.echo = echo_pin;

    if(srf04_init(&dev, &my_params) != SRF04_OK){
        printf("Failed to setup SRF04");
    }
    gpio_init_int(operative_pin,GPIO_IN,GPIO_BOTH, &switch_cb, NULL);
    gpio_init(led_pin,GPIO_OUT);
    xtimer_sleep(3);
    while(1) {
        if(onTop){
            printf("[MQTT] Publishing data to MQTT Broker\n");
            distance=srf04_get_distance(&dev);
            if(isReferenceMeasure){
                reference=distance-margin; //The tank is considered full when there is some margin
                isReferenceMeasure=0;
            }
            perc=(1-(distance/reference))*100;
            if(perc>=0){
                sprintf(str, "{\"id\":\"%s\",\"perc\":\"%d\"}", EMCUTE_ID, perc);
                pub(MQTT_TOPIC, str, 0);
                if(perc>=100){
                    make_sound();
                    gpio_set(led_pin);
                }
            }
            xtimer_sleep(getNextWakeTime());
        }
    }
}