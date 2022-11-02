#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-algo.h"
#include "net/linkaddr.h"
#include "net/nbr-table.h"
#include <stdio.h>
#include "random.h"
#include "lib/memb.h"
#include <malloc.h>
#include <math.h>
#include "lib/list.h"

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_SINK_LOG_LEVEL


static float POS_ARRAY[43][2] = {{0, 0}, {9.12, -36.06}, {-26.73, 27.66}, {22.71, 4.14}, {-19.98, -13.31}, {-15.59, 4.91}, {12.55, 15.9}, {-35.23, -6.45}, {13.41, -14.81}, {20.57, 20.81}, {-25.49, -21.44}, {-25.77, 16.84}, {-6.56, 16.93}, {27.02, -47.05}, {-6.04, -41.93}, {25.2, -39.95}, {-1.0, -46.9}, {17.27, -65.91}, {5.53, -60.18}, {-35.79, 57.72}, {-39.91, 49.28}, {-26.13, 64.71}, {-52.21, 52.12}, {-37.44, 61.16}, {0.87, 53.18}, {-46.6, 11.01}, {-48.11, 24.87}, {-17.29, 52.13}, {-53.49, 23.07}, {51.08, 27.64}, {44.6, 28.7}, {44.81, 29.31}, {40.09, 14.16}, {-43.09, -13.38}, {32.31, 43.15}, {50.7, 38.89}, {-46.93, -44.62}, {-44.31, -48.36}, {-6.33, -73.07}, {-20.64, 72.88}, {-18.8, 69.2}, {67.9, 16.65}, {-49.86, -51.63}};

static linkaddr_t last_flow_id = {{ 0x08, 0x00 }};
static float coef = 0.5;
#define ADMIT_FLOW_PDR        0.85    // in the joining process the selected parent must have EB pdr above ADMIT_FLOW_PDR
//static float coef_link_pdr = 0.9;
#define GLOBAL_TS_LIST_LEN    SDN_DATA_SLOTFRAME_SIZE
#define GLOBAL_CH_LIST_LEN    SDN_MAX_CH_OFF_NUM
static int global_ts_list[GLOBAL_CH_LIST_LEN][GLOBAL_TS_LIST_LEN];

int sdn_num_shared_cell = 0;
int sdn_num_shared_cell_in_rep = 0;

/* alloc size of shared cell matrix to control EB sending */
#define max_sf_offset_num      (int)((TSCH_EB_PERIOD/10) / SDN_DATA_SLOTFRAME_SIZE)
static int shared_cell_alloc_list[max_sf_offset_num][SDN_DATA_SLOTFRAME_SIZE];

#define SHARED_CELL_RATIO ((int)((float)NETWORK_SIZE/max_sf_offset_num + 2) * max_sf_offset_num)
//static int shared_cell_shuffle_list[12] = {1, 5, 3, 2, 6, 4, -1, -1, -1, -1, -1, -1};


linkaddr_t sink_addr;
#define MAX_NUM_PENDING_CONFIG   3

/*---------------------------------------------------------------------------*/
/* Declaration */

void init_global_ts_list(void);

void init_num_shared_cells(void);

int add_slot_to_global_ts_list(int slot, int ch_off);

void add_shared_cells_to_global_ts_list(void);

int find_free_channels_for_ts(int slot, int rep_period, int *channel);

struct sent_config_info * alloc_mem_to_sent_config(void);

void dealloc_mem_sent_config(struct sent_config_info* p);

struct sent_config_info * add_source_routing_info_to_config(struct request_id *req_id, struct rsrc_spec_of_config *req_spec);

int set_timer_to_config_packet(struct sent_config_info* sent_config);

int add_schedule_to_config(struct sdn_packet *config, struct rsrc_spec_of_config *req_spec, struct request_id *req_id);

int exist_active_timer_for_config(struct request_id *req_id);

int specify_shared_cell_offset_for_node(const linkaddr_t *node_addr);

void alloc_eb_shared_cell_to_sink(void);

int get_first_free_shared_timeslot(void);

int calculate_num_shared_cell(void);

/*---------------------------------------------------------------------------*/
/* list of shared cells */
struct list_of_shared_cell shared_cell;

