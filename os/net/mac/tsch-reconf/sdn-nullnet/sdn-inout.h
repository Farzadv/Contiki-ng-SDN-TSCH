


#ifndef _SDN_INOUT_H_
#define _SDN_INOUT_H_



void sdn_init(void);
int sdn_output(const linkaddr_t *dest_addr, const linkaddr_t *flow_id, int is_sdn_content);
void sdn_input(void);
void sdn_relay_send(const linkaddr_t *flow_id);


#endif