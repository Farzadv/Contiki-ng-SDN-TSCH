


#ifndef _SDN_HANDLE_H_
#define _SDN_HANDLE_H_


void sdn_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id);
void sdn_data_handle(const linkaddr_t *flow_id);

void sdn_handle_config_packet(struct sdn_packet *p, uint16_t len, const linkaddr_t *src_addr);
int send_config_ack(int seq_num);

void sdn_handle_config_control_plane(struct sdn_packet*, uint16_t len);
void sdn_handle_config_data_plane_path(struct sdn_packet*, uint16_t len);
void sdn_handle_config_data_plane_single_node(struct sdn_packet*, uint16_t len);

#endif
