

#ifndef _SDN_H_
#define _SDN_H_


/********** Includes **********/
#include "contiki.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/linkaddr.h"



/********** Data types **********/

/* Are we joined to a SDN network? */
extern int sdn_is_joined;
extern int num_from_controller_rx_slots;


#define SDN_MAX_PAYLOAD     100
#define FLOW_ID_SIZE        2

/* report packet index */
#define R_SENDER_ADDR_INDEX     0
#define R_IS_JOIN_INDEX         LINKADDR_SIZE
#define R_BATT_INDEX            (R_IS_JOIN_INDEX + 1)
#define R_REPORT_SEQ_NUM_INDEX  (R_BATT_INDEX + 1)
#define R_METRIC_TYP_INDEX      (R_REPORT_SEQ_NUM_INDEX + 1)
#define R_NBR_NUB_INDEX         (R_METRIC_TYP_INDEX + 1)


/* config packet index */
#define C_NUM_NODE_IN_PATH_INDEX      0
#define C_LIST_OF_NODE_IN_PATH_INDEX  (C_NUM_NODE_IN_PATH_INDEX + 1)



typedef enum { 
    RSSI=1, 
    EB_NUM=2, 
    RSSI_EB=3 
} metric;

typedef enum { 
    REPORT = 1, 
    CONFIG = 2
} sdn_typ;
/* SDN packet format */
struct sdn_packet {
  uint8_t typ;
  uint8_t payload[SDN_MAX_PAYLOAD];
  struct sdn_packet *next;
};


/* SDN link */
struct sdn_link {
  /* slotframe must be included SF size and SF handle later we can complete it*/
  uint16_t  sf; // we suppose here sf is just slotframe handle
  uint16_t slot;
  uint16_t channel_offset;
  linkaddr_t addr;
  linkaddr_t flow_id;
  uint8_t  link_option;
  uint8_t link_type;
};

#endif












