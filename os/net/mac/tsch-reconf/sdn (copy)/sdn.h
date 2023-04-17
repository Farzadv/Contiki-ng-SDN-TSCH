

#ifndef _SDN_H_
#define _SDN_H_


/********** Includes **********/
#include "contiki.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/linkaddr.h"



/********** Data types **********/

/* Are we joined to a SDN network? */
extern int sdn_is_joined; // we do not allow a node to send EB befor joining to th SDN network. else maybe a novel node sends its report packet to 
                          // node that it is also joined to TSCH netwok and it is steel novel node itself
extern int num_from_controller_rx_slots;
extern int sdn__sf_offset_to_send_eb;
extern int sdn__ts_offset_to_send_eb;
extern uint16_t sdn_max_used_sf_offs;
#if SINK
extern int sdn_num_shared_cell;
extern int sdn_num_shared_cell_in_rep;
#endif


#define DIST_UNIFORM_LEN      1000
struct list_dist_uniform{
  int list[DIST_UNIFORM_LEN];
  uint16_t len;
};

#define SDN_MAX_PAYLOAD     100
#define FLOW_ID_SIZE        2
#define CONFIG_CELL_SIZE    3   // 2 byte: timeslot, 1 byte: channel offset

/* max sdn neighbor can be placed in the report packet: 
   there is filter that takes the best NBRs to report.
   this value is given to packet size in TSCH */
#define SDN_MAX_NUM_REPORT_NBR   30


/******************** REPORT packet index *********************/
#define R_SENDER_ADDR_INDEX     0
#define R_IS_JOIN_INDEX         LINKADDR_SIZE
#define R_BATT_INDEX            (R_IS_JOIN_INDEX + 1)
#define R_REPORT_SEQ_NUM_INDEX  (R_BATT_INDEX + 1)
#define R_METRIC_TYP_INDEX      (R_REPORT_SEQ_NUM_INDEX + 1)
#define R_NBR_NUB_INDEX         (R_METRIC_TYP_INDEX + 1)

/*
     +----------+-----------+------------+------------+--------+------------+-----------+-------------+--------------------------------------------+
     | SDN pkt  |  sender   |            |            |  Rep   |  metric    |  NUM      |             |                                            |
     |  type:   |   addr    | is joined? | BATT level |  seq   |  type      |  of       |             |  list of NBRs + their metrics              |
     |  REPORT  |           |            |            |  NUM   |            |  NBRs     |             |                                            |
     +----------+-----------+------------+------------+--------+------------+-----------+-------------+--------------------------------------------+
*/

/********************** CONFIG packet index ********************/
#define CONF_EB_SF_OFFS_INDEX                   0
#define CONF_EB_TS_OFFS_INDEX                   (CONF_EB_SF_OFFS_INDEX + 2)
#define CONF_SEQ_NUM_INDEX                      (CONF_EB_TS_OFFS_INDEX + 2)
#define CONF_REQUEST_NUM_INDEX                  (CONF_SEQ_NUM_INDEX + 1)
#define CONF_REPETION_PERIOD                    (CONF_REQUEST_NUM_INDEX + 1)
#define CONF_NUM_SOURCE_ROUTING_NODE_INDEX      (CONF_REPETION_PERIOD + 2)
#define CONF_LIST_OF_NODE_IN_PATH_INDEX         (CONF_NUM_SOURCE_ROUTING_NODE_INDEX + 1)
/*
     +----------+---------+----------+--------+---------+------+---------- +-------------------------------------------------+
     | SDN pkt  |         |    NUM   | source | num     |flow  | slotframe |                                                 |
     |  type:   | Req-num | of nodes | routing| cell/hop|ID    |   ID      |  list of cells (by order of hops)               |
     |  CONFIG  |         |  in path | addr   |         |      |           |                                                 |
     +----------+---------+----------+--------+---------+------+-----------+-------------------------------------------------+
*/

/* CONFIG1 packet index */
#define C_REQUEST_NUM      0
#define C_NUM_NODE_IN_PATH_INDEX      (C_REQUEST_NUM + 1)
#define C_LIST_OF_NODE_IN_PATH_INDEX  (C_NUM_NODE_IN_PATH_INDEX + 1)
/********************** REQUEST packet index *******************/
#define REQ_SENDER_ADDR_INDEX           0
#define REQ_TYP_INDEX                   LINKADDR_SIZE
#define REQ_NUM_OF_REQ                  (REQ_TYP_INDEX + 1)
#define REQ_REMOTE_IPADDR_INDEX         (REQ_NUM_OF_REQ + 1)
#define REQ_REMOTE_PORT_INDEX           (REQ_REMOTE_IPADDR_INDEX + 16)
#define REQ_LOCAL_IPADDR_INDEX          (REQ_REMOTE_PORT_INDEX + 2)
#define REQ_LOCAL_PORT_INDEX            (REQ_LOCAL_IPADDR_INDEX + 16)
#define REQ_QOS_TRAFFIC_PERIOD_INDEX    (REQ_LOCAL_PORT_INDEX + 2)
#define REQ_QOS_RELIABILITY_INDEX       (REQ_QOS_TRAFFIC_PERIOD_INDEX + 2)
#define REQ_QOS_MAX_DELAY_INDEX         (REQ_QOS_RELIABILITY_INDEX + 2)
#define REQ_QOS_PACKET_SIZE_INDEX       (REQ_QOS_MAX_DELAY_INDEX + 2)
#define REQ_QOS_BURST_SIZE_INDEX        (REQ_QOS_PACKET_SIZE_INDEX + 2)


typedef enum { 
    FLOW_ID_REQ=1, 
    MISS_TABLE_REQ=2
} request_type;

/*
     +----------+-----------+------------+--------+------------+-----------+---------+------------------------------------------------------------+
     | SDN pkt  |   addr    | type       |socket  |socket      |socket     |socket   |                                                            |
     |   type   | of sender | of         |ripaddr |rport       |lipaddr    |lport    | QoS(traffic_period, reliability, max_delay, packet_size)   |
     | REQUEST  |           | request    |        |            |           |         |                                                            |
     +----------+-----------+------------+--------+------------+-----------+---------------------------------------------------------------------+
*/

/******************* ACK-CONFIG index ************************/
#define ACK_SENDER_ADDR_INDEX     0
#define ACK_SEQ_NUM_INDEX         LINKADDR_SIZE

/*
     +----------+-----------+----------+
     | SDN pkt  |   addr    |          |
     |  type:   | of sender | seq_num  |
     |CONFIG-ACK|           |          |
     +----------+-----------+----------+
*/

/*************************************************************/


typedef enum { 
    RSSI=1, 
    EB_NUM=2, 
    RSSI_EB=3 
} metric;

typedef enum { 
    REPORT = 1, 
    CONFIG = 2,
    REQUEST= 3,
    CONFIG_ACK=4
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












