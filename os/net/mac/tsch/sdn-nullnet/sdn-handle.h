


#ifndef _SDN_HANDLE_H_
#define _SDN_HANDLE_H_


void sdn_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id);
void config_handle(struct sdn_packet*, uint16_t len);

#endif