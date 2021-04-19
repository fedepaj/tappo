# Name of your application
APPLICATION = tappo

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f401re

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

UPLINK ?= ethos

# Ultrasonic range finder module
USEMODULE += srf04
USEMODULE += xtimer
USEMODULE += fmt
USEMODULE += periph_gpio
USEMODULE += periph_pwm
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6
USEMODULE += gnrc_ipv6_default
# Include MQTT-SN
USEMODULE += emcute
# Optimize network stack to for use with a single network interface
USEMODULE += gnrc_netif_single
USEMODULE += stdio_ethos gnrc_uhcpc

EMCUTE_ID ?= tank_1
ETHOS_BAUDRATE ?= 115200
TAP ?= tap0
USE_DHCPV6 ?= 0
IPV6_PREFIX ?= fe80:2::/64 #?
SERVER_ADDR = fec0:affe::1
SERVER_PORT = 1885
MQTT_TOPIC_MEAS = $(EMCUTE_ID)/measures
MQTT_TOPIC_ACT = $(EMCUTE_ID)/active
MQTT_TOPIC_LED = $(EMCUTE_ID)/led
MQTT_TOPIC_BUZZ = $(EMCUTE_ID)/buzz

CFLAGS += -DSERVER_ADDR='"$(SERVER_ADDR)"'
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
CFLAGS += -DMQTT_TOPIC_ACT='"$(MQTT_TOPIC_ACT)"'
CFLAGS += -DMQTT_TOPIC_MEAS='"$(MQTT_TOPIC_MEAS)"'
CFLAGS += -DMQTT_TOPIC_LED='"$(MQTT_TOPIC_LED)"'
CFLAGS += -DMQTT_TOPIC_BUZZ='"$(MQTT_TOPIC_BUZZ)"'

# Allow for env-var-based override of the nodes name (EMCUTE_ID)
ifneq (,$(EMCUTE_ID))
  CFLAGS += -DEMCUTE_ID=\"$(EMCUTE_ID)\"
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Comment this out to join RPL DODAGs even if DIOs do not contain
# DODAG Configuration Options (see the doc for more info)
# CFLAGS += -DCONFIG_GNRC_RPL_DODAG_CONF_OPTIONAL_ON_JOIN

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(CURDIR)/Makefile.ethos.conf
include $(RIOTBASE)/Makefile.include

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)