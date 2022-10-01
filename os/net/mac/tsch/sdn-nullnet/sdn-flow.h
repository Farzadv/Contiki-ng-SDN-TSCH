


#ifndef _SDN_FLOW_H_
#define _SDN_FLOW_H_

void sdn_flow_init(void);

int sdn_add_flow(const linkaddr_t *flow_id, const linkaddr_t *addr, int is_tx, int is_rx);

int sdn_add_flow_enrty(const linkaddr_t *flow_id, int sf_handle, int slot_num, int ch_off, int priority);

int sdn_add_rule(const linkaddr_t *dst_addr, const linkaddr_t *flow_id);

linkaddr_t *sdn_find_flow_rule(const linkaddr_t *addr);

int sdn_find_tx_flow_id(const linkaddr_t *flow_id);

linkaddr_t *sdn_find_flow_id(const int timeslot, const int sf_handle);

linkaddr_t *sdn_get_addr_for_flow_id(const linkaddr_t *flow_id);

int sdn_is_flowid_exist_in_table(const linkaddr_t *flow_id);
#endif