/*---------------------------------------------------------------------------*/
PROCESS(conf_handle_process, "conf handle process");
//AUTOSTART_PROCESSES(&conf_handle_process);
/*---------------------------------------------------------------------------*/
#define MAX_CONFIG_TRANSMISSION  4
struct sent_config_info{
  struct sent_config_info *next;
  struct request_id req_id;
  struct sdn_packet_info packet_info;
  linkaddr_t ack_sender;
  int seq_num;
  int transmission;
  int is_acked;
};
#define SENT_CONFIG_LIST_SIZE 300
LIST(sent_config_list);
MEMB(sent_config_list_mem, struct sent_config_info, SENT_CONFIG_LIST_SIZE);
/*---------------------------------------------------------------------------*/
#define PERIOD_RESEND_CONFIG    (50 * CLOCK_SECOND)
#define NUM_TIMER 200
LIST(timer_list);
MEMB(timer_list_mem, struct config_timer, NUM_TIMER);
/*---------------------------------------------------------------------------*/
#define NODE_NUM_IN_NETWORK 100
LIST(global_nbr_list);
MEMB(global_nbr_list_mem, struct nbrs_info, NODE_NUM_IN_NETWORK);
/*---------------------------------------------------------------------------*/
#define NODE_SLOT_LIST_SIZE       SDN_DATA_SLOTFRAME_SIZE
#define NODE_CHAN_LIST_SIZE       2  // each node only can have one link per ts
struct sdn_global_table_entry {
  struct sdn_global_table_entry *next;
  linkaddr_t node_addr;
  linkaddr_t parent_addr;
  int ctrl_taken_slot[NODE_CHAN_LIST_SIZE][NODE_SLOT_LIST_SIZE];
  int data_taken_slot[NODE_CHAN_LIST_SIZE][NODE_SLOT_LIST_SIZE];
  int to_ctrl_slot;
  int from_ctrl_slot;
  int best_effort_slot;
  int seq_num;
  int shared_cell_sf_offset;  // specify SF offset to send EB
  int shared_cell_ts_offset;  // specify timeslot offset to send EB
  //int ch_off;
};
#define NUM_GLOBAL_TABLE_ENTRY 100
LIST(sdn_global_table);
MEMB(sdn_global_table_mem, struct sdn_global_table_entry, NUM_GLOBAL_TABLE_ENTRY);
/*---------------------------------------------------------------------------*/
int
specify_parent_from_report(struct sdn_packet* p)
{
  if(p->payload[R_METRIC_TYP_INDEX] == EB_NUM && p->payload[R_NBR_NUB_INDEX] > 0) {
    int i;
    int j;
    int counter = R_NBR_NUB_INDEX + 1;
    int nbr_eb_number = 0;
    linkaddr_t addr;
    linkaddr_t parent_addr;
    linkaddr_t sender_addr;
    float exp_eb_num = ((float)SDN_REPORT_PERIOD / ((float)TSCH_EB_PERIOD/(float)CLOCK_SECOND));
    
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
    }
  
    for(i=0; i<p->payload[R_NBR_NUB_INDEX]; i++) {
      for(j = 0; j< LINKADDR_SIZE; ++j) {
        addr.u8[j] = p->payload[counter];
        counter++;
      }
      if(p->payload[counter] > nbr_eb_number) {
        nbr_eb_number = p->payload[counter];
        linkaddr_copy(&parent_addr, &addr);
        //LOG_INFO("CONTROLLER: specify parent 1 \n");
      }
      counter++;
    }
    
    
    if(((float)(float)nbr_eb_number / exp_eb_num) < ADMIT_FLOW_PDR) {
      LOG_INFO("CONTROLLER: no qualified parent for node:%d%d : Max NBRs links' quality %f lower than %f \n", sender_addr.u8[0],
      sender_addr.u8[1], (float)((float)nbr_eb_number / exp_eb_num), ADMIT_FLOW_PDR);
      return -1;
    } else {
      LOG_INFO("CONTROLLER: parent link quality: %f ]\n", (float)((float)nbr_eb_number / exp_eb_num));
    }
    
    if(!linkaddr_cmp(&parent_addr, &linkaddr_null)) {
      LOG_INFO("CONTROLLER: specify parent addr ");
      LOG_INFO_LLADDR(&parent_addr);
      LOG_INFO_("\n");
      if((i = sdn_add_node_to_global_table(&sender_addr, &parent_addr)) == 1) {
        //alloc_choff_to_novel_node(p);
        register_node_to_global_nbr_table(&sender_addr);
        //print_global_table(); //must be removed
        return 1;
      }
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/*int
specify_shared_cell_offset_for_node(const linkaddr_t *node_addr)
{
  struct sdn_global_table_entry *e;
  
  if(node_addr == NULL) {
    return 0;
  }
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
      break;
    }
  }
  
  if(e != NULL) {
    int sf_of;
    int find_cell = 0;
    int i;
    int j;
    int period_of_shared_cell = SDN_SF_REP_PERIOD / sdn_num_shared_cell_in_rep;
    for(j=1; j<=(sdn_num_shared_cell * max_sf_offset_num); j++) {
      
      int k;
      int ith_shared_cell_in_sf;
      //printf("shared cell ratio %d \n", SHARED_CELL_RATIO);
      for(k=0; k<SHARED_CELL_RATIO; k++) {
        if(shared_cell_shuffle_list[k] == j) {
          if(sdn_num_shared_cell != 0) {
            sf_of = (int)(k / (int)(sdn_num_shared_cell));
          } else {
            printf("Controller: sdn_num_shared_cell is zero\n");
          }
          ith_shared_cell_in_sf = k % (sdn_num_shared_cell);
          printf("shared cell ratio1: k:%d, ts: %d sf: %d\n", k, ith_shared_cell_in_sf, sf_of);
          break;
        }
      }
      
      printf("shared cell ratio %d \n", period_of_shared_cell);
      
      printf("shared cell ratio %d, %d \n", sf_of, ith_shared_cell_in_sf);
      for(i=0; i<SDN_DATA_SLOTFRAME_SIZE; i++) {
        if((i % period_of_shared_cell == 0) && (i / period_of_shared_cell == ith_shared_cell_in_sf) && (shared_cell_alloc_list[sf_of][i] == 0)) {
          shared_cell_alloc_list[sf_of][i] = node_addr->u8[1];  // as node id
          e->shared_cell_sf_offset = sf_of;
          e->shared_cell_ts_offset = i;
          find_cell = 1;
          printf("sf offset for node: %d, %d\n", e->shared_cell_sf_offset, e->shared_cell_ts_offset);
          break;
        }
      }
      if(find_cell) {
        return 1;
      }      
    }  
  }
  return 0;
} 
*/
/*---------------------------------------------------------------------------*/
int
specify_shared_cell_offset_for_node(const linkaddr_t *node_addr)
{
  struct sdn_global_table_entry *e;
  
  if(node_addr == NULL) {
    return 0;
  }
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
      break;
    }
  }

  if(e != NULL) {
    int sf_of;
    
    int ts = get_first_free_shared_timeslot();
    
    for(sf_of=0; sf_of<max_sf_offset_num; sf_of++) {
      if(ts != -1) {
          shared_cell_alloc_list[sf_of][ts] = node_addr->u8[1];  // as node id
          e->shared_cell_sf_offset = sf_of;
          e->shared_cell_ts_offset = ts;
          //printf("sf offset for node: %d, %d\n", e->shared_cell_sf_offset, e->shared_cell_ts_offset);
          return 1;
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* registre node to global nbr table and init the list to node */
int 
register_node_to_global_nbr_table(const linkaddr_t *node_addr)
{
  struct nbrs_info *e;
  for(e = list_head(global_nbr_list); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
      break;
    }
  }
  
  if(e == NULL) {
    e = memb_alloc(&global_nbr_list_mem);
    if(e != NULL) {
      int i;
      linkaddr_copy(&e->node_addr, node_addr);
      for(i=0; i<NBR_LIST_LEN; i++) {
        linkaddr_copy(&e->nbr_list[i].nbr_addr, &linkaddr_null);
        e->nbr_list[i].eb = -1; // consider 90% initial reliability for each link 
        //LOG_INFO("CONTROLLER: eb coef register %f\n", e->nbr_list[i].eb);
        e->nbr_list[i].rssi = -255;
        e->nbr_list[i].total_eb_num = 0;
        e->nbr_list[i].total_report_count = 0;
      } 
      
      list_add(global_nbr_list, e);
      LOG_INFO("CONTROLLER: register %d%d to global nbr table \n", node_addr->u8[0], node_addr->u8[1]);
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void 
update_node_nbr_list(struct sdn_packet* p)
{
  int j;
  int i;
  linkaddr_t sender_addr;
  linkaddr_t addr;
  struct nbrs_info *e;
  
  
  if(p != NULL) {
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
    }
  }
  
  
  for(e = list_head(global_nbr_list); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, &sender_addr)) {
      break;
    }
  }
  //TODO need to flash nbr list each time? or use EMWG?
  if(e != NULL) {
    int k;
    int counter = R_NBR_NUB_INDEX + 1;
    int nbr_exist;
    for(i=0; i<p->payload[R_NBR_NUB_INDEX]; i++) {
      //LOG_INFO("CONTROLLER: update EB %d \n", p->payload[R_NBR_NUB_INDEX]);
      for(j = 0; j< LINKADDR_SIZE; ++j) {
        addr.u8[j] = p->payload[counter];
        counter++;
      }
      nbr_exist = 0;
      for(k=0; k<NBR_LIST_LEN; k++) {
        if(linkaddr_cmp(&e->nbr_list[k].nbr_addr, &addr)) {
          if(p->payload[R_METRIC_TYP_INDEX] == EB_NUM) {
            if(e->nbr_list[k].eb != -1) {
              e->nbr_list[k].eb = (coef * e->nbr_list[k].eb) + ((1-coef) * (float)p->payload[counter]);
              e->nbr_list[k].total_eb_num = e->nbr_list[k].total_eb_num + p->payload[counter];
              e->nbr_list[k].total_report_count++;
            } else {
              e->nbr_list[k].eb = (float)p->payload[counter]; // first EB count for this NBR
            }       
            //LOG_INFO("CONTROLLER: EB-report: node:[%d%d], eb from:[%d%d]: EB: %d, eb_avg[%.2f]] \n", sender_addr.u8[0], sender_addr.u8[1],
            //                                          addr.u8[0], addr.u8[1], p->payload[counter], e->nbr_list[k].eb);
          }
          if(p->payload[R_METRIC_TYP_INDEX] == RSSI) {
            e->nbr_list[k].rssi = p->payload[counter];
          }
          nbr_exist = 1;
          break;
        }  
      }
      if(!nbr_exist) {
        for(k=0; k<NBR_LIST_LEN; k++) {
          if(linkaddr_cmp(&e->nbr_list[k].nbr_addr, &linkaddr_null)) {
            linkaddr_copy(&e->nbr_list[k].nbr_addr, &addr);
            if(p->payload[R_METRIC_TYP_INDEX] == EB_NUM) {
              if(e->nbr_list[k].eb != -1) {
                e->nbr_list[k].eb = (coef * e->nbr_list[k].eb) + ((1-coef) * (float)p->payload[counter]);
              } else {
                e->nbr_list[k].eb = ((float)(SDN_REPORT_PERIOD/(((float)TSCH_EB_PERIOD/(float)CLOCK_SECOND)))) * 0.85;
              }
              //LOG_INFO("CONTROLLER: EB-report: node:[%d%d], (first) eb from:[%d%d]: EB: %d, eb_avg[%.2f]] \n", sender_addr.u8[0], sender_addr.u8[1], addr.u8[0], addr.u8[1], p->payload[counter], e->nbr_list[k].eb);
              //LOG_INFO("CONTROLLER: add nbr %d%d to list \n", addr.u8[0], addr.u8[1]);
            }
            if(p->payload[R_METRIC_TYP_INDEX] == RSSI) {
              e->nbr_list[k].rssi = p->payload[counter];
            }
            break;
          }
        }
      }
      counter++;
    }
  } else{
    LOG_ERR("CONTROLLER: fail to update report packet -> node does not exist \n");
  }
}
/*---------------------------------------------------------------------------*/
linkaddr_t *
get_parent_from_node_addr(const linkaddr_t *node_addr)
{
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
      return &e->parent_addr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int 
add_slot_to_global_ts_list(int slot, int ch_off)
{
  if(slot<GLOBAL_TS_LIST_LEN && ch_off < GLOBAL_CH_LIST_LEN) {
    global_ts_list[ch_off][slot] = slot;
    return 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int 
add_slot_to_taken_cell_list(const linkaddr_t *node_addr, int is_ctrl_cell, int slot_offset, int ch_offset)
{ 
  //int i;
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
  /*    if(is_ctrl_cell == 1) {
        for(i=0; i<NODE_SLOT_LIST_SIZE; i++) {
          if(e->ctrl_taken_slot[i] == -1) {
            e->ctrl_taken_slot[i] = slot_offset;
            LOG_INFO("CONTROLLER: add slot %d to node \n", e->ctrl_taken_slot[i]);
            LOG_INFO_LLADDR(&e->node_addr);
            LOG_INFO("\n");
            break;
          } 
          if(e->ctrl_taken_slot[i] != -1 && i == NODE_SLOT_LIST_SIZE-1) {
            LOG_INFO("CONTROLLER: NODE_SLOT_LIST_SIZE exceeds, no free space to allocate! \n");
          }
        }
      } 
*/     
     if(slot_offset >= NODE_SLOT_LIST_SIZE) {
        LOG_ERR("CONTROLLER: NODE_SLOT_LIST_SIZE exceeds, no free space to allocate! \n");
        return -1;
     }
     if(e->data_taken_slot[0][slot_offset] == -1) {
        e->data_taken_slot[0][slot_offset] = slot_offset;
        e->data_taken_slot[1][slot_offset] = ch_offset;
        //LOG_INFO("CONTROLLER: add slot %d with ch %d to node \n", e->data_taken_slot[0][slot_offset], e->data_taken_slot[1][slot_offset]);
        //LOG_INFO_LLADDR(&e->node_addr);
        //LOG_INFO("\n");
        return 1;
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
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
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
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
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
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
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
  int i, j;
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    LOG_INFO("GLOBAL TABLE: node addr:");
    LOG_INFO_LLADDR(&e->node_addr);
    LOG_INFO_("\n");
    LOG_INFO(", parent addr:");
    LOG_INFO_LLADDR(&e->parent_addr);
    LOG_INFO_("\n");
    LOG_INFO(" taken slots: ctrl ");
/*
    printf("GLOBAL TABLE: node addr:");
    LOG_INFO_LLADDR(&e->node_addr);
    printf("\n");
    LOG_INFO(", parent addr:");
    LOG_INFO_LLADDR(&e->parent_addr);
    printf("\n");
    printf(" taken slots: ctrl ");
*/
/*
    for(i=0; i<NODE_SLOT_LIST_SIZE; i++) {
      if(e->ctrl_taken_slot[i] != -1) {
        LOG_INFO("%d ", e->ctrl_taken_slot[i]);
      } 
    }
*/

    LOG_INFO(" data: ");
    for(i=0; i<NODE_SLOT_LIST_SIZE; i++) {
      if(e->data_taken_slot[0][i] != -1) {
        LOG_INFO("<%d,%d> ", e->data_taken_slot[0][i], e->data_taken_slot[1][i]);
        //printf("<%d,%d> ", e->data_taken_slot[0][i], e->data_taken_slot[1][i]);
      } 
    }
    
    LOG_INFO(",TC: %d ", e->to_ctrl_slot);
    LOG_INFO(",FC: %d ", e->from_ctrl_slot);
    LOG_INFO(",BF: %d ", e->best_effort_slot);
    //LOG_INFO(",ch_off: %d \n", e->ch_off);
    LOG_INFO("\n");
  }
  LOG_INFO("\n");
  for(i=0; i<GLOBAL_CH_LIST_LEN; i++) {
    for(j=0; j<GLOBAL_TS_LIST_LEN; j++) {
      if(global_ts_list[i][j] != -1) {
        LOG_INFO("%d ", global_ts_list[i][j]);
      } 
    }
  }
  LOG_INFO("\n");
}
/*---------------------------------------------------------------------------*/
void 
print_sent_config_list(void)
{
  struct sent_config_info *e;

  LOG_INFO("List of sent config\n");    
  for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
    LOG_INFO("ack sender: ");
    LOG_INFO_LLADDR(&e->ack_sender);
    LOG_INFO(" ,seq num %d ", e->seq_num);
    LOG_INFO(",is acked %d ", e->is_acked);
    LOG_INFO(",transmission %d ", e->transmission);
    LOG_INFO(",request id %d.%d \n", e->req_id.is_ctrl, e->req_id.req_num);
  }
}
/*---------------------------------------------------------------------------*/
void
print_global_nbr_table(void)
{
  struct nbrs_info *e;
  int k;
  LOG_INFO("CONTROLLER: global nbr table\n"); 
  for(e = list_head(global_nbr_list); e != NULL; e = e->next) {
    LOG_INFO("node addr %d%d, to list: ", e->node_addr.u8[0], e->node_addr.u8[1]);
    for(k=0; k<NBR_LIST_LEN; k++) {
      if(!linkaddr_cmp(&e->nbr_list[k].nbr_addr, &linkaddr_null)) {
        LOG_INFO("%d%d -> eb:%f, ", e->nbr_list[k].nbr_addr.u8[0], e->nbr_list[k].nbr_addr.u8[1], e->nbr_list[k].eb);
      }
    }
    LOG_INFO("\n");
  }
}
/*---------------------------------------------------------------------------*/
int 
sdn_add_node_to_global_table(const linkaddr_t *node_addr, const linkaddr_t *parent_addr)
{
  struct list_of_shared_cell *shared_cell;
  struct sdn_global_table_entry *e;
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, node_addr)) {
      return -1;
    }
  }
  if(e == NULL) {
    e = memb_alloc(&sdn_global_table_mem);
    if(e != NULL) {
      linkaddr_copy(&e->node_addr, node_addr);
      linkaddr_copy(&e->parent_addr, parent_addr);
      int i, j;
      //initialize parameters
/*
      for(i=0; i<NODE_CHAN_LIST_SIZE; i++) {
        for(j=0; j<NODE_SLOT_LIST_SIZE; j++) {
          e->ctrl_taken_slot[i][j] = -1;
        }
      }
*/
      for(i=0; i<NODE_CHAN_LIST_SIZE; i++) {
        for(j=0; j<NODE_SLOT_LIST_SIZE; j++) {
          e->data_taken_slot[i][j] = -1;
        }
      }
      e->to_ctrl_slot = -1;
      e->from_ctrl_slot = -1;
      e->best_effort_slot = -1;
      e->seq_num = -1;
      e->shared_cell_sf_offset = -1;
      e->shared_cell_ts_offset = -1;
      list_add(sdn_global_table, e);
      
      //add shared cells to the list of cells 
      if((shared_cell = get_shared_cell_list()) != NULL) {
        for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
          if(shared_cell->cell_list[i] > -1) {
            //LOG_INFO("CONTROLLER: inside the func\n"); // must be removed
            add_slot_to_taken_cell_list(node_addr, 0, shared_cell->cell_list[i], 0); // set ch_off of shared cell to 0, sf =0
          }
        }
      }  
     
    }
    return 1;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int 
allocate_cell_per_hop(int sink_to_dest_num, int dest_to_src_num, struct rsrc_spec_of_config *req_spec, struct sdn_packet *config)
{ 

  if(config == NULL || req_spec== NULL) {
    return 0;
  }

  struct flow_hops_cells l[dest_to_src_num-1];
  struct flow_hops_cells l_ideal[dest_to_src_num-1]; /* Just for statistics: this list is just for calculating the ideal number of 
                                                        cell correspond to the simulator's link quality. we do it directly in the firmware.*/

  int i;
  int j;
  int k;
  int no_addr1;
  int no_addr2;
  int sum_cells = 0;
  float e2e_pdr = 1;
  float e2e_pdr_ideal = 1;
  float ideal_rx_prob = 0;
  float eb_period = ((float)TSCH_EB_PERIOD/(float)CLOCK_SECOND);
  //float exp_eb_num = ((float)SDN_REPORT_PERIOD/((eb_period - eb_period/4 + eb_period) / 2));
  float exp_eb_num = ((float)SDN_REPORT_PERIOD/eb_period);
  LOG_ERR("exp EB num: %f, exp eb period: %f\n", exp_eb_num, eb_period);
  float qos_pdr = ((float)req_spec->app_qos.reliability / 100); //just use 8 lower bit PDR value (0-100) int. if pdr is float then devide 10000
  int counter = (sink_to_dest_num * LINKADDR_SIZE) + CONF_LIST_OF_NODE_IN_PATH_INDEX;
  int hop_list_index = (config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] * LINKADDR_SIZE) + CONF_LIST_OF_NODE_IN_PATH_INDEX;
  linkaddr_t addr1;
  linkaddr_t addr2;
  struct nbrs_info *e1 = NULL;
  struct nbrs_info *e2 = NULL;
    
  for(j = 0; j< LINKADDR_SIZE; ++j) {
    addr2.u8[j] = config->payload[counter];
    counter++;
  }  
    
  // set cell num 0 for sr nodes  
  for(i=0; i<sink_to_dest_num; i++) {
    config->payload[hop_list_index] = 0;
    hop_list_index++;
  }
  
  
  
  for(i=0; i<dest_to_src_num-1; i++) {
  
    linkaddr_copy(&addr1, &addr2);
    
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      addr2.u8[j] = config->payload[counter];
      counter++;
    }
    
    for(e1 = list_head(global_nbr_list); e1 != NULL; e1 = e1->next) {
      if(linkaddr_cmp(&e1->node_addr, &addr1)) {
        break;
      }
    }
    
    for(e2 = list_head(global_nbr_list); e2 != NULL; e2 = e2->next) {
      if(linkaddr_cmp(&e2->node_addr, &addr2)) {
        break;
      }
    }
    
    if(e1 == NULL || e2 == NULL) {
      return 0;
    }
    no_addr1 = 1;
    no_addr2 = 1;
    for(k=0; k<NBR_LIST_LEN; k++) {
      if(linkaddr_cmp(&e1->nbr_list[k].nbr_addr, &addr2)) {
        
        /* get the link quality for revers direction */
        int w;
        float revers_direction_pdr = 1; 
        for(w=0; w<NBR_LIST_LEN; w++) {
          if(linkaddr_cmp(&e2->nbr_list[w].nbr_addr, &addr1)) {
            
            LOG_ERR("CONTROLLER: EB-num-back: %f, tot eb:%f for hop %d%d \n", e2->nbr_list[w].eb, 
                                          (float)e2->nbr_list[w].total_eb_num/e2->nbr_list[w].total_report_count,
                                          addr2.u8[0], addr2.u8[1]);
          
            // start making comment to change the
            /*
            if(e2->nbr_list[w].eb >= (float)exp_eb_num) {
              revers_direction_pdr = ((float)exp_eb_num - 0.001) / (float)exp_eb_num;
            } else{
              revers_direction_pdr = e2->nbr_list[w].eb / (float)exp_eb_num;
            }
            */
            // end to make comment
            /*
            if(e2->nbr_list[w].total_eb_num <= 0) {
              LOG_ERR("CONTROLLER: reject flow: no link estimation exists!\n");
              return 0;
            }
            */
            if(e2->nbr_list[w].total_eb_num > 0) {
              revers_direction_pdr = ((float)e2->nbr_list[w].total_eb_num/(float)e2->nbr_list[w].total_report_count)/(float)exp_eb_num;
              if(revers_direction_pdr > 0.999) {
                revers_direction_pdr = 0.999;
              }
              
              revers_direction_pdr = 0.95 * revers_direction_pdr;
            } else {
              revers_direction_pdr = 0;
            }
            
            no_addr2 = 0;
          }
        }
        
        linkaddr_copy(&l[i].node_addr, &addr1);
        linkaddr_copy(&l_ideal[i].node_addr, &addr1);
        
        l[i].cell_num = 1;   // min cell per hop
        l_ideal[i].cell_num = 1;
        
        
        //start making comment to change the
        /*
        if(e1->nbr_list[k].eb >= (float)exp_eb_num) {
          l[i].pdr_link = ((float)exp_eb_num - 0.001) / (float)exp_eb_num;
        } else{
          l[i].pdr_link = e1->nbr_list[k].eb / (float)exp_eb_num;
        }
        */
        // end to make comment
        
        if(e1->nbr_list[k].total_eb_num > 0) {
          l[i].pdr_link = ((float)e1->nbr_list[k].total_eb_num/(float)e1->nbr_list[k].total_report_count)/(float)exp_eb_num;
          if(l[i].pdr_link > 0.999) {
            l[i].pdr_link = 0.999;
          }
          l[i].pdr_link = 0.95 * l[i].pdr_link;
        } else {
          l[i].pdr_link = 0;
        }
        //
        
        LOG_ERR("CONTROLLER: EB-num-go: %f, tot eb:%f for hop %d%d \n", e1->nbr_list[k].eb, (float)e1->nbr_list[k].total_eb_num/e1->nbr_list[k].total_report_count, addr1.u8[0], addr1.u8[1]);
        LOG_ERR("CONTROLLER: pdr-go: %f pdr_back: %f for addr-go %d%d, addr-back %d%d \n", l[i].pdr_link, revers_direction_pdr,
                addr1.u8[0], addr1.u8[1], addr2.u8[0], addr2.u8[1]);
                
     /*   if(e1->nbr_list[k].total_eb_num <= 0) {
          LOG_ERR("CONTROLLER: reject flow: no link estimation exists!\n");
          return 0;
        } */
        
        if(l[i].pdr_link == 0 && revers_direction_pdr > 0) {
          l[i].pdr_link = revers_direction_pdr;
        } else if(l[i].pdr_link > 0 && revers_direction_pdr == 0) {
          revers_direction_pdr = l[i].pdr_link;
        } else if (l[i].pdr_link == 0 && revers_direction_pdr == 0) {
          LOG_ERR("CONTROLLER: reject flow: no link estimation exists!\n");
          return 0;
        }
        
        
        l[i].pdr_link = revers_direction_pdr * l[i].pdr_link;
        
        /************ calculate the RX probability for the ideal mode based
         ************ on the propagation model (UDGM) */
        float x1 = POS_ARRAY[(addr1.u8[1] - 1)][0];
        float y1 = POS_ARRAY[(addr1.u8[1] - 1)][1];
        float x2 = POS_ARRAY[(addr2.u8[1] - 1)][0];
        float y2 = POS_ARRAY[(addr2.u8[1] - 1)][1];

        float gdist = ((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1));
        float dist = sqrt(gdist);
        float max_dist = TX_RANGE;
        float ratio = (dist * dist)/(max_dist * max_dist);
        
        if(ratio > 1) {
          ideal_rx_prob = 0;
        } else {
          ideal_rx_prob = 1 - ratio * (1 - SDN_SIMULATION_RX_SUCCESS);
        } 
        
        l_ideal[i].pdr_link = ideal_rx_prob * ideal_rx_prob;
        
        LOG_ERR("CONTROLLER: Distance between two nodes: %.1f meter\n", dist);
        /********************************/
        
/*        
        if(l[i].pdr_link > 0.8) {
          l[i].pdr_link = 0.8 * l[i].pdr_link;
        }
*/
/*
        if(l[i].pdr_link > 0.95) {
          l[i].pdr_link = .8 * l[i].pdr_link;
        } else if(l[i].pdr_link > 0.8) {
          l[i].pdr_link = 0.85 * l[i].pdr_link;
        }
*/
/*        
        if(l[i].pdr_link < 0.45) {
          l[i].pdr_link = 0.45;
        }
*/        
        l[i].pdr_n_tx = 1 - (1 - l[i].pdr_link);
        l_ideal[i].pdr_n_tx = 1 - (1 - l_ideal[i].pdr_link);
       
        e2e_pdr = e2e_pdr * l[i].pdr_n_tx;
        e2e_pdr_ideal = e2e_pdr_ideal * l_ideal[i].pdr_n_tx;
        
        no_addr1 = 0;
        break;
      }
    } 
    if(no_addr1 == 1 || no_addr2 == 1) {
      //print_global_nbr_table();
      LOG_ERR("CONTROLLER: cell alloc per hop -> there is no addr %d%d in NBR list of %d%d\n", addr2.u8[0], addr2.u8[1], addr1.u8[0], addr1.u8[1]);
      return 0;
    }   
  }
  
  //LOG_ERR("CONTROLLER: e2e pdr first round %f \n", e2e_pdr);
  while(e2e_pdr < qos_pdr) {
    int min_cell_i = -1;
    float min_pdr = 1;
    e2e_pdr = 1;
    for(i=0; i<dest_to_src_num - 1; i++) {
      if(l[i].pdr_n_tx < min_pdr) {
        min_pdr = l[i].pdr_n_tx;
        min_cell_i = i;
      }
    }
    if(min_cell_i != -1) {
      l[min_cell_i].cell_num++;
      l[min_cell_i].pdr_n_tx = 1- pow((1 - l[min_cell_i].pdr_link), l[min_cell_i].cell_num);
    }
    
    for(i=0; i<dest_to_src_num - 1; i++) {
      e2e_pdr = e2e_pdr * l[i].pdr_n_tx;
    }  
  }
  
  
  
  
  while(e2e_pdr_ideal < qos_pdr) {
    int min_cell_i = -1;
    float min_pdr = 1;
    e2e_pdr_ideal = 1;
    for(i=0; i<dest_to_src_num - 1; i++) {
      if(l_ideal[i].pdr_n_tx < min_pdr) {
        min_pdr = l_ideal[i].pdr_n_tx;
        min_cell_i = i;
      }
    }
    if(min_cell_i != -1) {
      l_ideal[min_cell_i].cell_num++;
      l_ideal[min_cell_i].pdr_n_tx = 1- pow((1 - l_ideal[min_cell_i].pdr_link), l_ideal[min_cell_i].cell_num);
    }
    
    for(i=0; i<dest_to_src_num - 1; i++) {
      e2e_pdr_ideal = e2e_pdr_ideal * l_ideal[i].pdr_n_tx;
    }  
  }
  
  
  
  for(i=0; i<dest_to_src_num - 1; i++) {
    LOG_INFO("CONTROLLER: flow cell per hop[%d%d] est_cell[%d] est_pdr_l[%f] idl_cell[%d] idl_pdr_l[%f]] \n", l[i].node_addr.u8[0], l[i].node_addr.u8[1], l[i].cell_num, l[i].pdr_link, l_ideal[i].cell_num, l_ideal[i].pdr_link);
    config->payload[hop_list_index] = l[i].cell_num;
    hop_list_index++;
    sum_cells = sum_cells + l[i].cell_num;
  }
  
  int ideal_sum_cells = 0;
  for(i=0; i<dest_to_src_num - 1; i++) {
    //LOG_INFO("CONTROLLER: ideal cell per hop  %d%d cell %d pdr_ntx %f pdr_l %f \n", l_ideal[i].node_addr.u8[0], l_ideal[i].node_addr.u8[1], 
            //l_ideal[i].cell_num, l_ideal[i].pdr_n_tx, l_ideal[i].pdr_link);
    ideal_sum_cells = ideal_sum_cells + l_ideal[i].cell_num;
  }
  
  LOG_INFO("CONTROLLER: ideal cell number:[%d], estimated cell num:[%d] for LQR:[%.2f] and srce id:[%d%d]] \n", ideal_sum_cells, sum_cells, SDN_SIMULATION_RX_SUCCESS, req_spec->src_addr.u8[0], req_spec->src_addr.u8[1]);
  
  return sum_cells;
}
/*---------------------------------------------------------------------------*/
struct sent_config_info * 
add_source_routing_info_to_config(struct request_id *req_id, struct rsrc_spec_of_config *req_spec)
{
  if(req_id == NULL || req_spec == NULL) {
    LOG_ERR("CONTROLLER: NULL req_spec\n");
    return NULL;
  }
  
  /**************/
  struct sdn_packet *config = create_sdn_packet();
  if(config == NULL) {
    return NULL;
  }
  uint8_t is_config_for_novel_node;
  config->payload[CONF_SEQ_NUM_INDEX] = 0;
  if(req_id->is_ctrl == 1) {
    config->payload[CONF_REQUEST_NUM_INDEX] = 0;
    is_config_for_novel_node = 1;
  } else{
    config->payload[CONF_REQUEST_NUM_INDEX] = req_id->req_num;
    is_config_for_novel_node = 0;
  }
  
  config->typ = ((is_config_for_novel_node & 0x0f)<< 4)+(CONFIG & 0x0f);
  
  if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller)) {
    struct sdn_global_table_entry *e_novel_node;
  
    for(e_novel_node = list_head(sdn_global_table); e_novel_node != NULL; e_novel_node = e_novel_node->next) { 
      if(linkaddr_cmp(&e_novel_node->node_addr, &req_id->req_sender)) {
        config->payload[CONF_EB_SF_OFFS_INDEX] = e_novel_node->shared_cell_sf_offset & 0xff;
        config->payload[CONF_EB_SF_OFFS_INDEX + 1] = ((e_novel_node->shared_cell_sf_offset) >>8) & 0xff;
        
        config->payload[CONF_EB_TS_OFFS_INDEX] = e_novel_node->shared_cell_ts_offset & 0xff;
        config->payload[CONF_EB_TS_OFFS_INDEX + 1] = ((e_novel_node->shared_cell_ts_offset) >>8) & 0xff;
      }
    }
  }
  /**************/
  struct sent_config_info *my_config = alloc_mem_to_sent_config();
  
  /**************/
  /* define an array for SR(source route) */
  int i;
  int LEN_OF_SR_LIST = 60;
  struct src_route_addr{
    linkaddr_t sr_addr;
  };
  
  struct src_route_addr src_route_list[LEN_OF_SR_LIST];
  struct src_route_addr src_to_sink_list[LEN_OF_SR_LIST];
  struct src_route_addr dest_to_sink_list[LEN_OF_SR_LIST];
  struct src_route_addr data_path_list[LEN_OF_SR_LIST];
  
  for(i=0; i<LEN_OF_SR_LIST; i++) {
    linkaddr_copy(&src_route_list[i].sr_addr, &linkaddr_null);
  }
  for(i=0; i<LEN_OF_SR_LIST; i++) {
    linkaddr_copy(&src_to_sink_list[i].sr_addr, &linkaddr_null);
  }
  for(i=0; i<LEN_OF_SR_LIST; i++) {
    linkaddr_copy(&dest_to_sink_list[i].sr_addr, &linkaddr_null);
  }
  for(i=0; i<LEN_OF_SR_LIST; i++) {
    linkaddr_copy(&data_path_list[i].sr_addr, &linkaddr_null);
  }
  /*************/
  
  int sum_cell_hops = 0;
  uint8_t j;
  uint8_t num_node_from_sink_to_dest;
  uint8_t curr_list_index = LEN_OF_SR_LIST;
  int counter = CONF_LIST_OF_NODE_IN_PATH_INDEX;
  //struct cell_info *free_cell = NULL;
  struct sdn_global_table_entry *e;
  linkaddr_t temp_par_addr;
  
  /* extract SR from dest to root */
  if(!linkaddr_cmp(&sink_address, &req_spec->dest_addr)) {
  
    linkaddr_copy(&temp_par_addr, get_parent_from_node_addr(&req_spec->dest_addr));
    curr_list_index--;
    linkaddr_copy(&src_route_list[curr_list_index].sr_addr, &temp_par_addr);
    
    while(!linkaddr_cmp(&sink_address, &temp_par_addr)) {
      for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
        if(linkaddr_cmp(&e->node_addr, &temp_par_addr)) {
          linkaddr_copy(&temp_par_addr, &e->parent_addr);
          curr_list_index--;
          linkaddr_copy(&src_route_list[curr_list_index].sr_addr, &temp_par_addr);
          break;
        }
      }
    }
  }
  num_node_from_sink_to_dest = LEN_OF_SR_LIST - curr_list_index;
  
  //insert SR from sink to dest in config packet
  for(i=0; i<num_node_from_sink_to_dest; i++) {
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      config->payload[counter] = src_route_list[curr_list_index + i].sr_addr.u8[j];
      counter++;
    }
  }
  
  /* extract path from dest to src */
  int src_list_index = LEN_OF_SR_LIST;
  int dest_list_index = LEN_OF_SR_LIST;
  int data_path_list_index = LEN_OF_SR_LIST;
  uint8_t num_node_dest_to_sink;
  uint8_t num_node_src_to_sink;
  uint8_t num_data_node;
  
  // dest(destination) to sink
  linkaddr_copy(&temp_par_addr, &req_spec->dest_addr);
  dest_list_index--;
  linkaddr_copy(&dest_to_sink_list[dest_list_index].sr_addr, &temp_par_addr);
    
  while(!linkaddr_cmp(&sink_address, &temp_par_addr)) {
    for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->node_addr, &temp_par_addr)) {
        linkaddr_copy(&temp_par_addr, &e->parent_addr);
        dest_list_index--;
        linkaddr_copy(&dest_to_sink_list[dest_list_index].sr_addr, &temp_par_addr);
        break;
      }
    }
  }
  num_node_dest_to_sink = LEN_OF_SR_LIST - dest_list_index;
  
  //src(source) to sink
  linkaddr_copy(&temp_par_addr, &req_spec->src_addr);
  src_list_index--;
  linkaddr_copy(&src_to_sink_list[src_list_index].sr_addr, &temp_par_addr);
    
  while(!linkaddr_cmp(&sink_address, &temp_par_addr)) {
    for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->node_addr, &temp_par_addr)) {
        linkaddr_copy(&temp_par_addr, &e->parent_addr);
        src_list_index--;
        linkaddr_copy(&src_to_sink_list[src_list_index].sr_addr, &temp_par_addr);
        break;
      }
    }
  }
  num_node_src_to_sink = LEN_OF_SR_LIST - src_list_index;
  
  int k;
  int find_parent = 0;
  for(i=0; i<num_node_dest_to_sink; i++) {
    for(j=0; j<num_node_src_to_sink; j++) {
      if(linkaddr_cmp(&dest_to_sink_list[LEN_OF_SR_LIST - 1 - i].sr_addr, &src_to_sink_list[LEN_OF_SR_LIST - 1 - j].sr_addr)) {
        num_data_node = i + j + 1;
        for(k=0; k<=i; k++) {
          data_path_list_index--;
          linkaddr_copy(&data_path_list[data_path_list_index].sr_addr, &dest_to_sink_list[LEN_OF_SR_LIST - 1 - k].sr_addr);
        }
        
        for(k=0; k<j; k++) {
          data_path_list_index--;
          linkaddr_copy(&data_path_list[data_path_list_index].sr_addr, &src_to_sink_list[LEN_OF_SR_LIST - j + k].sr_addr);
        }
        find_parent = 1;
        break;
      }
    }
    if(find_parent == 1) {
      break;
    }
  }
  
  LOG_INFO("CONTROLLER: num_data_node = %d \n", num_data_node);
  LOG_INFO("CONTROLLER: num_node_from_sink_to_dest = %d \n", num_node_from_sink_to_dest);
  //insert data path nodes from dest to src in config packet
  for(i=0; i<num_data_node; i++) {
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      config->payload[counter] = data_path_list[LEN_OF_SR_LIST - 1 - i].sr_addr.u8[j];
      counter++;
    }
  }
  
  config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] = num_node_from_sink_to_dest + num_data_node;
  
  //add num of cells per hop
  if(req_id->is_ctrl == 1) {
    for(i=1; i<num_node_from_sink_to_dest + num_data_node; i++) {
      if(i > num_node_from_sink_to_dest) {
        config->payload[counter] = req_spec->num_ts;
        sum_cell_hops = sum_cell_hops + req_spec->num_ts;
      } else {
        config->payload[counter] = 0;
      }
      counter++;
    }
  } else{
    if((sum_cell_hops = allocate_cell_per_hop(num_node_from_sink_to_dest, num_data_node, req_spec, config)) > 0) {
      printf("CONTROLLER: cell list alloc: sum %d \n", sum_cell_hops);
      counter = counter + num_node_from_sink_to_dest + num_data_node -1;
    } else{
      LOG_WARN("CONTROLLER: cell list alloc: sum = 0\n");
      packet_deallocate(config);
      dealloc_mem_sent_config(my_config);
      return NULL;
    }
  } 

  // add flow id to config
  for(j = 0; j< 2; ++j) {
    config->payload[counter] = req_spec->flow_id.u8[j];
    counter++;
  }
  
  //check if we exceed the SDN_MAX_PACKET_LEN:
  if((counter + (sum_cell_hops * 3) + 2) < SDN_MAX_PACKET_LEN) { 
    //add schedule
    int add_sch;
    if((add_sch = add_schedule_to_config(config, req_spec, req_id)) == 1) {
      LOG_INFO("CONTROLLER: add schedule is ok \n");
      counter = counter + (sum_cell_hops * 3) + 1; // 1 = sf_id
      counter++;   //pkt->typ
    
      if(counter > SDN_MAX_PACKET_LEN) {//counter is the total length of the packet in SDN layer befor adding 6 byte SDN-IEs
        LOG_ERR("CONTROLLER: fail to creat Config packet!, exceed MAX_SDN_PACKET-LEN \n");
      } else {
        //print_global_table();
        memcpy(&my_config->packet_info.packet, config, sizeof(struct sdn_packet));
        my_config->packet_info.len = counter;
        linkaddr_copy(&my_config->ack_sender, &req_spec->src_addr);
        packet_deallocate(config);
        if(req_id->is_ctrl == 0) {
          LOG_ERR("CONTROLLER: HOP_TO_DEST: %d, ADDR:[[%d%d]]\n", num_data_node-1, req_id->req_sender.u8[0], req_id->req_sender.u8[1]);
        }
        return my_config;
      }
    }
  }
  
  LOG_ERR("CONTROLLER: fail to creat schedule! -> config content: \n");
  for(i=0; i<counter-1; i++) {
    LOG_ERR("%d \n", config->payload[i]);
  }
  
  packet_deallocate(config);
  dealloc_mem_sent_config(my_config);
  return NULL;
}
/*---------------------------------------------------------------------------*/
int 
find_free_channels_for_ts(int slot, int rep_period, int *channel)
{ 
  if(channel == NULL) {
    return -1;
  }
  
  int i, m;
  int chs[SDN_MAX_CH_OFF_NUM];
  int best_ch;
  int ch_free = 1;
  int num_rep = (int)(SDN_DATA_SLOTFRAME_SIZE/rep_period);
  
  for(i=0; i<SDN_MAX_CH_OFF_NUM; i++) {
    chs[i] = -1;
  }

  for(m = 0; m < num_rep; m++) {
    for(i=0; i<SDN_MAX_CH_OFF_NUM; i++) {
      if(global_ts_list[i][(slot + (m*rep_period))] != -1) {
        chs[i] = i;
      }
    }
  } 
  
  
  for(i=0; i<1000; i++) {
    best_ch = (random_rand() % SDN_MAX_CH_OFF_NUM);
    if(chs[best_ch] == best_ch) {
      ch_free = 0;
    }
    if(ch_free) {
      *channel = best_ch;
      return *channel;
    }
  }  
  
  return -1;            
}
/*---------------------------------------------------------------------------*/
int 
add_schedule_to_config(struct sdn_packet *config, struct rsrc_spec_of_config *req_spec, struct request_id *req_id)
{
  if(config == NULL || req_spec == NULL || req_id == NULL) {
    return 0;
  }
  int i;
  int k;
  int last_ts;
  uint8_t j;
  int repetition_num;
  
  struct sdn_global_table_entry *e;
  struct sdn_global_table_entry *e1;
  struct sdn_global_table_entry *e2;

  
  int hop_list_index = (config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] * LINKADDR_SIZE) + CONF_LIST_OF_NODE_IN_PATH_INDEX;
  int schedule_index = hop_list_index + config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] -1 + 2; // 2 = fid len
  int counter = CONF_LIST_OF_NODE_IN_PATH_INDEX;
  LOG_INFO("CONTROLLER: hop_list_index %d \n", hop_list_index);
   
  linkaddr_t addr1;
  linkaddr_t addr2;
  
  
  if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller)) {
    last_ts = -1;
  } 
  else if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller)) {
    last_ts = SDN_CONTROL_SLOTFRAME_SIZE;
  }
  else{
    //last_ts = SDN_SF_REP_PERIOD;
    last_ts = req_spec->app_qos.traffic_period;
  }
  
  //SF ID 
  config->payload[schedule_index] = req_spec->sf_id;
  schedule_index++;
  
  for(j = 0; j< LINKADDR_SIZE; ++j) {
    addr2.u8[j] = config->payload[counter];
    counter++;
  }
  

  config->payload[CONF_REPETION_PERIOD] = req_spec->app_qos.traffic_period & 0xff;
  config->payload[CONF_REPETION_PERIOD + 1] = (req_spec->app_qos.traffic_period >>8) & 0xff;
  
  repetition_num = (int)(SDN_DATA_SLOTFRAME_SIZE/req_spec->app_qos.traffic_period);
    
  /* iterate over hops */
  for(i=0; i<config->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]-1; i++) {
    linkaddr_copy(&addr1, &addr2);
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      addr2.u8[j] = config->payload[counter];
      counter++;
    }
    
    for(e = list_head(sdn_global_table); e != NULL; e = e->next) {
      if(linkaddr_cmp(&e->node_addr, &addr1)) {
        e1 = e;
      }
      if(linkaddr_cmp(&e->node_addr, &addr2)) {
        e2 = e;
      }
    }
    
    if(e1 == NULL || e2 == NULL) {
      LOG_ERR("NUll hop address! \n");
      return 0;
    }
    
    /* iterate for cells of each hop */
    for(k=0; k<config->payload[hop_list_index + i]; k++) {           
      if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller) && e2->from_ctrl_slot > -1) {
        int m;
        int ch = e2->data_taken_slot[1][e2->from_ctrl_slot];
        config->payload[schedule_index] = (e2->from_ctrl_slot) & 0xff;
        schedule_index++;
        config->payload[schedule_index] = ((e2->from_ctrl_slot)>>8) & 0xff;
        schedule_index++;
        config->payload[schedule_index] = ch; //TODO
        schedule_index++;
        for(m = 0; m < repetition_num; m++) {
          add_slot_to_taken_cell_list(&addr1, req_id->is_ctrl, e2->from_ctrl_slot + (m*req_spec->app_qos.traffic_period), ch);
        }
      } else{
        int slot_busy = 1;
        int num_rep = 0;
        int ts_ctrl;
        
        
        if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller)) {
          ts_ctrl = e1->to_ctrl_slot;
        }
        if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller)) {
          linkaddr_t *grand_par = get_parent_from_node_addr(&addr2);
          struct sdn_global_table_entry *e_gp;
          for(e_gp = list_head(sdn_global_table); e_gp != NULL; e_gp = e_gp->next) {
            if(grand_par != NULL && linkaddr_cmp(&e_gp->node_addr, grand_par)) {
              break;
            }
          }
          if(e_gp != NULL) {
            ts_ctrl = e_gp->from_ctrl_slot;
          }
        }
        if(linkaddr_cmp(&req_spec->flow_id, &flow_id_best_effort)) {
          ts_ctrl = e1->best_effort_slot;
          if(!linkaddr_cmp(&e1->node_addr, &sink_addr) && ts_ctrl == -1) {
            LOG_ERR("check 2 \n");
            return 0;
          }
        }
        
        while(slot_busy == 1 && num_rep < 2) {
          int channel_off;
          if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller)) {
            last_ts++;
          } else{
            last_ts--;
            //LOG_INFO("CONTROLLER: last_ts value %d \n", last_ts);
          }
          
          if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller) && ts_ctrl > -1 && last_ts > ts_ctrl) {
            slot_busy = 1;
          }
          else if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller) && ts_ctrl > -1 && last_ts < ts_ctrl) {
            slot_busy = 1;
          } 
          else if(linkaddr_cmp(&req_spec->flow_id, &flow_id_best_effort) && ts_ctrl > -1 && last_ts > ts_ctrl) {
            slot_busy = 1;
            LOG_INFO("CONTROLLER: besteff taken slot %d \n", last_ts);
          }
          else{
            slot_busy = 0;
          }
          
          if(slot_busy == 0) {    
            int m;
            for(m = 0; m < repetition_num; m++) {               
              if(e1->data_taken_slot[0][(last_ts + (m*req_spec->app_qos.traffic_period))] > -1 ||
                 e2->data_taken_slot[0][(last_ts + (m*req_spec->app_qos.traffic_period))] > -1) {
                slot_busy = 1;
                //LOG_INFO("CONTROLLER: TO or FROM taken slot %d \n", last_ts);
                break;
              }
            }
          }
          if(slot_busy == 0) { 
            int ch_ok;
            if((ch_ok = find_free_channels_for_ts(last_ts, req_spec->app_qos.traffic_period, &channel_off)) == -1) {
              //LOG_INFO("CONTROLLER: cannot find a free ch_off %d \n", last_ts);
              slot_busy = 1;
            }
          }
          
          if(slot_busy == 0) {
            if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller)) {
              specify_to_ctrl_cell(&addr2, last_ts, req_spec->ch_off);
            }
            if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller)) {
              specify_from_ctrl_cell(&addr2, last_ts, req_spec->ch_off);
            }
            if(linkaddr_cmp(&req_spec->flow_id, &flow_id_best_effort)) {
              specify_best_effort_cell(&addr2, last_ts, req_spec->ch_off);
            }

            int m;
            config->payload[schedule_index] = last_ts & 0xff;
            schedule_index++;
            config->payload[schedule_index] = ((last_ts) >>8) & 0xff;
            schedule_index++;
            config->payload[schedule_index] = channel_off;
            schedule_index++;
            for(m = 0; m < repetition_num; m++) {
              int fali_to_add_cell;
              fali_to_add_cell = add_slot_to_taken_cell_list(&addr1, req_id->is_ctrl, last_ts + (m*req_spec->app_qos.traffic_period), channel_off);
              if(fali_to_add_cell == -1)
              {
                LOG_INFO("CONTROLLER: fail to allocate cell: ts %d, ch %d \n", last_ts + (m*req_spec->app_qos.traffic_period), channel_off);
              }
              fali_to_add_cell = add_slot_to_taken_cell_list(&addr2, req_id->is_ctrl, last_ts + (m*req_spec->app_qos.traffic_period), channel_off);
              if(fali_to_add_cell == -1)
              {
                LOG_INFO("CONTROLLER: fail to allocate cell: ts %d, ch %d \n", last_ts + (m*req_spec->app_qos.traffic_period), channel_off);
              }
              add_slot_to_global_ts_list(last_ts + (m*req_spec->app_qos.traffic_period), channel_off);

              //LOG_INFO("CONTROLLER: slot %d is free \n", last_ts + (m*req_spec->app_qos.traffic_period));
            }
          }  
        
          if(linkaddr_cmp(&req_spec->flow_id, &flow_id_from_controller)) {
            if(last_ts == req_spec->app_qos.traffic_period-1) {
              last_ts = -1;
              num_rep++;
            }
          } 
          else if(linkaddr_cmp(&req_spec->flow_id, &flow_id_to_controller)) {
            if(last_ts == 0) {
              last_ts = req_spec->app_qos.traffic_period;
              num_rep++;
            }
          }
          else{
            if((last_ts % SDN_SF_REP_PERIOD) == SDN_CONTROL_SLOTFRAME_SIZE && (int)(last_ts / SDN_SF_REP_PERIOD) == 0) {
              
              LOG_INFO("CONTROLLER: warning: not enough resource to allocate: last-ts %d \n", last_ts);
             // return -1;
              
              last_ts = SDN_SF_REP_PERIOD;
              num_rep++;
              LOG_INFO("CONTROLLER: warning: not enough resource to allocate: num_rep %d \n", num_rep);
              //num_rep++;
            } else if((last_ts % SDN_SF_REP_PERIOD) == SDN_CONTROL_SLOTFRAME_SIZE) {
              last_ts = last_ts - SDN_CONTROL_SLOTFRAME_SIZE;
            }
          }
        }
      }
    }  
  }
  return 1; 
}
/*---------------------------------------------------------------------------*/
struct sdn_packet_info * 
request_confing_packet(struct request_id *req_id, struct rsrc_spec_of_config *req_spec)
{
  struct sent_config_info *p = NULL;
  struct sent_config_info *dup_config = NULL;
  
  if(req_id == NULL || req_spec == NULL) {
    return NULL;
  }
  dup_config = look_for_req_id_in_sent_config_list(req_id);
  
  if(dup_config == NULL) {
    p = add_source_routing_info_to_config(req_id, req_spec);
    if(p != NULL) {
      set_timer_to_config_packet(p);
      memcpy(&p->req_id, req_id, sizeof(struct request_id));
      return &p->packet_info;
    }
  } else{
  
    LOG_INFO("CONTROLLER: request is duplicate! \n");
    
    if(!exist_active_timer_for_config(req_id)) {
      return &dup_config->packet_info;
    }
  } 
  
  return NULL;
}
/*---------------------------------------------------------------------------*/
struct sent_config_info *
look_for_req_id_in_sent_config_list(struct request_id *req_id)
{
  struct sent_config_info *e;
  if(req_id != NULL) {
    for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->req_id.req_sender, &req_id->req_sender) &&
       e->req_id.is_ctrl == req_id->is_ctrl && e->req_id.req_num == req_id->req_num) {
        LOG_INFO("CONTROLLER: dup request_id! -> addr:is_ctrl:req_num = %d%d:%d:%d \n",req_id->req_sender.u8[0], req_id->req_sender.u8[1],
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
  if(e != NULL) {
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
  if(p != NULL) {
    list_remove(sent_config_list, p);
    memb_free(&sent_config_list_mem, p);
  }
}
/*---------------------------------------------------------------------------*/
/* check if there exists a active resend-timer for config request,
   prevent to resend the config packet*/
int 
exist_active_timer_for_config(struct request_id *req_id)
{
  struct sent_config_info *e;
  struct config_timer *t = NULL;
  
  if(req_id != NULL) {
    for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->req_id.req_sender, &req_id->req_sender) &&
       e->req_id.is_ctrl == req_id->is_ctrl && e->req_id.req_num == req_id->req_num) {
        /* check there is a active timer */
        for(t = list_head(timer_list); t != NULL; t = t->next) { 
          if(linkaddr_cmp(&t->ack_sender, &e->ack_sender) && t->seq_num == e->seq_num) {
            LOG_INFO("CONTROLLER: there is a active timer for this req \n");
            return 1;
          }
        }
      }
    }
  }
  return 0;
} 
/*---------------------------------------------------------------------------*/
void
dealloc_mem_config_timer(struct config_timer* t)
{
  if(t != NULL) {
    list_remove(timer_list, t);
    memb_free(&timer_list_mem, t);
    LOG_INFO("CONTROLLER: remove timer \n");
  }
}
/*---------------------------------------------------------------------------*/
int 
set_timer_to_config_packet(struct sent_config_info* sent_config)
{ 
  int seq_num = -1;
  struct config_timer *t = NULL;
  struct sdn_global_table_entry *e;
  
  if(sent_config == NULL) {
    return -1;
  }
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, &sent_config->ack_sender)) {
      e->seq_num = e->seq_num + 1;
      seq_num = e->seq_num;
      LOG_INFO("CONTROLLER: set timer 1 \n");
    }
  }
  for(t = list_head(timer_list); t != NULL; t = t->next) { 
    if(linkaddr_cmp(&t->ack_sender, &sent_config->ack_sender) && t->seq_num == seq_num) {
      LOG_INFO("CONTROLLER: timer is already set \n");
    }
  }
  
  if(t == NULL) {
    t = memb_alloc(&timer_list_mem);
    if(t != NULL) {
      linkaddr_copy(&t->ack_sender, &sent_config->ack_sender);
      t->seq_num = seq_num;
      
      PROCESS_CONTEXT_BEGIN(&conf_handle_process);
      etimer_set(&t->timer, PERIOD_RESEND_CONFIG);
      PROCESS_CONTEXT_END(&conf_handle_process);
      
      list_add(timer_list, t);
      LOG_INFO("CONTROLLER: set timer 2 %d\n", seq_num);
      /* set sent_config entry */
      sent_config->packet_info.packet.payload[CONF_SEQ_NUM_INDEX] = seq_num;
      sent_config->seq_num = seq_num;
      sent_config->transmission = 1;
      sent_config->is_acked = 0;
    }  
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
void 
handle_config_ack_packet(struct sdn_packet* ack)
{
  linkaddr_t ack_sender;
  int seq_num;
  uint8_t j;
  struct sent_config_info *e;
  
  if(ack != NULL) {
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      ack_sender.u8[j] = ack->payload[ACK_SENDER_ADDR_INDEX + j];
    }
    seq_num = ack->payload[ACK_SEQ_NUM_INDEX];
      
    for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->ack_sender, &ack_sender) && e->seq_num == seq_num) {
        e->is_acked = 1;
      }
    }
    //print_sent_config_list();
  }
}
/*---------------------------------------------------------------------------*/
void
handle_config_timer(struct config_timer *t)
{
  struct sent_config_info *e = NULL;
  struct sent_config_info *e_conf = NULL;
  int need_remove_timer = 0;
  
  if(t != NULL) {
    for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
      if(linkaddr_cmp(&e->ack_sender, &t->ack_sender) && e->seq_num == t->seq_num) {
        e_conf = e;
        if(e->is_acked == 1 || e->transmission >= MAX_CONFIG_TRANSMISSION) {
          need_remove_timer = 1;
        }
        break;
      }
    }
  
    if(need_remove_timer) {
      dealloc_mem_config_timer(t);
    } 
    if(need_remove_timer == 0 && e_conf != NULL) {
      PROCESS_CONTEXT_BEGIN(&conf_handle_process);
      etimer_set(&t->timer, PERIOD_RESEND_CONFIG);
      PROCESS_CONTEXT_END(&conf_handle_process);
      
      e_conf->transmission = e_conf->transmission + 1;
      sdn_handle_config_packet(&e_conf->packet_info.packet, e_conf->packet_info.len, &linkaddr_null);
      LOG_INFO("CONTROLLER: resend config packet \n");
    }  
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(conf_handle_process, ev, data)
{ 
  static struct config_timer *t;
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(PROCESS_EVENT_TIMER);
    for(t = list_head(timer_list); t != NULL; t = t->next) { 
      if((struct etimer*)data == (&t->timer)) {
        LOG_INFO("CONTROLLER: timer is over \n");
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
  //int j = 0;
  for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
    shared_cell.cell_list[i] = -1;
  }
  
  /****** get list of shared cells in sf ******/
  int num_rep = (int)(SDN_DATA_SLOTFRAME_SIZE / SDN_SF_REP_PERIOD);
  //int num_sh_cell_in_rep = (int)ceil((float)((float)sdn_num_shared_cell / (float)num_rep));
  printf("(float)(sdn_num_shared_cell / num_rep): %f", (float)((float)sdn_num_shared_cell/(float)num_rep));
  printf("num_rep:%d sdn_num_shared_cell_in_rep: %d, sdn_num_shared_cell:%d \n", num_rep, sdn_num_shared_cell_in_rep, sdn_num_shared_cell);
#define SHARED_CELL_LIST_LEN_TMP  DIST_UNIFORM_LEN  
  const int list_shared_cell_len = (int)(sdn_num_shared_cell_in_rep * num_rep);
  static int list_of_shared_cell[SHARED_CELL_LIST_LEN_TMP];
  
  for(i=0; i<SHARED_CELL_LIST_LEN_TMP; i++) {
    list_of_shared_cell[i] = -1;
    //printf("list_of_shared_cell %d len %d\n", list_of_shared_cell[i], list_shared_cell_len);
  }
  
  
  static struct list_dist_uniform list_dist;
  list_dist.len = list_shared_cell_len;
  
  for(i=0; i<DIST_UNIFORM_LEN; i++) {
    list_dist.list[i] = 0;
  }
  sdn_distribute_list_uniform(&list_dist);
  
  //for(i=0; i<list_dist.len; i++) {
    //printf("...... %d", list_dist.list[i]);
  //}
  //printf("\n");
  int m;
  for(i=1; i<=sdn_num_shared_cell; i++) {  
    for(m=0; m<list_shared_cell_len; m++) {
      if(list_dist.list[m] == i) {
        list_of_shared_cell[i-1] = ((m / sdn_num_shared_cell_in_rep) * (SDN_SF_REP_PERIOD)) + 
                                   ((m % sdn_num_shared_cell_in_rep) * (int)(SDN_SF_REP_PERIOD / sdn_num_shared_cell_in_rep));   
        printf(" [%d, %d, %d] ", list_dist.list[m], m, list_of_shared_cell[i-1]);     
      }
    }   
  }
  printf("\n");
  
  
  int k = 0; 
  for(i=0; i<SHARED_CELL_LIST_LEN_TMP; i++) {
    if(list_of_shared_cell[i] != -1) {
      shared_cell.cell_list[k] = list_of_shared_cell[i];
      k++;
    } 
  }

/*  
  int k; 
  if(sdn_num_shared_cell <= SHARED_CELL_LIST_LEN) {
    for(i=0; i<sdn_num_shared_cell; i++) {
      for(k=0; k<sdn_num_shared_cell_in_rep; k++){
        shared_cell.cell_list[i] = j + (k * (SDN_SF_REP_PERIOD / sdn_num_shared_cell_in_rep));
        if(k<sdn_num_shared_cell_in_rep-1){
          i++;
        }
      }  
      j = j + SDN_SF_REP_PERIOD;
    }
  } else{
    for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
      shared_cell.cell_list[i] = j;
      j = j + SDN_SF_REP_PERIOD;
    }
    LOG_WARN("CONTROLLER: sdn_num_shared_cell exceeds shard cell array len\n");
  }
*/


  
  // print shared cells
/*
  LOG_INFO("CONTROLLER: list of shared cells:\n");
  for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
    if(shared_cell.cell_list[i] > -1) {
      LOG_INFO(" %d", shared_cell.cell_list[i]);
    }
  }
  LOG_INFO("\n");
*/  
  return &shared_cell;
}
/*---------------------------------------------------------------------------*/
struct request_id * 
get_request_id(struct sdn_packet* p)
{
  struct request_id *req_id = (struct request_id *) malloc(sizeof(struct request_id));
  linkaddr_t sender_addr;
  int j;
     
  for(j = 0; j< LINKADDR_SIZE; ++j) {
    sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
  }
     
     
  if((p->typ & 0x0f) == REPORT) {
    if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
      LOG_INFO("node is not registered\n");
      req_id->req_num = flow_id_to_controller.u8[0];
    }
    else if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) > 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) == 0) {
      LOG_INFO("node has to-ctrl, does not have from-ctrl \n");
      req_id->req_num = flow_id_from_controller.u8[0];
    }
    else if(((p->payload[R_IS_JOIN_INDEX] & 0x1c)>>2) == 0 && ((p->payload[R_IS_JOIN_INDEX] & 0xE0)>>5) > 0) {
      LOG_INFO("node has from-ctrl, does not have to-ctrl \n");
      req_id->req_num = flow_id_to_controller.u8[0];
    } 
    else if((p->payload[R_IS_JOIN_INDEX] & 0x03) == 0) {
      LOG_INFO("node is registered, with no best effort fid\n");
      req_id->req_num = flow_id_best_effort.u8[0];
    }
    else{
      req_id->req_num = 0;
    }
    
    req_id->is_ctrl = 1;
    linkaddr_copy(&req_id->req_sender, &sender_addr);
  }
  
  
  if((p->typ & 0x0f) == REQUEST) {
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
     
  for(j = 0; j< LINKADDR_SIZE; ++j) {
    sender_addr.u8[j] = p->payload[R_SENDER_ADDR_INDEX + j];
  }
  
  if((p->typ & 0x0f) == REPORT) {

    switch(req_id->req_num) {
      case 0:
        LOG_INFO("CONTROLLER: register node\n");
        break;

      case 2:
        LOG_INFO("CONTROLLER: not register node: case 2\n");          
        linkaddr_copy(&req_spec->flow_id, &flow_id_to_controller);
        if((is_true = specify_parent_from_report(p)) == -1) {
          LOG_ERR("CONTROLLER: cannot specify parent addr\n");
          linkaddr_copy(&req_spec->dest_addr, &linkaddr_null);
        } else {
          specify_shared_cell_offset_for_node(&sender_addr);
          linkaddr_copy(&req_spec->src_addr, &sender_addr);
          linkaddr_copy(&req_spec->dest_addr, get_parent_from_node_addr(&sender_addr));
          LOG_INFO("CONTROLLER: report addr:id %d:%d\n",sender_addr.u8[1], req_id->req_num);
        }
        
        break;

      case 3:
        LOG_INFO("CONTROLLER: has to_ctrl, has not from-ctrl: case 3\n");       
        linkaddr_copy(&req_spec->flow_id, &flow_id_from_controller);
        linkaddr_copy(&req_spec->src_addr, get_parent_from_node_addr(&sender_addr));
        linkaddr_copy(&req_spec->dest_addr, &sender_addr);
        LOG_INFO("CONTROLLER: report addr:id %d:%d\n",sender_addr.u8[1], req_id->req_num);
        break;

      case 4:
        LOG_INFO("CONTROLLER: register, no best effort: case 4\n");      
        linkaddr_copy(&req_spec->flow_id, &flow_id_best_effort);
        linkaddr_copy(&req_spec->src_addr, &sender_addr);
        linkaddr_copy(&req_spec->dest_addr, get_parent_from_node_addr(&sender_addr));
        LOG_INFO("CONTROLLER: report addr:id %d:%d\n",sender_addr.u8[1], req_id->req_num);
        break;
        
    }
    if(linkaddr_cmp(&req_spec->flow_id, &flow_id_best_effort)) {
      int find_period = 0;
      int x = (int)(SDN_DATA_SLOTFRAME_SIZE/BF_TS_NUM);
      while(!find_period){
        if((x % SDN_SF_REP_PERIOD) == 0){
          LOG_INFO("CONTROLLER: find BF period: %d \n", x);
          req_spec->app_qos.traffic_period = x;
          find_period = 1;
        } else {
          x--;
        }
      }
    } else{
      req_spec->app_qos.traffic_period = SDN_SF_REP_PERIOD;
    }
    req_spec->sf_id = 0;
    req_spec->num_ts = 1;
    req_spec->ch_off = 0;    
  }
  
  if((p->typ & 0x0f) == REQUEST) {
    req_spec->sf_id = 0;
    req_spec->num_ts = 1;
    req_spec->ch_off = 0;
    if((free_flow_id = get_free_flow_id()) == NULL) {
      LOG_INFO("CONTROLLER: there is no free flow id\n");
      free(req_spec);
      return NULL;
    }
    linkaddr_copy(&req_spec->flow_id, free_flow_id);
    linkaddr_copy(&req_spec->src_addr, &sender_addr);
    //get dest lladdr from IP addr
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      req_spec->dest_addr.u8[j] = p->payload[REQ_REMOTE_IPADDR_INDEX + 14 + j];
    }
    /* for now just support traffic periods that is devidible to SDN_DATA_SLOTFRAME_SIZE */
    req_spec->app_qos.traffic_period = p->payload[REQ_QOS_TRAFFIC_PERIOD_INDEX + 1];
    req_spec->app_qos.traffic_period = (req_spec->app_qos.traffic_period <<8) + p->payload[REQ_QOS_TRAFFIC_PERIOD_INDEX];
    
    if(req_spec->app_qos.traffic_period > SDN_DATA_SLOTFRAME_SIZE) {
      req_spec->app_qos.traffic_period = SDN_DATA_SLOTFRAME_SIZE;
    }
    

    req_spec->app_qos.packet_size = p->payload[REQ_QOS_PACKET_SIZE_INDEX + 1];
    req_spec->app_qos.packet_size = (req_spec->app_qos.packet_size <<8) + p->payload[REQ_QOS_PACKET_SIZE_INDEX];
    
    req_spec->app_qos.reliability = p->payload[REQ_QOS_RELIABILITY_INDEX + 1];
    req_spec->app_qos.reliability = (req_spec->app_qos.reliability <<8) + p->payload[REQ_QOS_RELIABILITY_INDEX];
    
    req_spec->app_qos.max_delay = p->payload[REQ_QOS_MAX_DELAY_INDEX + 1];
    req_spec->app_qos.max_delay = (req_spec->app_qos.max_delay <<8) + p->payload[REQ_QOS_MAX_DELAY_INDEX];
    
    req_spec->app_qos.burst_size = p->payload[REQ_QOS_BURST_SIZE_INDEX + 1];
    req_spec->app_qos.burst_size = (req_spec->app_qos.burst_size <<8) + p->payload[REQ_QOS_BURST_SIZE_INDEX];
    
    LOG_INFO("CONTROLLER: req_spec->app_qos.reliability %u\n", req_spec->app_qos.reliability);
  }
  
  return req_spec;
}
/*---------------------------------------------------------------------------*/
linkaddr_t * 
get_free_flow_id(void)
{
  if(last_flow_id.u8[0] < 255) {
    last_flow_id.u8[0] = last_flow_id.u8[0] + 1;
    return &last_flow_id;
  } else if(last_flow_id.u8[0] == 255 && last_flow_id.u8[1] < 255) {
    last_flow_id.u8[1] = last_flow_id.u8[1] + 1;
    return &last_flow_id;
  } else{
    LOG_ERR("CONTROLLER: there is no free flow id\n");
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
sdn_send_packet_to_controller(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id)
{
  int i;
  struct sdn_packet *p = create_sdn_packet();
  if(p != NULL) {
  memcpy((uint8_t*)p, buf, len);

/*  LOG_INFO("count %u\n", p->typ);
  for(i=0; i<len-1; i++) {
    LOG_INFO("count %u\n", p->payload[i]);
  }
 */ 
  struct sdn_packet_info *created_conf = NULL;
  struct rsrc_spec_of_config *req_spec = NULL;
  struct request_id *req_id = NULL;
  
  switch(p->typ & 0x0f) {
    case CONFIG_ACK:
      LOG_INFO("sdn-handle: receive config ack \n");
      handle_config_ack_packet(p);
      break;
  
    case REQUEST:
    
      LOG_INFO("CONTROLLER: receive request flow \n");
      
      req_id = get_request_id(p);        
      
      if(req_id != NULL) {
        req_spec = get_resource_spec_from_request(p, req_id);
        LOG_INFO("CONTROLLER: request id-> addr:is_ctrl:req_num = %d%d:%d:%d \n",req_id->req_sender.u8[0], req_id->req_sender.u8[1],
               req_id->is_ctrl, req_id->req_num);
      }
      if(req_spec != NULL) {
        LOG_INFO("CONTROLLER: req spec: dest addr: %d%d flow id: %d%d \n",req_spec->dest_addr.u8[0], req_spec->dest_addr.u8[1],
               req_spec->flow_id.u8[0], req_spec->flow_id.u8[1]);
               
        created_conf = request_confing_packet(req_id, req_spec);
        if(created_conf != NULL) {
          LOG_INFO("created conf-req len %u\n", created_conf->len);
          for (i=0; i<created_conf->len -1; i++) {
            LOG_INFO("created config-req %u\n", created_conf->packet.payload[i]);
          }
          sdn_handle_config_packet(&created_conf->packet, created_conf->len, &linkaddr_null);
          //print_global_table();
        }
      }
      
      if(req_id != NULL) { 
        free(req_id); 
      }
      if(req_spec != NULL) {
        free(req_spec);
      }
      
      break;
      
    case REPORT:
              { 
      struct sent_config_info *e;
      int nonack_config = 0;
      for(e = list_head(sent_config_list); e != NULL; e = e->next) { 
        if(e->is_acked == 0 && e->transmission < MAX_CONFIG_TRANSMISSION) {
          nonack_config++;
        }
      }
      if(nonack_config > MAX_NUM_PENDING_CONFIG) {
        break;
      }
       
      req_id = get_request_id(p);        
      
      if(req_id != NULL && req_id->req_num > 1) {
        req_spec = get_resource_spec_from_request(p, req_id);
        
        if(!linkaddr_cmp(&req_spec->dest_addr, &linkaddr_null)) {
          printf(" update report \n");
          update_node_nbr_list(p);
        }
        
        if(req_spec != NULL && !linkaddr_cmp(&req_spec->dest_addr, &linkaddr_null)) {
          created_conf = request_confing_packet(req_id, req_spec);
        } 
        if(created_conf != NULL) {
          LOG_INFO("created conf-best len %u\n", created_conf->len);
          for (i=0; i<created_conf->len -1; i++) {
            LOG_INFO("created config-best %u\n", created_conf->packet.payload[i]);
          }
          sdn_handle_config_packet(&created_conf->packet, created_conf->len, &linkaddr_null);
          //print_global_table();
        } 
         
      } else{
        update_node_nbr_list(p);
      }
      if(req_id != NULL) { 
        free(req_id); 
      }
      if(req_spec != NULL) {
        free(req_spec);
      }
      //print_global_nbr_table();
      break;
      }
           
    case CONFIG:
      sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
      break;
  }
 packet_deallocate(p);
 }
}
/*---------------------------------------------------------------------------*/
/* add shared cells to the list of cells */
void
add_shared_cells_to_global_ts_list(void)
{
  int i;
  struct list_of_shared_cell *shared_cell;
  
  if((shared_cell = get_shared_cell_list()) != NULL) {
    for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
      if(shared_cell->cell_list[i] > -1) {
        add_slot_to_global_ts_list(shared_cell->cell_list[i], 0);  // alloc ch_off = 0 to all shared cells
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
init_global_ts_list(void)
{
  int i, j;
  for(i=0; i<GLOBAL_CH_LIST_LEN; i++) {
    for(j=0; j<GLOBAL_TS_LIST_LEN; j++) {
      global_ts_list[i][j] = -1;
    }
  }
}

/*
void
init_num_shared_cells(void)
{
  float cell_ratio = (float)NETWORK_SIZE/(int)((TSCH_EB_PERIOD/10) / SDN_DATA_SLOTFRAME_SIZE) + ceil(NETWORK_SIZE/5);
//  //printf("cell ration diff: %f", cell_ratio - (int)cell_ratio);
  if(NETWORK_SIZE > 0) {
    if((cell_ratio - (int)cell_ratio) > 0.5) {
      cell_ratio = cell_ratio + 1;
      //printf("cell ration: %f", cell_ratio);
    }
  }
  
  if((cell_ratio - (int)cell_ratio) == 0) {
    cell_ratio = cell_ratio + 1;
  } else {
    cell_ratio = ceil(cell_ratio);
  }
//
  sdn_num_shared_cell = (int)cell_ratio;
  sdn_num_shared_cell_in_rep = sdn_num_shared_cell / ((SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD));
  
  int i;
  int j;
  for(j=0; j<max_sf_offset_num; j++) {
    for(i=0; i<SDN_DATA_SLOTFRAME_SIZE; i++) {
      shared_cell_alloc_list[j][i] = -1;
    }
  }
  
  struct list_of_shared_cell *shared_cell;
  
  if((shared_cell = get_shared_cell_list()) != NULL) {
    for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
      if(shared_cell->cell_list[i] > -1) {
        for(j=0; j<max_sf_offset_num; j++) {
          shared_cell_alloc_list[j][shared_cell->cell_list[i]] = 0;
          //printf("add shared cell: %d \n", shared_cell->cell_list[i]);
        } 
      }
    }
  } 
}
*/
/*---------------------------------------------------------------------------*/
/* this function calculate the number of needed shared cells given:
   network size
   topology density
   accepted collision rate
   load of traffic
*/
int
calculate_num_shared_cell(void)
{
  float p; // we accept p% collision in shared cellls
#if SDN_UNCONTROLLED_EB_SENDING
  p = 0.01;     
#else
  p = 0.03; 
#endif
  float r = 0.01;
  float p_lumbda = 1;
  float lumbda = 1 + r;        // load of traffic in shared cells
  int ncell;
  
  while(p_lumbda > p) {
    lumbda = lumbda - r;
    p_lumbda = 1 - (exp(-lumbda) * (lumbda + 1));    
  }
  
  ncell = (int)round(1 / lumbda);
  printf("ncell for lambda: %d, ncell: %f >> \n", lumbda, ncell);  
  
  int num_ring = (int)ceil(NETWORK_RADIUS/TX_RANGE);
  int sum_cell;
  int i;
  int r1 = 0;
  int r2 = 0;
  float pi = 3.142857;
  int nodei;
  //int last_full_ring;
  // we simply multiply last ring cells to reserve also some cells for ( config, request, ack-config)
  // times by 2 because : we have report + ( config, request, ack-config)
  int cell_coef;
#if SDN_SHARED_CONTROL_PLANE
  cell_coef = 2;     // to include report, config, and (request, ack-config)
#elif SDN_SHARED_FROM_CTRL_FLOW
  cell_coef = 1;     // to include only config and intial reports
#else
  cell_coef = 1;
#endif

#if SDN_UNCONTROLLED_EB_SENDING
  int non_eb_cells = 0;
#endif


  for(i=1; i<num_ring+1; i++) {
    if(i*TX_RANGE > NETWORK_RADIUS) {
      r1 = r2;
      r2 = NETWORK_RADIUS;
    } else {
      r1 = r2;
      r2 = i*TX_RANGE;
    }    
    nodei = (int)round(((pi*pow(r2,2) - pi*pow(r1,2)) / (pi*pow(NETWORK_RADIUS,2))) * NETWORK_SIZE);

#if !SDN_UNCONTROLLED_EB_SENDING    
    // to not include the sink node
    if(i == 1) {
      nodei = nodei - 1;
    }
#endif    
    
    if(i == num_ring) {
      nodei = nodei * cell_coef;
    }
#if SDN_SHARED_CONTROL_PLANE   
    sum_cell = sum_cell + i * (nodei * ncell);    
    printf("cell calculation: ring: %d, sumcell: %d, nodei: %d >> \n", i, sum_cell, nodei);
#elif SDN_UNCONTROLLED_EB_SENDING
    // remove multiplication of hops: eb works one hop
    sum_cell = sum_cell + (nodei * ncell);   // EB cells
       
    if(i == num_ring) {                           // non EB cells
      non_eb_cells = i * (nodei * ncell);    
      printf("non eb cells calculation: ring: %d, non_eb_cells: %d >> \n", i, non_eb_cells);
    }
    printf("cell calculation: ring: %d, sumcell: %d, nodei: %d >> \n", i, sum_cell, nodei);
#else  
    /* only consider the last ring: it should be enough, because the joining process is like a wave.
       In each priod of time, nodes of one ring are joining to the network
    */
    if(i == num_ring) {     
      sum_cell = sum_cell + i * (nodei * ncell);    
      printf("cell calculation: ring: %d, sumcell: %d, nodei: %d >> \n", i, sum_cell, nodei);
    }
#endif
  }

#if SDN_UNCONTROLLED_EB_SENDING
  sum_cell = (int)ceil((float)(sum_cell * SDN_DATA_SLOTFRAME_SIZE) / (float)((float)(TSCH_EB_PERIOD / CLOCK_SECOND) * 100));
  
  sum_cell = sum_cell + (int)ceil((float)(non_eb_cells * SDN_DATA_SLOTFRAME_SIZE) / (float)(SDN_REPORT_PERIOD * 100));
  printf("estimated shared sum_cell: [%d] \n", sum_cell);
#else  
  sum_cell = (int)ceil((float)(sum_cell * SDN_DATA_SLOTFRAME_SIZE) / (float)(SDN_REPORT_PERIOD * 100));
  printf("estimated shared sum_cell: [%d] \n", sum_cell);
#endif  
  return sum_cell;
}
/*---------------------------------------------------------------------------*/
/* specify number of shared cells given the network size */
/* TODO we can only test the case sf_len = sf_rep */
void
init_num_shared_cells(void)
{
#if !SDN_UNCONTROLLED_EB_SENDING
  float cell_ratio = (float)NETWORK_SIZE/(int)((TSCH_EB_PERIOD/10) / SDN_DATA_SLOTFRAME_SIZE);
#endif
  int extra_shared_cells = calculate_num_shared_cell();
  //printf("cell ration0: %f, eb/sf: %d\n", cell_ratio, (int)((TSCH_EB_PERIOD/10) / SDN_DATA_SLOTFRAME_SIZE));
  //printf("cell ration diff: %f", cell_ratio - (int)cell_ratio);
  /*if(NETWORK_SIZE > 0) {
    if((cell_ratio - (int)cell_ratio) > 0.5) {
      cell_ratio = cell_ratio + 1;
      //printf("cell ration1: %f\n", cell_ratio);
    }
  }
  */
  
  
  /*
  if((cell_ratio - (int)cell_ratio) == 0) {
    cell_ratio = cell_ratio + 1;
  } else {
    cell_ratio = ceil(cell_ratio);
  }
  */
  //printf("cell ration2: %f\n", cell_ratio);
#if SDN_UNCONTROLLED_EB_SENDING
  sdn_num_shared_cell = (int)(extra_shared_cells);
#else
  sdn_num_shared_cell = (int)(cell_ratio + extra_shared_cells);
#endif
  sdn_num_shared_cell_in_rep = (int)ceil(((float)sdn_num_shared_cell / (float)(SDN_DATA_SLOTFRAME_SIZE / SDN_SF_REP_PERIOD)));
                               //(int)ceil((float)((float)sdn_num_shared_cell / (float)num_rep))
  printf("sdn_num_shared_cell: %d, sdn_num_shared_cell_in_rep: %d\n", sdn_num_shared_cell, sdn_num_shared_cell_in_rep);
  
  int i;
  int j;
  for(j=0; j<max_sf_offset_num; j++) {
    for(i=0; i<SDN_DATA_SLOTFRAME_SIZE; i++) {
      shared_cell_alloc_list[j][i] = -1;
    }
  }
  
  struct list_of_shared_cell *shared_cell;
  
  if((shared_cell = get_shared_cell_list()) != NULL) {
    for(i=0; i<SHARED_CELL_LIST_LEN; i++) {
      if(shared_cell->cell_list[i] > -1) {
        for(j=0; j<max_sf_offset_num; j++) {
          shared_cell_alloc_list[j][shared_cell->cell_list[i]] = 0;
          //printf("add shared cell: %d \n", shared_cell->cell_list[i]);
        } 
      }
    }
  } 
}
/*---------------------------------------------------------------------------*/
int 
get_first_free_shared_timeslot(void)
{
  int i;
  int num_rep = (int)(SDN_DATA_SLOTFRAME_SIZE / SDN_SF_REP_PERIOD);

  const int list_shared_cell_len = (int)(sdn_num_shared_cell_in_rep * num_rep);
  
 
  static struct list_dist_uniform list_dist;
  list_dist.len = list_shared_cell_len;
  
  for(i=0; i<DIST_UNIFORM_LEN; i++) {
    list_dist.list[i] = 0;
  }
  sdn_distribute_list_uniform(&list_dist);
  
  //for(i=0; i<list_dist.len; i++) {
    //printf("...... %d", list_dist.list[i]);
  //}
  //printf("\n");
  int m;
  int j;
  int ts;
  for(i=1; i<=sdn_num_shared_cell; i++) {  
    for(m=0; m<list_shared_cell_len; m++) {
      if(list_dist.list[m] == i) {
        ts = ((m / sdn_num_shared_cell_in_rep) * (SDN_SF_REP_PERIOD)) + 
                                   ((m % sdn_num_shared_cell_in_rep) * (int)(SDN_SF_REP_PERIOD / sdn_num_shared_cell_in_rep)); 
        
        for(j=0; j<max_sf_offset_num; j++) {
          if(shared_cell_alloc_list[j][ts] == 0){            
            //printf("first free shared cell: %d \n", ts);
            return ts;
          }
        }      
      }
    }   
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/* allocate a cell to sink node for EB */
/*void
alloc_eb_shared_cell_to_sink(void)
{
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, &sink_addr)) {
      break;
    }
  }

  if(e != NULL) {
    int sf_of;
    int find_cell = 0;
    int i;
    int j;
    int period_of_shared_cell = SDN_SF_REP_PERIOD / sdn_num_shared_cell_in_rep;
    for(j=1; j<=(sdn_num_shared_cell * max_sf_offset_num); j++) {
      
      int k;
      int ith_shared_cell_in_sf;
      //printf("shared cell ratio %d \n", SHARED_CELL_RATIO);
      for(k=0; k<SHARED_CELL_RATIO; k++) {
        if(shared_cell_shuffle_list[k] == j) {
          if(sdn_num_shared_cell != 0) {
            sf_of = (int)(k / (int)(sdn_num_shared_cell));
          } else {
            printf("Controller: sdn_num_shared_cell is zero\n");
          }
          ith_shared_cell_in_sf = k % (sdn_num_shared_cell);
          printf("shared cell ratio1: k:%d, ts: %d sf: %d\n", k, ith_shared_cell_in_sf, sf_of);
          break;
        }
      }
      
      printf("shared cell ratio %d \n", period_of_shared_cell);
      
      printf("shared cell ratio %d, %d \n", sf_of, ith_shared_cell_in_sf);
      for(i=0; i<SDN_DATA_SLOTFRAME_SIZE; i++) {
        if((i % period_of_shared_cell == 0) && (i / period_of_shared_cell == ith_shared_cell_in_sf) && (shared_cell_alloc_list[sf_of][i] == 0)) {
          shared_cell_alloc_list[sf_of][i] = sink_addr.u8[1];  // as node id
          e->shared_cell_sf_offset = sf_of;
          e->shared_cell_ts_offset = i;
          find_cell = 1;
          printf("sf offset for node: %d, %d\n", e->shared_cell_sf_offset, e->shared_cell_ts_offset);
          break;
        }
      }
      if(find_cell) {
        break;
      }
      
    }  
  }
}*/
/*---------------------------------------------------------------------------*/
/* allocate a cell to sink node for EB */
void
alloc_eb_shared_cell_to_sink(void)
{
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, &sink_addr)) {
      break;
    }
  }

  if(e != NULL) {
    //int i;
    int ts = get_first_free_shared_timeslot();
    if(ts != -1) {
      shared_cell_alloc_list[0][ts] = sink_addr.u8[1];  // as node id
      e->shared_cell_sf_offset = 0;
      e->shared_cell_ts_offset = ts;
    }
    /*
    for(i=0; i<SDN_DATA_SLOTFRAME_SIZE; i++) {
      if(shared_cell_alloc_list[0][i] == 0) {
        shared_cell_alloc_list[0][i] = sink_addr.u8[1];  // as node id
        e->shared_cell_sf_offset = 0;
        e->shared_cell_ts_offset = i;
        //printf("sf offset for node: %d, %d\n", e->shared_cell_sf_offset, e->shared_cell_ts_offset);
        break;
      }
    }*/
  }
  
/*  
  int i;
  struct list_dist_uniform list;
  list.len = 33;
  for(i=0; i<DIST_UNIFORM_LEN; i++) {
    list.list[i] = 0;
  }
  sdn_distribute_list_uniform(&list);
  
  for(i=0; i<list.len; i++) {
    printf(" %d", list.list[i]);
  }
*/
}
/*---------------------------------------------------------------------------*/
int
sdn_get_sink_sf_eb_offset(void)
{
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, &sink_addr)) {    
      
      return e->shared_cell_sf_offset;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
sdn_get_sink_ts_eb_offset(void)
{
  struct sdn_global_table_entry *e;
  
  for(e = list_head(sdn_global_table); e != NULL; e = e->next) { 
    if(linkaddr_cmp(&e->node_addr, &sink_addr)) {
      
      return e->shared_cell_ts_offset;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/* initialize controller  */
void 
sdn_controller_init(void)
{
  init_num_shared_cells();

  linkaddr_copy(&sink_addr, &sink_address);
  
  process_start(&conf_handle_process, NULL);
  
  memb_init(&sdn_global_table_mem);
  list_init(sdn_global_table);
  
  memb_init(&timer_list_mem);
  list_init(timer_list);

  memb_init(&sent_config_list_mem);
  list_init(sent_config_list);

  memb_init(&global_nbr_list_mem);
  list_init(global_nbr_list);
  
  init_global_ts_list();
  
  add_shared_cells_to_global_ts_list();
  
  register_node_to_global_nbr_table(&sink_addr);
#if SINK
  sdn_add_node_to_global_table(&sink_addr, &sink_addr);
  print_global_table();
#endif

 alloc_eb_shared_cell_to_sink();
}
/*---------------------------------------------------------------------------*/


