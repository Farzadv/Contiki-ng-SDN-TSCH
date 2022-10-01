#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "net/mac/tsch/tsch.h"
#include "net/nbr-table.h"
#include <stdio.h>

#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-inout.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#include "sys/log.h"
#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC



linkaddr_t addr =  {{ 0x01, 0x00 }};

/* flow table */
struct flow_entry {
  uint8_t is_tx_available;
  uint8_t is_rx_available;
  linkaddr_t addr;
};
NBR_TABLE(struct flow_entry, flow_table);
/*---------------------------------------------------------------------------*/
struct route_entry {
  struct route_entry *next;
  linkaddr_t rx_flow_id;
  linkaddr_t tx_flow_id;
  uint16_t src_port;
  uint16_t dst_port;
  linkaddr_t dst_addr;
};
#define MAX_ROUTE_ENTRY 20
LIST(routing_table);
MEMB(routing_mem, struct route_entry, MAX_ROUTE_ENTRY);
/*---------------------------------------------------------------------------*/
struct flow_table_entry {
  struct flow_table_entry *next;
  uint8_t sf_handle;
  uint8_t slot_num;
  uint8_t ch_off;
  uint8_t priority;
  linkaddr_t flow_id;
};
#define MAX_FLOW_ENTRY 30
LIST(sdn_flow_table);
MEMB(flow_table_mem, struct flow_table_entry, MAX_ROUTE_ENTRY);
/*---------------------------------------------------------------------------*/
/* this function add a flow to the flow table */
int 
sdn_add_flow_enrty(const linkaddr_t *flow_id, int sf_handle, int slot_num, int ch_off, int priority)
{
  struct flow_table_entry *e;
  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){
    if (sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && linkaddr_cmp(&e->flow_id, flow_id) && priority == e->priority){
      printf("SDN FLOW: flow exists in table \n");
      return 0;
    }
    if (sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && linkaddr_cmp(&e->flow_id, flow_id) && priority != e->priority){
      // update entry priority 
      e->priority = priority;
      // NB: we do not sort the entries by priority number here: just in lookup
      // function we care about it and select the entry with highest priority 
      printf("SDN FLOW: flow exists but update priority \n");
      return 1;
    }
  }

  if (e == NULL){
    e = memb_alloc(&flow_table_mem);
    if (e != NULL){
      linkaddr_copy(&e->flow_id, flow_id);
      e->sf_handle = sf_handle;
      e->slot_num = slot_num;
      e->ch_off = ch_off;
      e->priority = priority;
      list_add(sdn_flow_table, e);
      printf("SDN FLOW: add a new flow, sfid %d, slot num %d, ch-off %d, priority %d, flowid \n",
             e->sf_handle, e->slot_num, e->ch_off, e->priority);
      LOG_INFO_LLADDR(&e->flow_id);
      LOG_INFO_("\n");
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* this function find the flow id coressponds to the slotframe handle and timeslot */
/* it findes the flow with highest priority */
linkaddr_t *
sdn_find_flow_id(const int sf_handle, const int timeslot)
{
  struct flow_table_entry *e;
  linkaddr_t *best_flow_id = NULL;
  int priority = 1000;

  if(!tsch_is_locked()){

    for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
      if (sf_handle == e->sf_handle && timeslot == e->slot_num){
        if (e->priority < priority){
          priority = e->priority;
          best_flow_id = &e->flow_id;
        }
      } 
    }

  }
 return best_flow_id;
}
/*---------------------------------------------------------------------------*/
linkaddr_t *
sdn_get_addr_for_flow_id(const linkaddr_t *flow_id)
{
  struct flow_table_entry *e;

  struct tsch_slotframe *slotframe;
  struct tsch_link *link;

    for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
      if (linkaddr_cmp(&e->flow_id, flow_id)){
        slotframe = tsch_schedule_get_slotframe_by_handle(e->sf_handle);
        link = tsch_schedule_get_link_by_timeslot(slotframe, e->slot_num, e->ch_off);
        if(link != NULL){
          //printf("find addr for flow \n");
          return &link->addr;
        }
      }
    } 
 return NULL;
}
/*---------------------------------------------------------------------------*/
int
sdn_is_flowid_exist_in_table(const linkaddr_t *flow_id)
{
  struct flow_table_entry *e;
  int num_entry = 0;

  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
      if(linkaddr_cmp(&e->flow_id, flow_id)){
        num_entry++;
      }
  }
 return num_entry;
}








/*---------------------------------------------------------------------------*/
linkaddr_t *
sdn_find_flow_rule(const linkaddr_t *addr)
{
  struct route_entry *e;
  for(e = list_head(routing_table); e != NULL; e = e->next){
    if (linkaddr_cmp(&e->dst_addr, addr)){
      printf("there is a match rule \n");
      return &e->tx_flow_id;
    }
  } 
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* add a route entry */
int 
sdn_add_rule(const linkaddr_t *dst_addr, const linkaddr_t *flow_id)
{
  struct route_entry *n;
  n = memb_alloc(&routing_mem);
  if (n != NULL){
    linkaddr_copy(&n->dst_addr, dst_addr);
    linkaddr_copy(&n->tx_flow_id, flow_id);
    list_add(routing_table, n);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
sdn_find_tx_flow_id(const linkaddr_t *flow_id)
{
  struct flow_entry *stat = (struct flow_entry *)nbr_table_get_from_lladdr(flow_table, (linkaddr_t *)flow_id);
  if(stat != NULL) {
    if (stat->is_tx_available > 0){
      return stat->is_tx_available;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int 
sdn_add_flow(const linkaddr_t *flow_id, const linkaddr_t *addr, int is_tx, int is_rx)
{
  struct flow_entry *stat = (struct flow_entry *)nbr_table_get_from_lladdr(flow_table, (linkaddr_t *)flow_id);
  if(stat == NULL) {
    stat = (struct flow_entry *)nbr_table_add_lladdr(flow_table, (linkaddr_t *)flow_id, NBR_TABLE_REASON_MAC, NULL);
    if (stat != NULL){
      linkaddr_copy(&stat->addr, addr);
      stat->is_tx_available = is_tx;
      stat->is_rx_available = is_rx;
              
       LOG_INFO("flow ID ");
       LOG_INFO_LLADDR(flow_id);
       LOG_INFO_("\n");
       printf(" flow ID is tx %d \n",stat->is_tx_available);
       printf(" flow ID is rx %d \n",stat->is_rx_available);
       
    }
    printf(" flow ID is added to flow table \n");
    return 0;
  } else {
    /* update flow table */ 
    linkaddr_copy(&stat->addr, addr);
    if (is_tx > 0){
      stat->is_tx_available = is_tx;
    }
    if (is_rx > 0){
      stat->is_rx_available = is_rx;
    }
    printf(" flow ID exists in flow table \n");
    printf(" flow ID is tx %d \n",stat->is_tx_available);
    printf(" flow ID is rx %d \n",stat->is_rx_available);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
void 
sdn_flow_init(void)
{ 
  /* initialize flow table */
  if(nbr_table_is_registered(flow_table) == 0) {
    nbr_table_register(flow_table, NULL);
  }
  /* initialize routing table */ 
  memb_init(&routing_mem);
  list_init(routing_table);
  /* initialize sdn_flow_table table */ 
  memb_init(&flow_table_mem);
  list_init(sdn_flow_table);
}
/*---------------------------------------------------------------------------*/













