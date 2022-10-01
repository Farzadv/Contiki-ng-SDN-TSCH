


#ifndef _SDN_FLOW_REQUEST_H_
#define _SDN_FLOW_REQUEST_H_

/* app libs */
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"


struct flowreq_packet_info {
  struct sdn_packet packet;
  uint8_t len;
};

void sdn_app_flow_init(void);
void init_app_request_list_array(void);

linkaddr_t *sdn_app_find_flow_id(struct socket *udp_socket, struct app_qos_attr *udp_app_qos);
int sdn_add_app_flow_enrty(const linkaddr_t *flow_id, struct socket *udp_socket, struct app_qos_attr *udp_app_qos);
int sdn_is_my_app_flow_id(const linkaddr_t *flow_id);
struct sdn_packet *sdn_create_request_packet(int request_type, struct socket *udp_socket, struct app_qos_attr *udp_app_qos);
int sdn_response_for_flow_id(int flow_request_number, const linkaddr_t *flow_id, const int rep_period);
#endif
