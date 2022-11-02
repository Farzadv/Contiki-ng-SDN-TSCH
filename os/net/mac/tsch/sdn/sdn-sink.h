

#ifndef _SDN_SINK_H_
#define _SDN_SINK_H_

#include "net/ipv6/simple-udp.h"
/*---------------------------------------------------------------------------*/
struct sdn_packet_info {
  struct sdn_packet packet;
  uint8_t len;
};

struct cell_info{
  uint8_t slot;
  uint8_t ch_off;
  uint8_t sf_id;
};

#define SHARED_CELL_LIST_LEN      300
struct list_of_shared_cell{
  int cell_list[SHARED_CELL_LIST_LEN];
  uint8_t ch_off;
};

struct config_timer{
  struct config_timer *next;
  struct etimer timer;
  linkaddr_t ack_sender;
  int seq_num;
};

struct request_id{
  linkaddr_t req_sender;
  int is_ctrl;
  int req_num;
};

struct rsrc_spec_of_config{
  struct app_qos_attr app_qos;
  linkaddr_t src_addr;
  linkaddr_t dest_addr;
  linkaddr_t flow_id;
  int sf_id;
  int num_ts;
  int ch_off;
};


/*------------------------*/
/* NBR structs definition */

struct nbr{
  linkaddr_t nbr_addr;
  float eb;
  int rssi;
  uint32_t total_eb_num;
  uint16_t total_report_count;  
};

#define NBR_LIST_LEN 50

struct nbrs_info{
  struct nbrs_info *next;
  linkaddr_t node_addr;
  struct nbr nbr_list[NBR_LIST_LEN];
};


struct flow_hops_cells{
  linkaddr_t node_addr;
  float pdr_link;
  float pdr_n_tx;
  int cell_num;
};
/*---------------------------------------------------------------------------*/
void sdn_send_packet_to_controller(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id);


void sdn_controller_init(void);

struct sdn_packet_info * request_confing_packet(struct request_id *req_id, struct rsrc_spec_of_config *req_spec);

int specify_parent_from_report(struct sdn_packet* p);

int sdn_add_node_to_global_table(const linkaddr_t *node_addr, const linkaddr_t *parent_addr);

struct list_of_shared_cell * get_shared_cell_list(void);

int add_slot_to_taken_cell_list(const linkaddr_t *node_addr, int is_ctrl_cell, int slot_offset, int ch_offset);

int specify_to_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);
int specify_from_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);
int specify_best_effort_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);

linkaddr_t * get_parent_from_node_addr(const linkaddr_t *node_addr);

void print_global_table(void);
void print_sent_config_list(void);
void print_global_nbr_table(void);

void handle_config_ack_packet(struct sdn_packet* ack);

void handle_config_timer(struct config_timer *timer);

void dealloc_mem_config_timer(struct config_timer* t);

struct request_id * get_request_id(struct sdn_packet* p);

struct sent_config_info * look_for_req_id_in_sent_config_list(struct request_id *req_id);

struct rsrc_spec_of_config * get_resource_spec_from_request(struct sdn_packet* p, struct request_id *req_id);

linkaddr_t * get_free_flow_id(void);

int register_node_to_global_nbr_table(const linkaddr_t *node_addr);

void update_node_nbr_list(struct sdn_packet* p);

//int round_float_up(float n);

int allocate_cell_per_hop(int sink_to_dest_num, int dest_to_src_num, struct rsrc_spec_of_config *req_spec, struct sdn_packet *config);

int alloc_choff_to_novel_node(struct sdn_packet* p);

void print_global_table(void);

int sdn_get_sink_sf_eb_offset(void);

int sdn_get_sink_ts_eb_offset(void);

#endif











