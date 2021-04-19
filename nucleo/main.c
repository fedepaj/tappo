#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xtimer.h"
#include "periph/gpio.h"
#include "periph/pwm.h"

//Ultrasonic distance sensor
#include "srf04.h"
#include "srf04_params.h"
#include "fmt.h"

//Networking
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"

//PWM
#define PWM_FREQ (494U)
#define PWM_RES (255U)

//Netwworking
#define _IPV6_DEFAULT_PREFIX_LEN        (64U)

//Emcute
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)
#define NUMOFSUBS           (16U)
#define TOPIC_MAXLEN        (64U)

static emcute_sub_t subscriptions[NUMOFSUBS];
static char stack[THREAD_STACKSIZE_MAIN];

gpio_t trigger_gpio = GPIO_PIN(PORT_A, 10); //D2
//D3 PWM
gpio_t echo_gpio = GPIO_PIN(PORT_B, 5); //D4
gpio_t led_gpio = GPIO_PIN(PORT_B, 4); //D5
//D6 PWM
gpio_t sw_gpio = GPIO_PIN(PORT_A,8); //D7

typedef enum state {
    STANDBY, REFERENCE, MEASURING, CLEAN, ALERT
}state;

state currState = STANDBY;
typedef enum alertstate {
    ALERTED, NOT_ALERTED
}alertstate;

alertstate currAlertS  = NOT_ALERTED; 
/*Prototypes*/

//Network prototypes
static uint8_t get_prefix_len(char *addr);
static int netif_add(char *iface_name,char *addr_str);

//MQTT-SN emcute prototypes
int setup_mqtt(void);
static void *emcute_thread(void *arg);
static int pub(char* topic, const char* data, int qos);
static void on_pub_buzz(const emcute_topic_t *topic, void *data, size_t len);
static void on_pub_led(const emcute_topic_t *topic, void *data, size_t len);


void switch_cb(void *arg){
    (void)arg;
    int val = gpio_read(sw_gpio);
    if(val){
        currState=REFERENCE;
        puts("ON");
    }else {
        currState=CLEAN;
        puts("OFF");
    }fflush(stdin);
}

int main(void)
{

    int dist=0;
    int ref=0;
    int perc=0;
    char buffer[15];
    srf04_t dev;
    srf04_params_t my_params;
    my_params.trigger = trigger_gpio;
    my_params.echo = echo_gpio;
    
    if(srf04_init(&dev, &my_params) != SRF04_OK){
       puts("Failed to setup SRF04");
    }
    setup_mqtt();
    pwm_init(PWM_DEV(0),PWM_LEFT,PWM_FREQ,PWM_RES); //USING D3(BUZZ) AND D6(UNUSABLE)
    pwm_set(PWM_DEV(0),1,100);
    pwm_poweroff(PWM_DEV(0));
    gpio_init(led_gpio,GPIO_OUT);
    gpio_init_int(sw_gpio, GPIO_IN, GPIO_BOTH, &switch_cb, NULL);
    while(1){
        switch(currState){
            case REFERENCE:
                pub(MQTT_TOPIC_ACT, "{\"setOn\":\"1\"}", 0);
                ref=srf04_get_distance(&dev);
                puts("IN MAIN STATE REFERENCE");
                if(ref>0)
                    currState=MEASURING;
                break;
            case MEASURING:
                puts("IN MAIN STATE MEASURING");
                dist=srf04_get_distance(&dev);
                if(dist>0){
                    perc=(int)((1.0-((double)dist/(double)ref))*100);
                    sprintf(buffer,"{\"perc\":\"%d\"}",perc);
                    pub(MQTT_TOPIC_MEAS, buffer, 0);
                    puts(buffer);
                    for(int i=0;i<15;i++)
                        buffer[i]=0;
                    if(perc>=70){
                        currState=ALERT;
                    }
                    xtimer_sleep(5);
                }else xtimer_sleep(1);
                break;
            case CLEAN:
                //This case is used to clean stuff
                ref=0;
                perc=0;
                dist=0;
                pub(MQTT_TOPIC_ACT, "{\"setOn\":\"0\"}", 0);
                pub(MQTT_TOPIC_LED, "{\"setOn\":\"0\"}", 0);
                pub(MQTT_TOPIC_BUZZ, "{\"setOn\":\"0\"}", 0);
                currState=STANDBY;
                currAlertS=NOT_ALERTED;
                break;
            case STANDBY:
                puts("IN MAIN STATE STANDBY");
                xtimer_sleep(5);
                break;
            case ALERT:
                puts("IN MAIN STATE ALERT");
                if(currAlertS==NOT_ALERTED){
                    pub(MQTT_TOPIC_LED, "{\"setOn\":\"1\"}", 0);
                    pub(MQTT_TOPIC_BUZZ, "{\"setOn\":\"1\"}", 0);
                    currAlertS=ALERTED;
                }
                currState=MEASURING;
                break;
        }
    }

}

static void *emcute_thread(void *arg)
{
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    /* should never be reached */
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
    
    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, SERVER_ADDR) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }

    if (emcute_con(&gw, true, NULL, NULL, 0, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", SERVER_ADDR, (int)gw.port);
        return 1;
    } else printf("Successfully connected to gateway at [%s]:%i\n", SERVER_ADDR, (int)gw.port);

    // setup subscription to topic
    
    unsigned flags = EMCUTE_QOS_0;
    subscriptions[0].cb = on_pub_led;
    subscriptions[1].cb = on_pub_buzz;

    subscriptions[0].topic.name = MQTT_TOPIC_LED;
    subscriptions[1].topic.name = MQTT_TOPIC_BUZZ;

    if (emcute_sub(&subscriptions[0], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", MQTT_TOPIC_LED);
        return 1;
    }

    if (emcute_sub(&subscriptions[1], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", MQTT_TOPIC_BUZZ);
        return 1;
    }
    printf("Now subscribed to %s\n", MQTT_TOPIC_LED);
    printf("Now subscribed to %s\n", MQTT_TOPIC_BUZZ);
    
    return 0;
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

    t.name = topic;
    if(emcute_reg(&t) != EMCUTE_OK){
        printf("PUB ERROR: Unable to obtain Topic ID, %s",topic);
        return 1;
    }
    if(emcute_pub(&t, data, strlen(data), flags) != EMCUTE_OK){
        printf("PUB ERROR: unable to publish data to topic '%s [%i]'\n", t.name, (int)t.id);
        return 1;
    }

    printf("PUB SUCCESS: Published %s on topic %s\n", data, topic);
    return 0;
}

static void on_pub_buzz(const emcute_topic_t *topic, void *data, size_t len){
    char *in = (char *)data;
    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    if(strcmp(data,"{\"setOn\":\"1\"}")==0){
        pwm_poweron(PWM_DEV(0));
    }
    if(strcmp(data,"{\"setOn\":\"0\"}")==0){
        pwm_poweroff(PWM_DEV(0));
    }

    for (size_t i = 0; i < len; i++) {
        printf("%c", in[i]);
    }
    puts("");
}

static void on_pub_led(const emcute_topic_t *topic, void *data, size_t len){
    char *in = (char *)data;
    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    if(strcmp(data,"{\"setOn\":\"1\"}")==0){
        gpio_set(led_gpio);
    }
    if(strcmp(data,"{\"setOn\":\"0\"}")==0){
        gpio_clear(led_gpio);
    }

    for (size_t i = 0; i < len; i++) {
        printf("%c", in[i]);
    }
    puts("");
}

/********* NETWORK RELATED PART *********/ 
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
/********* NETWORK RELATED PART END *********/ 

