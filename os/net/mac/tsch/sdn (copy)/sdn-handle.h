


#ifndef _SDN_HANDLE_H_
#define _SDN_HANDLE_H_


void sdn_control_packet_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id);
void sdn_data_packet_handle(const linkaddr_t *flow_id);

void sdn_handle_config_packet(struct sdn_packet *p, uint16_t len, const linkaddr_t *src_addr);
int send_config_ack(int seq_num);

void sdn_input_packet_statistic(const linkaddr_t *flow_id);

#endif
