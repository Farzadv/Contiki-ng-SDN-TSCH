


#ifndef _SDN_FLOW_H_
#define _SDN_FLOW_H_


/********************************/
struct flow_table_entry {
  struct flow_table_entry *next;
  int sf_handle;
  int slot_num;
  int ch_off;
  int priority;
  linkaddr_t flow_id;
};

/********************************/
void sdn_flow_init(void);

int sdn_add_flow_enrty(const linkaddr_t *flow_id, int sf_handle, int slot_num, int ch_off, int priority);

linkaddr_t *sdn_find_flow_id(const int timeslot, const int sf_handle);

linkaddr_t *sdn_get_addr_for_flow_id(const linkaddr_t *flow_id);

int sdn_is_flowid_exist_in_table(const linkaddr_t *flow_id);

int sdn_associate_choff_to_nbr(const linkaddr_t *nbr_addr, int ch_off);

int sdn_get_fromctrl_choff_from_nbr_addr(const linkaddr_t *nbr_addr);

int sdn_do_cells_exist_in_flowtable_for_flow_id(const linkaddr_t *flow_id);

int sdn_find_first_timeslot_in_slotframe(const linkaddr_t *flow_id);

void sdn_print_flow_table(void);

#endif
