




#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/linkaddr.h"
#include "net/nbr-table.h"
#include <stdio.h>

#include "sys/log.h"
//#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC

/* list of registered sdn nodes */
struct sdn_registered_nodes {
  uint8_t num_nbr;
};
NBR_TABLE(struct sdn_registered_nodes, registered_nodes);
/*---------------------------------------------------------------------------*/
/* add sender of report packets to the registered list */
int
is_registered(struct sdn_packet* p)
{
  linkaddr_t sender_addr;
  linkaddr_copy(&sender_addr, &linkaddr_null);
#if LINKADDR_SIZE == 2
  sender_addr.u8[0] = p->payload[R_SENDER_ADDR_INDEX];
  sender_addr.u8[1] = p->payload[R_SENDER_ADDR_INDEX + 1];
#else
  printf("don't support linkaddr_size");
#endif
    //  printf("this node is registere\n");
    //  LOG_INFO_LLADDR(&sender_addr);
    //  printf("\n");
  struct sdn_registered_nodes *stat = (struct sdn_registered_nodes *)nbr_table_get_from_lladdr(registered_nodes, (linkaddr_t *)&sender_addr);
  /* if the node is not registered acreat and send a config packet, if is registeed, update */
  if(stat == NULL) {
    stat = (struct sdn_registered_nodes *)nbr_table_add_lladdr(registered_nodes, (linkaddr_t *)&sender_addr, NBR_TABLE_REASON_MAC, NULL);
    printf("this node is not in registered list \n");
    return 0;
  } 
  if(stat != NULL && ((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
    printf("this node is registered but diassociated \n");
    return 0;
  }
  if(stat != NULL && ((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) > 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
    printf("this node has to-ctrl not has from-ctrl \n");
    return 2;
  }
  if(stat != NULL && ((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) > 0) {
    printf("this node has from-ctrl not has to-ctrl \n");
    return 3;
  } else {
    if(stat != NULL && (p->payload[R_IS_JOIN_INDEX] & 0x03) == 0){
      printf("this node is registered with no best effort flow\n");
      return 4;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing(void)
{

  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 1;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);
  
  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 2;
  // add the list of nodes in the path to the config packet
  uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x02;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 4;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= (1 -> TX), (2 -> RX), (4-> shared), (8 -> keepalive)
  uint8_t link_option = 1;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing4(void)
{

  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 0;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);
  
  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 2;
  // add the list of nodes in the path to the config packet
  uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x04;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 8;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= (1 -> TX), (2 -> RX), (4-> shared), (8 -> keepalive)
  uint8_t link_option = 1;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing1(void)
{

  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 1;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);
  
  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 2;
  // add the list of nodes in the path to the config packet
  uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x03;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 6;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= (1 -> TX), (2 -> RX), (4-> shared), (8 -> keepalive)
  uint8_t link_option = 2;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing2(void)
{
  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};
  static linkaddr_t addr3 =         {{ 0x03, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 1;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);

  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 3;
  // add the list of nodes in the path to the config packet
   uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr3.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x02;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 3;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= 1 -> TX, 2 -> RX, 4-> shared, 8 -> keepalive
  uint8_t link_option = 1;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing3(void)
{
  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};
  static linkaddr_t addr3 =         {{ 0x03, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 1;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);

  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 3;
  // add the list of nodes in the path to the config packet
   uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr3.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x03;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 7;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= 1 -> TX, 2 -> RX, 4-> shared, 8 -> keepalive
  uint8_t link_option = 2;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* create config packet */
struct sdn_packet * 
create_confing5(void)
{
  int config_size = C_LIST_OF_NODE_IN_PATH_INDEX;
  static linkaddr_t addr1 =         {{ 0x01, 0x00 }};
  static linkaddr_t addr2 =         {{ 0x02, 0x00 }};
  static linkaddr_t addr3 =         {{ 0x03, 0x00 }};

  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 0;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);

  // i need a function to explor the route and the address of the nodes in the path and put it in the packet
  config->payload[C_NUM_NODE_IN_PATH_INDEX] = 3;
  // add the list of nodes in the path to the config packet
   uint8_t j;
  for (j = 0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr1.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr2.u8[j];
    config_size++;
  }
  for (j =0; j< LINKADDR_SIZE; ++j){
    config->payload[config_size] = addr3.u8[j];
    config_size++;
  }
  //set flow ID
  config->payload[config_size] = 0x00;
  config_size++;
  config->payload[config_size] = 0x04;
  config_size++;
  // set SF handle
  /* slotframe must be included SF size and SF handle later we can complete it*/
  config->payload[config_size] = 0;
  config_size++;
  //num cell
  //TODO I must check if the sink node is "is_befor_last_node" in the source routing. if yes, install the schedule
  config->payload[config_size] = 1;
  config_size++;
  //slot offset
  config->payload[config_size] = 1;
  config_size++;
  //channel offset
  config->payload[config_size] = 0;
  config_size++;
  //cell option= 1 -> TX, 2 -> RX, 4-> shared, 8 -> keepalive
  uint8_t link_option = 1;
  //link type: 0 -> normal, 1-> advertising, 2-> advertising-only
  uint8_t link_type = 0;
  config->payload[config_size] = ((link_type & 0x0f)<< 4)+(link_option & 0x0f);
  config_size++;
  return config;
}
/*---------------------------------------------------------------------------*/
/* initialize the registered_nodes table  */
void 
sdn_sink_init(void)
{
  if(nbr_table_is_registered(registered_nodes) == 0) {
    nbr_table_register(registered_nodes, NULL);
  }
}
/*---------------------------------------------------------------------------*/


















