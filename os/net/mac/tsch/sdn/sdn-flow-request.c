#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "net/mac/tsch/tsch.h"
#include <stdio.h>

/* app libs */
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"

#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-flow-request.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-inout.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_REQ_FLOW_LOG_LEVEL



#define FLOW_REQ_PACKET_SIZE (LINKADDR_SIZE+49)


PROCESS(flow_request_process, "flow request process");
static int flow_request_replied;
static struct flowreq_packet_info flow_request_packet;
static struct etimer flow_request_timer;
#define FLOW_REQ_PERIOD    (200 * CLOCK_SECOND)


uint8_t request_number = 0;


struct list_of_requests {
  int request_num;
  struct socket *socket;
  struct app_qos_attr *app_qos;
};
#define REQ_LIST_LENGTH  4
static struct list_of_requests req_list[REQ_LIST_LENGTH];

static linkaddr_t best_effort = {{ 0x04, 0x00 }};
/*---------------------------------------------------------------------------*/
struct app_flow_entry {
  struct app_flow_entry *next;
  linkaddr_t flow_id;
  struct socket *socket;
  struct app_qos_attr *app_qos;
};
#define MAX_APP_FLOW_ENTRY 4
LIST(sdn_app_flow_table);
MEMB(app_flow_table_mem, struct app_flow_entry, MAX_APP_FLOW_ENTRY);
/*---------------------------------------------------------------------------*/
/* create a SDN request packet */
struct sdn_packet *
sdn_create_request_packet(int request_type, struct socket *udp_socket, struct app_qos_attr *udp_app_qos)
{
  struct sdn_packet *request = NULL;
  if(udp_socket != NULL && udp_app_qos != NULL){
    request_number++;
    request = create_sdn_packet();
    request->typ = REQUEST;
    int k;
    for (k = 0; k < LINKADDR_SIZE; k++){
      request->payload[REQ_SENDER_ADDR_INDEX + k] = linkaddr_node_addr.u8[k];
    }
    request->payload[REQ_TYP_INDEX] = request_type;
    request->payload[REQ_NUM_OF_REQ] = request_number;
  
    // set remote IP addr
    for (k = 0; k < 16; k++){
      request->payload[REQ_REMOTE_IPADDR_INDEX + k] = udp_socket->ripaddr.u8[k];
    }
   
    // set remote port
    request->payload[REQ_REMOTE_PORT_INDEX] =  udp_socket->rport & 0xff;
    request->payload[REQ_REMOTE_PORT_INDEX + 1] = ((udp_socket->rport) >>8) & 0xff;
  
    // set local IP addr
    for (k = 0; k < 16; k++){
      request->payload[REQ_LOCAL_IPADDR_INDEX + k] = udp_socket->lipaddr.u8[k];
    }
  
    //set local port
    request->payload[REQ_LOCAL_PORT_INDEX] =  (udp_socket->lport) & 0xff;
    request->payload[REQ_LOCAL_PORT_INDEX + 1] = ((udp_socket->lport) >>8) & 0xff;
 
    // set QoS: traffic_period;
    request->payload[REQ_QOS_TRAFFIC_PERIOD_INDEX] = udp_app_qos->traffic_period & 0xff;
    request->payload[REQ_QOS_TRAFFIC_PERIOD_INDEX + 1] = ((udp_app_qos->traffic_period) >>8) & 0xff;
    // set QoS: reliability;
    request->payload[REQ_QOS_RELIABILITY_INDEX] = udp_app_qos->reliability & 0xff;
    request->payload[REQ_QOS_RELIABILITY_INDEX + 1] = ((udp_app_qos->reliability) >> 8) & 0xff;
    // set QoS: max_delay;
    request->payload[REQ_QOS_MAX_DELAY_INDEX] = udp_app_qos->max_delay & 0xff;
    request->payload[REQ_QOS_MAX_DELAY_INDEX + 1] = ((udp_app_qos->max_delay) >>8) & 0xff;
    // set QoS: packet_size;
    request->payload[REQ_QOS_PACKET_SIZE_INDEX] = udp_app_qos->packet_size & 0xff;
    request->payload[REQ_QOS_PACKET_SIZE_INDEX + 1] = ((udp_app_qos->packet_size) >>8) & 0xff;
    // set QoS: burst_size;
    request->payload[REQ_QOS_BURST_SIZE_INDEX] = udp_app_qos->burst_size & 0xff;
    request->payload[REQ_QOS_BURST_SIZE_INDEX + 1] = ((udp_app_qos->burst_size) >>8) & 0xff;
    
    // store request number 
    for(k=0; k<REQ_LIST_LENGTH; k++){
      if(req_list[k].socket == NULL){
        req_list[k].socket = udp_socket;
        req_list[k].app_qos = udp_app_qos;
        req_list[k].request_num = request_number;
        break;
      }
    }
 
  }
  return request;
}
/*---------------------------------------------------------------------------*/
int 
sdn_response_for_flow_id(int flow_request_number, const linkaddr_t *flow_id, const int rep_period)
{
  int i;
  for(i=0; i<REQ_LIST_LENGTH; i++){
    if(req_list[i].request_num == flow_request_number){
      sdn_add_app_flow_enrty(flow_id, req_list[i].socket, req_list[i].app_qos);
      sdn_app_flow_id_response(req_list[i].socket, flow_id, rep_period);
      flow_request_replied = 1;
      
      LOG_INFO("SDN-FLOW-REQUEST: socket -> ipaddr\n");
      LOG_INFO_6ADDR(&req_list[i].socket->ripaddr);
      LOG_INFO_("\n");
      LOG_INFO("SDN-FLOW-REQUEST: socket -> rport %u\n", req_list[i].socket->rport);
      LOG_INFO("SDN-FLOW-REQUEST: socket -> lport %u\n", req_list[i].socket->lport);
      
      return 1;
      break;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* add a flow id to app flow table; this flow id correspods to a socket and QoS reqirements */
int 
sdn_add_app_flow_enrty(const linkaddr_t *flow_id, struct socket *udp_socket, struct app_qos_attr *udp_app_qos)
{
  struct app_flow_entry *e;
 
  for(e = list_head(sdn_app_flow_table); e != NULL; e = e->next){ 
    if(udp_socket->rport == e->socket->rport && udp_socket->lport == e->socket->lport &&
       uip_ipaddr_cmp(&udp_socket->ripaddr, &e->socket->ripaddr)){       
      LOG_INFO("SDN-FLOW-REQUEST: there is a flow-id for this socket in table\n");
      return 0;
    }
  }
  if(e == NULL){
    e = memb_alloc(&app_flow_table_mem);
    if(e != NULL){
      linkaddr_copy(&e->flow_id, flow_id);
      e->socket = udp_socket;
      e->app_qos = udp_app_qos;
      list_add(sdn_app_flow_table, e);
      LOG_INFO("SDN-FLOW-REQUEST: add a new app flow id, addr = [%d%d], RECEIVE_ASN=[0x%lx]]]]] \n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], tsch_current_asn.ls4b);
      LOG_INFO_LLADDR(&e->flow_id);
      LOG_INFO_("\n");
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* check if there is flow id for a socket and QoS info; if no send a request to the controller */
linkaddr_t *
sdn_app_find_flow_id(struct socket *udp_socket, struct app_qos_attr *udp_app_qos)
{
  struct app_flow_entry *e;
  linkaddr_t *flow_id = NULL;
 
  for(e = list_head(sdn_app_flow_table); e != NULL; e = e->next){ 
    if(udp_socket->rport == e->socket->rport && udp_socket->lport == e->socket->lport &&
       uip_ipaddr_cmp(&udp_socket->ripaddr, &e->socket->ripaddr)){       
      if(linkaddr_cmp(&e->flow_id, &linkaddr_null)){
        flow_id = &e->flow_id;
      }
    }
  }
  if(flow_id == NULL){
    if(udp_app_qos != NULL && udp_app_qos->traffic_period == 0 && udp_app_qos->reliability == 0 &&
       udp_app_qos->max_delay == 0 && udp_app_qos->packet_size == 0 && udp_app_qos->burst_size == 0){
      flow_id = &best_effort;
      sdn_add_app_flow_enrty(&best_effort, udp_socket, udp_app_qos);
    } else {
#if SINK
      flow_id = &best_effort; //just for support sink ...
#else
      struct sdn_packet *request = sdn_create_request_packet(FLOW_ID_REQ, udp_socket, udp_app_qos);
      if (request != NULL){
        int i;
        
        flow_request_replied = 0;
        process_start(&flow_request_process, NULL);
        
        // set timer for flow request
        PROCESS_CONTEXT_BEGIN(&flow_request_process);
        etimer_set(&flow_request_timer, FLOW_REQ_PERIOD);        
        PROCESS_CONTEXT_END(&flow_request_process);
        
        memcpy(&flow_request_packet.packet, request, FLOW_REQ_PACKET_SIZE);
        flow_request_packet.len = FLOW_REQ_PACKET_SIZE;
        
        for(i=0; i<50; i++){
          LOG_INFO("request payload %u\n", flow_request_packet.packet.payload[i]);
        }
        
        packetbuf_clear();
        packetbuf_copyfrom((uint8_t *)request, FLOW_REQ_PACKET_SIZE);
        if(sdn_is_joined){
          sdn_output(&linkaddr_null, &flow_id_to_controller, 1, NULL);
        }
      }
      packet_deallocate(request);
      LOG_INFO("SDN-FLOW-REQUEST: request a critical fid = [%d%d] from controller, SEND_ASN=[0x%lx]]]]]\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], tsch_current_asn.ls4b);
      //flow_id = &best_effort; // temp...
#endif
    }
  } 
  return flow_id;
}
/*---------------------------------------------------------------------------*/
int 
sdn_is_my_app_flow_id(const linkaddr_t *flow_id)
{
  struct app_flow_entry *e;
  
  for(e = list_head(sdn_app_flow_table); e != NULL; e = e->next){ 
    if(linkaddr_cmp(&e->flow_id, flow_id)){       
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void 
init_app_request_list_array(void)
{
  int i;
  for(i=0; i<REQ_LIST_LENGTH; i++){
    req_list[i].socket = NULL;
    req_list[i].app_qos = NULL;
    req_list[i].request_num = 0;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flow_request_process, ev, data)
{ 
  PROCESS_BEGIN();
  while(!flow_request_replied) {
    PROCESS_WAIT_EVENT_UNTIL(PROCESS_EVENT_TIMER);
    if(!flow_request_replied) {
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)&flow_request_packet.packet, FLOW_REQ_PACKET_SIZE);
      if(sdn_is_joined){
        sdn_output(&linkaddr_null, &flow_id_to_controller, 1, NULL);
        LOG_INFO("resend flow request to controller\n");
      }
      etimer_reset(&flow_request_timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void 
sdn_app_flow_init(void)
{    
  memb_init(&app_flow_table_mem);
  list_init(sdn_app_flow_table);
  init_app_request_list_array();
  
}
/*---------------------------------------------------------------------------*/













