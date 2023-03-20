#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/linkaddr.h"
#include "net/nbr-table.h"
#include <stdio.h>
#include "random.h"
#include "lib/memb.h"
#include <malloc.h>
#include "lib/list.h"

#include "sys/log.h"
#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC


static linkaddr_t last_flow_id = {{ 0x08, 0x00 }};
/*---------------------------------------------------------------------------*/
/* Declaration */
void print_global_table(void);

struct sent_config_info * alloc_mem_to_sent_config(void);

void dealloc_mem_sent_config(struct sent_config_info* p);

struct sent_config_info * add_source_routing_info_to_config(struct sdn_packet* config, const linkaddr_t *sender_addr, const linkaddr_t *parent_addr, const linkaddr_t *flow_id);

void set_timer_to_config_packet(struct sent_config_info* sent_config);

/*---------------------------------------------------------------------------*/
/* list of shared cells */
struct list_of_shared_cell shared_cell;

/*---------------------------------------------------------------------------*/
PROCESS(conf_handle_process, "conf handle process");
//AUTOSTART_PROCESSES(&conf_handle_process);
/*---------------------------------------------------------------------------*/
#define MAX_CONFIG_TRANSMISSION  2
struct sent_config_info{
  struct sent_config_info *next;
  struct request_id req_id;
  struct sdn_packet_info packet_info;
  linkaddr_t ack_sender;
  int seq_num;
  int transmission;
  int is_acked;
};
#define SENT_CONFIG_LIST_SIZE 80
LIST(sent_config_list);
MEMB(sent_config_list_mem, struct sent_config_info, SENT_CONFIG_LIST_SIZE);
/*---------------------------------------------------------------------------*/
#define PERIOD_RESEND_CONFIG    (2 * CLOCK_SECOND)
#define NUM_TIMER 60
LIST(timer_list);
MEMB(timer_list_mem, struct config_timer, NUM_TIMER);
/*---------------------------------------------------------------------------*/
#define NODE_SLOT_LIST_SIZE 40
struct sdn_global_table_entry {
  struct sdn_global_table_entry *next;
  linkaddr_t node_addr;
  linkaddr_t parent_addr;
  int taken_slot[NODE_SLOT_LIST_SIZE];
  int to_ctrl_slot;
  int from_ctrl_slot;
  int best_effort_slot;
  int seq_num;
};
#define NUM_GLOBAL_TABLE_ENTRY 30
LIST(sdn_global_table);
MEMB(sdn_global_table_mem, struct sdn_global_table_entry, NUM_GLOBAL_TABLE_ENTRY);
/*---------------------------------------------------------------------------*/
int
specify_parent_from_report(struct sdn_packet* p)
{
  if(p->payload[R_METRIC_TYP_INDEX] == EB_NUM && p->payload[R_NBR_NUB_INDEX] > 0){
  
    int i;
    int j;
    int counter = R_NBR_NUB_INDEX + 1;
    int nbr_eb_number = 0;
    linkaddr_t addr;
    linkaddr_t parent_addr;
    linkaddr_t sender_addr;
    
    for(j = 0; j< LINKADDR_SIZE; ++j){
      sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
    }
  
    printf("CONTROLLER: specify parent 1 \n");
    for(i=0; i<p->payload[R_NBR_NUB_INDEX]; i++){
      for(j = 0; j< LINKADDR_SIZE; ++j){
        addr.u8[j] = p->payload[counter];
        counter++;
      }
      if(p->payload[counter] > nbr_eb_number){
        nbr_eb_number = p->payload[counter];
        linkaddr_copy(&parent_addr, &addr);
        printf("CONTROLLER: specify parent 1 \n");
      }
      counter++;
    }
    LOG_INFO("CONTROLLER: specify parent addr ");
    LOG_INFO_LLADDR(&parent_addr);
    LOG_INFO_("\n");
    
    if((i = sdn_add_node_to_global_table(&sender_addr, &parent_addr)) == 1){
      print_global_table(); //must be removed
      return 1;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/*int 
flash_previous_config(const linkaddr_t *node_addr, , const linkaddr_t *flow_id)
{
  struct sdn_global_table_entry *e;
  struct sdn_global_table_entry *e_child = NULL;
  struct sdn_global_table_entry *e_parent = NULL;
  linkaddr_t parent_addr;
  int i;
  int slot_to_remove = -1;
  int need_flash = 0;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      e_child = e;
      rintf("CONTROLLER: node needs to be flash \n");
      if(&e->parent_addr != NULL){
        linkaddr_copy(&parent_addr, &e->parent_addr);
        need_flash = 1;
      } else{
        printf("CONTROLLER: parent is NULL to flash \n");
      }
    }
  }
  if(need_flash){
    for(e = list_head(sdn_global_table); e != NULL; e = e->next){
      if(linkaddr_cmp(&e->node_addr, &parent_addr)){
        e_parent = e;
      }
    }
  }
  
  if(linkaddr_cmp(flow_id, &flow_id_to_controller) && e_child != NULL){
    slot_to_remove = e_child->to_ctrl_slot;
    list_remove(sdn_global_table, e_child);
    memb_free(&sdn_global_table_mem, e_child);
    printf("CONTROLLER: remove to_ctrl node entry from table \n");
    if(need_flash){
      for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
        if(slot_to_remove > -1 && e_parent->taken_slot[i] == slot_to_remove){
          e_parent->taken_slot[i] = -1;
          printf("CONTROLLER: parent -> remove to_ctrl slot \n");
          break;
        } 
      }
    } 
  }
  if(linkaddr_cmp(flow_id, &flow_id_from_controller) && e_child != NULL){
    if(e_parent != NULL){
      slot_to_remove = e_parent->from_ctrl_slot;
    }
    for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
      if(slot_to_remove > -1 && e_child->taken_slot[i] == slot_to_remove){
        e_child->taken_slot[i] = -1;
        printf("CONTROLLER: child -> remove from_ctrl slot \n");
        break;
      } 
    }  
  }
  if(linkaddr_cmp(flow_id, &flow_id_best_effort) && e_child != NULL){
    slot_to_remove = e_child->best_effort_slot;
    if(slot_to_remove > -1){
      e_child->best_effort_slot = -1;
      for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
        if(e_child->taken_slot[i] == slot_to_remove){
          e_child->taken_slot[i] = -1;
          printf("CONTROLLER: remove to_ctrl slot \n");
          break;
        } 
        if(e_parent->taken_slot[i] == slot_to_remove){
          e_parent->taken_slot[i] = -1;
          printf("CONTROLLER: remove to_ctrl slot \n");
          break;
        }
      }
    }
  }
}*/
/*---------------------------------------------------------------------------*/
linkaddr_t *
get_parent_from_node_addr(const linkaddr_t *node_addr)
{
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      return &e->parent_addr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int 
add_slot_to_taken_cell_list(const linkaddr_t *node_addr, int slot_offset, int ch_offset)
{ 
  int i;
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
        if(e->taken_slot[i] == -1){
          e->taken_slot[i] = slot_offset;
          printf("CONTROLLER: add slot %d to node \n", e->taken_slot[i]);
          LOG_INFO_LLADDR(&e->node_addr);
          printf("\n");
          return 1;
        } 
        if(e->taken_slot[i] != -1 && i == NODE_SLOT_LIST_SIZE-1){
          printf("CONTROLLER: NODE_SLOT_LIST_SIZE exceeds, no free space to allocate \n");
        }
      }
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int 
specify_to_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset)
{
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      e->to_ctrl_slot = slot_offset;
      return 1;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int 
specify_from_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset)
{
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      e->from_ctrl_slot = slot_offset;
      return 1;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int 
specify_best_effort_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset)
{
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      e->best_effort_slot = slot_offset;
      return 1;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
void 
print_global_table(void)
{
  int i;
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){ 
    LOG_INFO("GLOBAL TABLE: node addr:");
    LOG_INFO_LLADDR(&e->node_addr);
    LOG_INFO_("\n");
    LOG_INFO(", parent addr:");
    LOG_INFO_LLADDR(&e->parent_addr);
    LOG_INFO_("\n");
    printf(" taken slots: ");
    for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
      if(e->taken_slot[i] != -1){
        printf("%d ", e->taken_slot[i]);
      } 
    }
    printf(",to_ctrl_slot: %d ", e->to_ctrl_slot);
    printf(",from_ctrl_slot: %d ", e->from_ctrl_slot);
    printf(",best_ctrl_slot: %d \n", e->best_effort_slot);
  }
}
/*---------------------------------------------------------------------------*/
void 
print_sent_config_list(void)
{
  struct sent_config_info *e;
      
  for(e = list_head(sent_config_list); e != NULL; e = e->next){ 
    printf("List of sent config\n");
    LOG_INFO("ack sender: ");
    LOG_INFO_LLADDR(&e->ack_sender);
    printf(" ,seq num %d ", e->seq_num);
    printf(",is acked %d ", e->is_acked);
    printf(",transmission %d ", e->transmission);
    printf(",request id %d.%d \n", e->req_id.is_ctrl, e->req_id.req_num);
  }
}
/*---------------------------------------------------------------------------*/
int 
sdn_add_node_to_global_table(const linkaddr_t *node_addr, const linkaddr_t *parent_addr)
{
  struct list_of_shared_cell *shared_cell;
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->node_addr, node_addr)){
      return -1;
    }
  }
  if(e == NULL){
    e = memb_alloc(&sdn_global_table_mem);
    if(e != NULL){
      linkaddr_copy(&e->node_addr, node_addr);
      linkaddr_copy(&e->parent_addr, parent_addr);
      int i;
      //initialize parameters
      for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
        e->taken_slot[i] = -1;
      }
      e->to_ctrl_slot = -1;
      e->from_ctrl_slot = -1;
      e->best_effort_slot = -1;
      e->seq_num = -1;
      
      list_add(sdn_global_table, e);
      
      //add shared cells to the list of cells 
      if((shared_cell = get_shared_cell_list()) != NULL){
        for(i=0; i<SHARED_CELL_LIST_LEN; i++){
          if(shared_cell->cell_list[i] > -1){
            printf("CONTROLLER: inside the func\n"); // must be removed
            add_slot_to_taken_cell_list(node_addr, shared_cell->cell_list[i], 0); // set ch_off of shared cell to 0
          }
        }
      }       
    }
    return 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
