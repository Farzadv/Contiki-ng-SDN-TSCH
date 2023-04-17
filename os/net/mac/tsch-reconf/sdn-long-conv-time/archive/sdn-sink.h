

#ifndef _SDN_SINK_H_
#define _SDN_SINK_H_

struct sdn_packet_info {
  struct sdn_packet packet;
  uint8_t len;
};

struct cell_info{
  uint8_t slot;
  uint8_t ch_off;
  uint8_t sf_id;
};

#define SHARED_CELL_LIST_LEN 5
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



void sdn_sink_init(void);
int is_registered(struct sdn_packet* p);
struct sdn_packet * create_confing(void);
struct sdn_packet * create_confing2(void);
struct sdn_packet * create_confing1(void);
struct sdn_packet * create_confing3(void);
struct sdn_packet * create_confing4(void);
struct sdn_packet * create_confing5(void);


struct sdn_packet * create_data_confing_node3(int req_num);

struct sdn_packet * create_data_confing_node_new(int req_num);

struct sdn_packet_info * sdn_create_confing_packet(struct sdn_packet* report, const linkaddr_t *flow_id);

int specify_parent_from_report(struct sdn_packet* p);

int sdn_add_node_to_global_table(const linkaddr_t *node_addr, const linkaddr_t *parent_addr);
                                                      
struct cell_info * get_free_slot_control_plane(const linkaddr_t *flow_id, const linkaddr_t *sender_addr, const linkaddr_t *parent_addr);

struct list_of_shared_cell * get_shared_cell_list(void);

int add_slot_to_taken_cell_list(const linkaddr_t *node_addr, int slot_offset, int ch_offset);

int specify_to_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);
int specify_from_ctrl_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);
int specify_best_effort_cell(const linkaddr_t *node_addr, int slot_offset, int ch_offset);

linkaddr_t * get_parent_from_node_addr(const linkaddr_t *node_addr);

void print_global_table(void);
void print_sent_config_list(void);

void handle_config_ack_packet(struct sdn_packet* ack);

void handle_config_timer(struct config_timer *timer);

void dealloc_mem_config_timer(struct config_timer* t);
#endif











