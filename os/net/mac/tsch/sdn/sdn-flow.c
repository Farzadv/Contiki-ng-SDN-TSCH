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
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_FLOW_LOG_LEVEL


int remove_flow_entry(struct flow_table_entry *e);


linkaddr_t addr =  {{ 0x01, 0x00 }};

/*---------------------------------------------------------------------------*/
struct from_ctrl_ch_off {
  struct from_ctrl_ch_off *next;
  linkaddr_t nbr_addr;
  uint8_t ch_off;
};
#define FROM_CTRL_CHOFF_LEN 1500
LIST(from_ctrl_nbr_table);
MEMB(from_ctrl_nbr_table_mem, struct from_ctrl_ch_off, FROM_CTRL_CHOFF_LEN);
/*---------------------------------------------------------------------------*/
#define MAX_FLOW_ENTRY 1500
LIST(sdn_flow_table);
MEMB(flow_table_mem, struct flow_table_entry, MAX_FLOW_ENTRY);
/*---------------------------------------------------------------------------*/
/* this function add a flow to the flow table */
int 
sdn_add_flow_enrty(const linkaddr_t *flow_id, int sf_handle, int slot_num, int ch_off, int priority)
{
  struct flow_table_entry *e;
  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){

/*  
#if SDN_RECONF_MODE
    if(linkaddr_cmp(&e->flow_id, flow_id) && linkaddr_cmp(&e->flow_id, &flow_id_to_controller)) {
      e->sf_handle = sf_handle;
      e->slot_num = slot_num;
      e->ch_off = ch_off;
      e->priority = priority;
      
      //flush rest of data flows and prepare it to reconfig //TODO
      struct flow_table_entry *ee;
      for(ee = list_head(sdn_flow_table); ee != NULL; ee = ee->next) {
        if(ee->flow_id.u8[0] > 8) {
          printf("remove data flow-conf %d \n", ee->flow_id.u8[0]);
          list_remove(sdn_flow_table, ee);
          memb_free(&flow_table_mem, ee);
        }      
      }
      return 1;
    }
#endif
*/
 
    if (sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && linkaddr_cmp(&e->flow_id, flow_id) && priority == e->priority){
      LOG_INFO("SDN FLOW: flow exists in table \n");
      return 0;
    }
    if (sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && linkaddr_cmp(&e->flow_id, flow_id) && priority != e->priority){
      // update entry priority 
      e->priority = priority;
      // NB: we do not sort the entries by priority number here: just in lookup
      // function we care about it and select the entry with highest priority 
      LOG_INFO("SDN FLOW: flow exists but update priority \n");
      return 1;
    }
    /*
    if(sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && priority == e->priority){
      LOG_INFO("SDN FLOW: schedule exist for another fid -> flash the schedule \n");
      remove_flow_entry(e);
      e = NULL;
      break;
    }
    */
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
      LOG_INFO("SDN FLOW: add a new flow, sfid %u, slot num %u, ch-off %u, priority %u, flowid ",
             e->sf_handle, e->slot_num, e->ch_off, e->priority);
      LOG_INFO_LLADDR(&e->flow_id);
      LOG_INFO_("\n");
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int 
remove_flow_entry(struct flow_table_entry *e)
{
  if(e != NULL){
    list_remove(sdn_flow_table, e);
    memb_free(&flow_table_mem, e);
    return 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/* this function removes a flow from the flow table */
int 
sdn_remove_flow_entry(const linkaddr_t *flow_id, int sf_handle, int slot_num, int ch_off, int priority)
{
  struct flow_table_entry *e;
  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){
    if(sf_handle == e->sf_handle && slot_num == e->slot_num && ch_off == e->ch_off && linkaddr_cmp(&e->flow_id, flow_id) && priority == e->priority){
      remove_flow_entry(e);
      LOG_INFO("SDN FLOW: remove flow, sfid %u, slot num %u, ch-off %u, priority %u, flowid ",
             e->sf_handle, e->slot_num, e->ch_off, e->priority);
      LOG_INFO_LLADDR(&e->flow_id);
      LOG_INFO_("\n");
      return 0;
    }
  }
  LOG_INFO("SDN FLOW: fail remove flow, sfid %u, slot num %u, ch-off %u, priority %u, flowid ",
             sf_handle, slot_num, ch_off, priority);
  LOG_INFO_LLADDR(flow_id);
  LOG_INFO_("\n");
  return -1;
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
/* are there schedule for this flow-id in the flow table? */
int
sdn_do_cells_exist_in_flowtable_for_flow_id(const linkaddr_t *flow_id)
{
  struct flow_table_entry *e;

  if(!tsch_is_locked()){
    for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
      if (linkaddr_cmp(&e->flow_id, flow_id) && (e->slot_num > -1)){
        return 1;
      } 
    }
  }
  return 0;
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
          //LOG_INFO("find addr for flow \n");
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
int
sdn_associate_choff_to_nbr(const linkaddr_t *nbr_addr, int ch_off)
{
  struct from_ctrl_ch_off *e;
  for(e = list_head(from_ctrl_nbr_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->nbr_addr, nbr_addr)){
      return 0;
    }
  }
  if(e == NULL){
    e = memb_alloc(&from_ctrl_nbr_table_mem);
    if(e != NULL){
      linkaddr_copy(&e->nbr_addr, nbr_addr);
      e->ch_off = ch_off;
      list_add(from_ctrl_nbr_table, e);
      //LOG_INFO("SDN_FLOW: assoc choff to nbr addr addr: %d%d , choff %d \n", e->nbr_addr.u8[0], e->nbr_addr.u8[1], ch_off);
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
sdn_get_fromctrl_choff_from_nbr_addr(const linkaddr_t *nbr_addr)
{
  struct from_ctrl_ch_off *e = NULL;
  
  if(nbr_addr != NULL){
    if(!tsch_is_locked()){ 
      for(e = list_head(from_ctrl_nbr_table); e != NULL; e = e->next){ 
        if(linkaddr_cmp(&e->nbr_addr, nbr_addr)){
          return e->ch_off;
        }
      }
      if(e == NULL){
        return -1;
      }
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
void 
sdn_print_flow_table(void)
{
  struct flow_table_entry *e;

  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
    if(e != NULL) {
      LOG_INFO("SDN FLOW: * sf: %d, slot: %d, ch: %d, pr: %d, fid: %d%d *\n",
             e->sf_handle, e->slot_num, e->ch_off, e->priority, e->flow_id.u8[0], e->flow_id.u8[1]);
    } 
  }
}
/*---------------------------------------------------------------------------*/
/* find the first timeslot correspond to the flowid this func is used in
   app layer to generate packets right befor the first timeslot */
int 
sdn_find_first_timeslot_in_slotframe(const linkaddr_t *flow_id)
{
  struct flow_table_entry *e;
  uint16_t min_ts = SDN_DATA_SLOTFRAME_SIZE;

  for(e = list_head(sdn_flow_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->flow_id, flow_id)){
      if(e->slot_num < min_ts) {
        min_ts = e->slot_num;
      }
    }
  }
  
  if(min_ts == SDN_DATA_SLOTFRAME_SIZE) {
    return 0;
  }
  return min_ts;
}
/*---------------------------------------------------------------------------*/
void 
sdn_flow_init(void)
{ 
  /* initialize sdn_flow_table table */ 
  memb_init(&flow_table_mem);
  list_init(sdn_flow_table);
  
  /* initialize from_ctrl_nbr_table */ 
  memb_init(&from_ctrl_nbr_table_mem);
  list_init(from_ctrl_nbr_table);
  
}
/*---------------------------------------------------------------------------*/