struct sent_config_info * 
add_source_routing_info_to_config(struct sdn_packet* config, const linkaddr_t *sender_addr,
                                  const linkaddr_t *parent_addr, const linkaddr_t *flow_id)
{
  struct sent_config_info *my_config = alloc_mem_to_sent_config();
  
  /**************/
  /*define an array for (source route)SR*/
  int i;
  int len_of_sr_list = 20;
  struct src_route_addr{
    linkaddr_t sr_addr;
  };
  
  struct src_route_addr src_route_list[len_of_sr_list];
  for(i=0; i<len_of_sr_list; i++){
    linkaddr_copy(&src_route_list[i].sr_addr, &linkaddr_null);
  }
  /*************/
  
  linkaddr_t temp_par_addr;
  linkaddr_t last_hop_addr;
  uint8_t j;
  uint8_t num_src_routing_node;
  uint8_t curr_list_index = len_of_sr_list -1;
  int counter = CONF_LIST_OF_NODE_IN_PATH_INDEX;
  struct cell_info *free_cell = NULL;
  struct sdn_global_table_entry *e;
  
  linkaddr_copy(&temp_par_addr, sender_addr);
  linkaddr_copy(&src_route_list[curr_list_index].sr_addr, sender_addr);
  
  // extract SR from table
  while(!linkaddr_cmp(&sink_address, &temp_par_addr)){
    for(e = list_head(sdn_global_table); e != NULL; e = e->next){ 
      if(linkaddr_cmp(&e->node_addr, &temp_par_addr)){
        linkaddr_copy(&temp_par_addr, &e->parent_addr);
        curr_list_index--;
        linkaddr_copy(&src_route_list[curr_list_index].sr_addr, &temp_par_addr);
        break;
      }
    }
  }
  
  num_src_routing_node = len_of_sr_list - curr_list_index;
  
  //insert SR in config packet
  for(i=0; i<num_src_routing_node; i++){
    for(j = 0; j< LINKADDR_SIZE; ++j){
      config->payload[counter] = src_route_list[curr_list_index + i].sr_addr.u8[j];
      counter++;
    }
    if(!linkaddr_cmp(flow_id, &flow_id_from_controller) && (i == num_src_routing_node-1)){
      linkaddr_copy(&last_hop_addr, &src_route_list[curr_list_index + i].sr_addr);
    }
  }
  
  
  // add the last hops to SR in from_ctrl flow_id
  if(linkaddr_cmp(flow_id, &flow_id_from_controller)){
    for(j = 0; j< LINKADDR_SIZE; ++j){
      config->payload[counter] = parent_addr->u8[j];
      counter++;
    }
    num_src_routing_node = num_src_routing_node + 1;
    linkaddr_copy(&last_hop_addr, parent_addr);
  }
  
  config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] = num_src_routing_node;
  
  //add num of cells per hop
  for(i=1; i<num_src_routing_node; i++){
    if(i == num_src_routing_node -1){
      config->payload[counter] = 1; // num of cell for control plane hops
    } else {
      config->payload[counter] = 0;
    }
    counter++;
  }
  
  // add flow id to config
  for(j = 0; j< 2; ++j){
    config->payload[counter] = flow_id->u8[j];
    counter++;
  }
  
  //add schedule
  if((free_cell = get_free_slot_control_plane(flow_id, sender_addr, parent_addr)) != NULL){
    config->payload[counter] = free_cell->sf_id;
    counter++;
    config->payload[counter] = free_cell->slot;
    counter++;
    config->payload[counter] = free_cell->ch_off;
    counter++;
    
    free(free_cell);
    if(my_config != NULL){
      memcpy(&my_config->packet_info.packet, config, sizeof(struct sdn_packet));
      my_config->packet_info.len = counter + 1;
      linkaddr_copy(&my_config->ack_sender, &last_hop_addr);
      return my_config;
    }
  }
  
  printf("CONTROLLER: config content: \n");
  for(i=0; i<counter; i++){
    printf("%d \n", config->payload[i]);
  }
  
  dealloc_mem_sent_config(my_config);
  return NULL;
}
/*---------------------------------------------------------------------------*/
struct cell_info * 
get_free_slot_control_plane(const linkaddr_t *flow_id, const linkaddr_t *sender_addr, const linkaddr_t *parent_addr)
{
  struct cell_info *free_cell_info = (struct cell_info *) malloc(sizeof(struct cell_info));
  struct sdn_global_table_entry *e;
  struct sdn_global_table_entry *e_parent = NULL;
  struct sdn_global_table_entry *e_child = NULL;
  int i;
  //int j;
  int find_free_slot = 0;
  int slot_exist = 0;
  int slot;
  
  if(linkaddr_cmp(flow_id, &flow_id_to_controller)){
  
    for(e = list_head(sdn_global_table); e != NULL; e = e->next){
      if(linkaddr_cmp(&e->node_addr, parent_addr)){
        while(find_free_slot == 0 ){
          slot = (random_rand() % SDN_SLOTFRAME_SIZE);
          printf("CONTROLLER: s num %d \n", slot);
          print_global_table();
          for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
            if(e->taken_slot[i] == slot){
              slot_exist = 1;
              printf("CONTROLLER: taken slot %d \n", e->taken_slot[i]);
              break;
            } else{
              slot_exist = 0;
            }
          } 
          if(!slot_exist){
            printf("CONTROLLER: !slot_exist %d \n", slot);
            find_free_slot = 1;
            free_cell_info->slot = slot;
            free_cell_info->ch_off = 0; // TODO change the ch_off
            free_cell_info->sf_id = 0; // TODO change
            
            add_slot_to_taken_cell_list(parent_addr, free_cell_info->slot, free_cell_info->ch_off);
            add_slot_to_taken_cell_list(sender_addr, free_cell_info->slot, free_cell_info->ch_off);
            if(linkaddr_cmp(flow_id, &flow_id_to_controller)){
              specify_to_ctrl_cell(sender_addr, free_cell_info->slot,free_cell_info->ch_off);
            }
            if(linkaddr_cmp(flow_id, &flow_id_best_effort)){
              specify_best_effort_cell(sender_addr, free_cell_info->slot,free_cell_info->ch_off);
            }
            return free_cell_info;
          }
        }
      }
    }
  } else if(linkaddr_cmp(flow_id, &flow_id_best_effort)){
    for(e = list_head(sdn_global_table); e != NULL; e = e->next){
      if(linkaddr_cmp(&e->node_addr, parent_addr)){
        e_parent = e;
      }
      if(linkaddr_cmp(&e->node_addr, sender_addr)){
        e_child = e;
      }
    }
    if(e_parent != NULL && e_child != NULL){
      while(find_free_slot == 0 ){
        slot = (random_rand() % SDN_SLOTFRAME_SIZE);
        printf("CONTROLLER: s num %d \n", slot);
        print_global_table();
        for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
          if(e_parent->taken_slot[i] == slot || e_child->taken_slot[i] == slot){
            slot_exist = 1;
            printf("CONTROLLER: taken slot %d \n", e_parent->taken_slot[i]);
            break;
          } else{
            slot_exist = 0;
          }
        } 
        if(!slot_exist){
          printf("CONTROLLER: !slot_exist %d \n", slot);
          find_free_slot = 1;
          free_cell_info->slot = slot;
          free_cell_info->ch_off = 0; // TODO change the ch_off
          free_cell_info->sf_id = 0; // TODO change
            
          add_slot_to_taken_cell_list(parent_addr, free_cell_info->slot, free_cell_info->ch_off);
          add_slot_to_taken_cell_list(sender_addr, free_cell_info->slot, free_cell_info->ch_off);
          if(linkaddr_cmp(flow_id, &flow_id_to_controller)){
            specify_to_ctrl_cell(sender_addr, free_cell_info->slot,free_cell_info->ch_off);
          }
          if(linkaddr_cmp(flow_id, &flow_id_best_effort)){
            specify_best_effort_cell(sender_addr, free_cell_info->slot,free_cell_info->ch_off);
          }
          return free_cell_info;
        }
      }
    }        
  } else if(linkaddr_cmp(flow_id, &flow_id_from_controller)){
    
    for(e = list_head(sdn_global_table); e != NULL; e = e->next){
      if(linkaddr_cmp(&e->node_addr, parent_addr)){
      
        if(e->from_ctrl_slot > -1){
            free_cell_info->slot = e->from_ctrl_slot;
            free_cell_info->ch_off = 0; // TODO change the ch_off
            free_cell_info->sf_id = 0;
            
            add_slot_to_taken_cell_list(sender_addr, free_cell_info->slot, free_cell_info->ch_off);
            return free_cell_info;
            
        } else{
          while(find_free_slot == 0){
            slot = random_rand() % SDN_SLOTFRAME_SIZE;
            for(i=0; i<NODE_SLOT_LIST_SIZE; i++){
              if(e->taken_slot[i] == slot){
                slot_exist = 1;
                break;
              } else{
                slot_exist = 0;
              }
            }
            if(!slot_exist){
              find_free_slot = 1;
              free_cell_info->slot = slot;
              free_cell_info->ch_off = 0; // TODO change the ch_off
              free_cell_info->sf_id = 0;
              
              add_slot_to_taken_cell_list(parent_addr, free_cell_info->slot, free_cell_info->ch_off);
              add_slot_to_taken_cell_list(sender_addr, free_cell_info->slot, free_cell_info->ch_off);
              specify_from_ctrl_cell(parent_addr, free_cell_info->slot,free_cell_info->ch_off);
              
              return free_cell_info;
            }
          }
        }
      }
    }
  } else{
    printf("CONTROLLER: config flow id is not valid \n");
  } 
  free(free_cell_info);
  return NULL;
}
/*---------------------------------------------------------------------------*/
struct sdn_packet_info * 
request_confing_packet(struct request_id *req_id, struct rsrc_spec_of_config *req_spec)
{
  struct sent_config_info *p = NULL;
  struct sent_config_info *dup_config = NULL;
  
  struct sdn_packet *config = create_sdn_packet();
  uint8_t is_config_for_novel_node = 1;
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);
  config->payload[CONF_SEQ_NUM_INDEX] = 0;
  config->payload[CONF_REQUEST_NUM_INDEX] = 0;

  //TODO : if req_id != dup => specify resources: fid, num_ts, ch_off
  //TODO : if dup => make config to uninstal resources-> if done THEN make a new reesources and send config
  
  dup_config = look_for_req_id_in_sent_config_list(req_id);
  
  if(dup_config == NULL){
    p = add_source_routing_info_to_config(config, &req_spec->src_addr, &req_spec->dest_addr, &req_spec->flow_id);
    if(p != NULL){
      set_timer_to_config_packet(p);
      memcpy(&p->req_id, req_id, sizeof(struct request_id));
    }
  } else{
    printf("CONTROLLER: request is duplicate! need to uninstall previous config... \n");
  } 
  
  packet_deallocate(config);
  
  return &p->packet_info;
}
/*---------------------------------------------------------------------------*/
struct sent_config_info *
look_for_req_id_in_sent_config_list(struct request_id *req_id)
{
  struct sent_config_info *e;
  if(req_id != NULL){
    for(e = list_head(sent_config_list); e != NULL; e = e->next){ 
      if(linkaddr_cmp(&e->req_id.req_sender, &req_id->req_sender) && e->req_id.is_ctrl == req_id->is_ctrl && e->req_id.req_num == req_id->req_num){
        printf("CONTROLLER: dup request_id! -> addr:is_ctrl:req_num = %d%d:%d:%d \n",req_id->req_sender.u8[0], req_id->req_sender.u8[1],
               req_id->is_ctrl, req_id->req_num);
        return e;
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
struct sent_config_info *
alloc_mem_to_sent_config(void)
{
  struct sent_config_info *e = NULL;
  e = memb_alloc(&sent_config_list_mem);
  if(e != NULL){
    e->seq_num = -1;
    e->transmission = 0;
    e->is_acked = 0;
    e->req_id.is_ctrl = -1;
    e->req_id.req_num = -1;
    list_add(sent_config_list, e);
  }
  return e;
}
/*---------------------------------------------------------------------------*/
void
dealloc_mem_sent_config(struct sent_config_info* p)
{
  if(p != NULL){
    list_remove(sent_config_list, p);
    memb_free(&sent_config_list_mem, p);
  }
}
/*---------------------------------------------------------------------------*/
void
dealloc_mem_config_timer(struct config_timer* t)
{
  if(t != NULL){
    list_remove(timer_list, t);
    memb_free(&timer_list_mem, t);
    printf("CONTROLLER: remove timer \n");
  }
}
/*---------------------------------------------------------------------------*/
void 
set_timer_to_config_packet(struct sent_config_info* sent_config)
{ 
  int seq_num = -1;
  struct config_timer *t;
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->node_addr, &sent_config->ack_sender)){
      e->seq_num = e->seq_num + 1;
      seq_num = e->seq_num;
    }
  }
  for(t = list_head(timer_list); t != NULL; t = t->next){ 
    if(linkaddr_cmp(&t->ack_sender, &sent_config->ack_sender) && t->seq_num == seq_num){
      printf("CONTROLLER: timer is already set \n");
    }
  }
  
  if(t == NULL){
    t = memb_alloc(&timer_list_mem);
    if(t != NULL){
      linkaddr_copy(&t->ack_sender, &sent_config->ack_sender);
      t->seq_num = seq_num;
      
      PROCESS_CONTEXT_BEGIN(&conf_handle_process);
      etimer_set(&t->timer, PERIOD_RESEND_CONFIG);
      PROCESS_CONTEXT_END(&conf_handle_process);
      
      list_add(timer_list, t);
      
      /* set sent_config entry */
      sent_config->packet_info.packet.payload[CONF_SEQ_NUM_INDEX] = seq_num;
      sent_config->seq_num = seq_num;
      sent_config->transmission = 1;
      sent_config->is_acked = 0;
    }  
  }
}
/*---------------------------------------------------------------------------*/
void 
handle_config_ack_packet(struct sdn_packet* ack)
{
  linkaddr_t ack_sender;
  int seq_num;
  uint8_t j;
  struct sent_config_info *e;
  
  if(ack != NULL){
    for(j = 0; j< LINKADDR_SIZE; ++j){
      ack_sender.u8[j] = ack->payload[ACK_SENDER_ADDR_INDEX + j];
    }
    seq_num = ack->payload[ACK_SEQ_NUM_INDEX];
      
    for(e = list_head(sent_config_list); e != NULL; e = e->next){ 
      if(linkaddr_cmp(&e->ack_sender, &ack_sender) && e->seq_num == seq_num){
        e->is_acked = 1;
      }
    }
    print_sent_config_list();
  }
}
/*---------------------------------------------------------------------------*/
void
handle_config_timer(struct config_timer *t)
{
  struct sent_config_info *e;
  int is_need_remove_timer = 0;
  
  if(t != NULL){
    for(e = list_head(sent_config_list); e != NULL; e = e->next){ 
      if(linkaddr_cmp(&e->ack_sender, &t->ack_sender) && e->seq_num == t->seq_num){
        if(e->is_acked == 1 || e->transmission >= MAX_CONFIG_TRANSMISSION){
          is_need_remove_timer = 1;
          break;
        }
      }
    }
  
    if(is_need_remove_timer){
      dealloc_mem_config_timer(t);
    } else{
      PROCESS_CONTEXT_BEGIN(&conf_handle_process);
      etimer_set(&t->timer, PERIOD_RESEND_CONFIG);
      PROCESS_CONTEXT_END(&conf_handle_process);
      
      e->transmission = e->transmission + 1;
      sdn_handle_config_packet(&e->packet_info.packet, e->packet_info.len, &linkaddr_null);
      printf("CONTROLLER: resend config packet \n");
    }  
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(conf_handle_process, ev, data)
{ 
  static struct config_timer *t;
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(PROCESS_EVENT_TIMER);
    for(t = list_head(timer_list); t != NULL; t = t->next){ 
      if((struct etimer*)data == (&t->timer)){
        printf("CONTROLLER: timer is over-> seq num \n");
        LOG_INFO_LLADDR(&t->ack_sender);
        LOG_INFO_("\n");
        handle_config_timer(t);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
struct list_of_shared_cell *
get_shared_cell_list(void)
{
  // initialize cell list array
  shared_cell.ch_off = 0;
  int i;
  for(i=0; i<SHARED_CELL_LIST_LEN; i++){
    shared_cell.cell_list[i] = -1;
  }
  
  int sh_slot_indx = (int)(SDN_SLOTFRAME_SIZE/SDN_NUM_SHARED_CELL);
  int j = 0;
    
  if(SDN_NUM_SHARED_CELL <= SHARED_CELL_LIST_LEN){
    for(i=0; i<SDN_NUM_SHARED_CELL; i++){
      shared_cell.cell_list[i] = j;
      j = j + sh_slot_indx;
    }
  } else{
    for(i=0; i<SHARED_CELL_LIST_LEN; i++){
      shared_cell.cell_list[i] = j;
      j = j + sh_slot_indx;
    }
    printf("CONTROLLER: SDN_NUM_SHARED_CELL exceeds shard cell array len\n");
  }
  
  // print shared cells
  printf("CONTROLLER: list of shared cells:\n");  // must be removed
  for(i=0; i<SHARED_CELL_LIST_LEN; i++){
    if(shared_cell.cell_list[i] > -1){
      printf(" %d", shared_cell.cell_list[i]);
    }
  }
  printf("\n");
  
  return &shared_cell;
}
/*---------------------------------------------------------------------------*/
struct request_id * 
get_request_id(struct sdn_packet* p)
{
  struct request_id *req_id = (struct request_id *) malloc(sizeof(struct request_id));
  linkaddr_t sender_addr;
  int j;
     
  for(j = 0; j< LINKADDR_SIZE; ++j){
    sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
  }
     
     
  if((p->typ & 0x0f) == REPORT){
    if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
      printf("node is not registered\n");
      req_id->req_num = flow_id_to_controller.u8[0];
    }
    else if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) > 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
      printf("node has to-ctrl, does not have from-ctrl \n");
      req_id->req_num = flow_id_from_controller.u8[0];
    }
    else if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) > 0) {
      printf("node has from-ctrl, does not have to-ctrl \n");
      req_id->req_num = flow_id_to_controller.u8[0];
    } 
    else if((p->payload[R_IS_JOIN_INDEX] & 0x03) == 0){
      printf("node is registered, with no best effort fid\n");
      req_id->req_num = flow_id_best_effort.u8[0];
    }
    else{
      req_id->req_num = 0;
    }
    
    req_id->is_ctrl = 1;
    linkaddr_copy(&req_id->req_sender, &sender_addr);
  }
  if((p->typ & 0x0f) == REQUEST){
    req_id->is_ctrl = 0;
    req_id->req_num = p->payload[REQ_NUM_OF_REQ];
    linkaddr_copy(&req_id->req_sender, &sender_addr);
  }
  
  return req_id;
}
/*---------------------------------------------------------------------------*/
/* this function determines the resource spec from the content of request packet */
struct rsrc_spec_of_config *
get_resource_spec_from_request(struct sdn_packet* p, struct request_id *req_id)
{
  struct rsrc_spec_of_config *req_spec = (struct rsrc_spec_of_config *) malloc(sizeof(struct rsrc_spec_of_config));
  
  linkaddr_t sender_addr;
  linkaddr_t *free_flow_id;
  int j;
  int is_true;
     
  for(j = 0; j< LINKADDR_SIZE; ++j){
    sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
  }
  
  if((p->typ & 0x0f) == REPORT){
    switch(req_id->req_num){
      case 0:
        printf("CONTROLLER: register node\n");
        break;

      case 2:
        printf("CONTROLLER: not register node: case 2\n");          
        linkaddr_copy(&req_spec->flow_id, &flow_id_to_controller);
        if((is_true = specify_parent_from_report(p)) == -1){
          printf("CONTROLLER: cannot specify parent addr\n");
        }
        break;

      case 3:
        printf("CONTROLLER: has to_ctrl, has not from-ctrl: case 3\n");       
        linkaddr_copy(&req_spec->flow_id, &flow_id_from_controller);
        break;

      case 4:
        printf("CONTROLLER: register, no best effort: case 4\n");      
        linkaddr_copy(&req_spec->flow_id, &flow_id_best_effort);
        break;
    }
  
    linkaddr_copy(&req_spec->src_addr, &sender_addr); //TODO must be changed after changing config func
    linkaddr_copy(&req_spec->dest_addr, get_parent_from_node_addr(&sender_addr));
    req_spec->sf_id = 0;
    req_spec->num_ts = 1;
    req_spec->ch_off = 0;
  }
  
  if((p->typ & 0x0f) == REQUEST){
    req_spec->sf_id = 0;
    req_spec->num_ts = 1;
    req_spec->ch_off = 0;
    if((free_flow_id = get_free_flow_id()) == NULL){
      printf("CONTROLLER: there is no free flow id\n");
      free(req_spec);
      return NULL;
    }
    linkaddr_copy(&req_spec->flow_id, free_flow_id);
    linkaddr_copy(&req_spec->src_addr, &sender_addr);
    //get dest lladdr from IP addr
    for(j = 0; j< LINKADDR_SIZE; ++j){
      req_spec->dest_addr.u8[j] = p->payload[REQ_REMOTE_IPADDR_INDEX + 14 + j];
    }
  }
  
  return req_spec;
}
/*---------------------------------------------------------------------------*/
linkaddr_t * 
get_free_flow_id(void)
{
  if(last_flow_id.u8[0] < 255){
    last_flow_id.u8[0] = last_flow_id.u8[0] + 1;
    return &last_flow_id;
  } else if(last_flow_id.u8[0] == 255 && last_flow_id.u8[1] < 255){
    last_flow_id.u8[1] = last_flow_id.u8[1] + 1;
    return &last_flow_id;
  } else{
    printf("CONTROLLER: there is no free flow id\n");
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
sdn_send_packet_to_controller(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id)
{
  int i;
  struct sdn_packet *p = create_sdn_packet();
  memcpy((uint8_t*)p, buf, len);

  printf("count %u\n", p->typ);
  for(i=0; i<len-1; i++){
    printf("count %u\n", p->payload[i]);
  }
  
  struct sdn_packet_info *created_conf = NULL;
  struct rsrc_spec_of_config *req_spec = NULL;
  struct request_id *req_id = NULL;
  
  switch(p->typ & 0x0f){
    case CONFIG_ACK:
      printf("sdn-handle: receive config ack \n");
      handle_config_ack_packet(p);
      break;
  
    case REQUEST:
      //TODO get req_id
      //TODO extract the src and dest addr
      printf("CONTROLLER: receive request flow \n");
      
      req_id = get_request_id(p);        
      req_spec = get_resource_spec_from_request(p, req_id);
      
      if(req_id != NULL){
        printf("CONTROLLER: request id-> addr:is_ctrl:req_num = %d%d:%d:%d \n",req_id->req_sender.u8[0], req_id->req_sender.u8[1],
               req_id->is_ctrl, req_id->req_num);
      }
      if(req_spec != NULL){
        printf("CONTROLLER: req spec: dest addr: %d%d flow id: %d%d \n",req_spec->dest_addr.u8[0], req_spec->dest_addr.u8[1],
               req_spec->flow_id.u8[0], req_spec->flow_id.u8[1]);
      }
      free(req_id); 
      free(req_spec);
      /*struct sdn_packet *data_config;
      if(p->payload[1] == 0x03){
        data_config = create_data_confing_node_new(p->payload[REQ_NUM_OF_REQ]);
        sdn_handle_config_packet(data_config, 18, &linkaddr_null);
        packet_deallocate(data_config);
      }*/
      break;
      
    case REPORT:
              {      
      req_id = get_request_id(p);        
      req_spec = get_resource_spec_from_request(p, req_id);
      
      if(req_id->req_num > 1){
        if(&req_spec->dest_addr != NULL && req_spec != NULL){
          created_conf = request_confing_packet(req_id, req_spec);
        } 
        if(created_conf != NULL){
          printf("created conf-best len %u\n", created_conf->len);
          for (i=0; i<created_conf->len -1; i++){
            printf("created config-best %u\n", created_conf->packet.payload[i]);
          }
          sdn_handle_config_packet(&created_conf->packet, created_conf->len, &linkaddr_null);
          print_global_table();
        }  
      }  
      free(req_id); 
      free(req_spec);
      break;
      }
           
    case CONFIG:
      sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
      break;
  }
 packet_deallocate(p);
}
/*---------------------------------------------------------------------------*/
/* initialize controller  */
void 
sdn_controller_init(void)
{
  process_start(&conf_handle_process, NULL);
  
  linkaddr_t sink_addr;
  linkaddr_copy(&sink_addr, &sink_address);
  
  memb_init(&sdn_global_table_mem);
  list_init(sdn_global_table);
#if SINK
  sdn_add_node_to_global_table(&sink_addr, &sink_addr);
  print_global_table();
#endif

  memb_init(&timer_list_mem);
  list_init(timer_list);

  memb_init(&sent_config_list_mem);
  list_init(sent_config_list);

}
/*---------------------------------------------------------------------------*/



