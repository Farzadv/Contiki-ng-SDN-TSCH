


#ifndef _SDN_INOUT_H_
#define _SDN_INOUT_H_



void sdn_init(void);
int sdn_output(const linkaddr_t *dest_addr, const linkaddr_t *flow_id, int is_sdn_content, mac_callback_t sent);
void sdn_input(int is_duplicate);


#endif
