CONTIKI_PROJECT = udp-client udp-client2 udp-client3 udp-server udp-server2
all: $(CONTIKI_PROJECT)

CONTIKI=../..

MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_MAC_DIR)/tsch/sdn

TARGET_LIBFILES = -lm

include $(CONTIKI)/Makefile.include
