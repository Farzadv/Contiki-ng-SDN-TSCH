#include "contiki.h"
//#include "net/routing/routing.h"
#include "random.h"
//#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_CLIENT_APP_LOG_LEVEL

/* App QoS requirements: */
#define TRAFFIC_PERIOD1  (APP1_INT * 100) //ts
#define TRAFFIC_PERIOD2  (APP2_INT * 100) //ts
#define MAX_DELAY       0
#define PACKET_SIZE     0
#define RELIABILITY     99
#define BURST_SIZE      0

#define WITH_SERVER_REPLY      1
#define UDP_APP1_PORT	        1020
#define UDP_APP2_PORT	        1030
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL1		((APP1_INT * 2) * CLOCK_SECOND)
#define SEND_INTERVAL2		((APP2_INT * 2) * CLOCK_SECOND)

//static linkaddr_t dest_addr = {{ 0x00, 0x01 }};


static struct simple_udp_connection udp_conn1;
static struct simple_udp_connection udp_conn2;
static struct app_qos_attr app_qos;
int j = 0;
int k = 0;
static uint16_t count_app1 = 1;
static uint16_t count_app2 = 300;
/*---------------------------------------------------------------------------*/
PROCESS(udp_app1_process, "UDP client");
PROCESS(udp_app2_process, "UDP client");
PROCESS(seqnum_reset_process, "seqnum reset process");
AUTOSTART_PROCESSES(&udp_app1_process, &seqnum_reset_process, &udp_app2_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  //LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  //LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_app1_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer initial_timer;
  static uint8_t msg[2*LINKADDR_SIZE+6];
  //static char str[32];
  uip_ipaddr_t server_ipaddr;
  uip_ip6addr(&server_ipaddr,  0xfe80, 0, 0, 0, 0, 0xff, 0xfe00, 0x1);

  
  PROCESS_BEGIN();
  
#if SDN_ENABLE
  app_qos_request(&udp_conn1, &app_qos, TRAFFIC_PERIOD1, RELIABILITY, MAX_DELAY, PACKET_SIZE, BURST_SIZE);
#endif
  /* Initialize UDP connection */
  //udp_conn1.qos_attr = app_qos;
  
  etimer_set(&initial_timer, CLOCK_SECOND);
  while(num_from_controller_rx_slots == 0){   
    PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
    etimer_reset(&initial_timer);
  }
  //LOG_INFO("num from cells %d\n", num_from_controller_rx_slots);
  
  
  etimer_set(&periodic_timer, 240*CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
  
  simple_udp_register(&udp_conn1, UDP_APP1_PORT, &server_ipaddr,
                      UDP_SERVER_PORT, udp_rx_callback);
   
   
                      
  etimer_set(&periodic_timer, CLOCK_SECOND);
  while(linkaddr_cmp(&udp_conn1.udp_conn->flow_id, &linkaddr_null)){
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }
  
  
  
  if(linkaddr_cmp(&udp_conn1.udp_conn->flow_id, &flow_id_best_effort)){
    int fid_exsit;
    etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
    while((fid_exsit = sdn_is_flowid_exist_in_table(&flow_id_best_effort)) == 0){
      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }
      
 
  //etimer_set(&periodic_timer, 2400*CLOCK_SECOND);
  //PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));    
                  
  etimer_set(&periodic_timer, SEND_INTERVAL1);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    //if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      LOG_INFO("Sending data %u to sink with asn 0x%lx ", count_app1, tsch_current_asn.ls4b);
      //LOG_INFO_6ADDR(&server_ipaddr);
      LOG_INFO_("\n");
      //snprintf(str, sizeof(str), "hello %d", count_app1);
      //simple_udp_sendto(&udp_conn1, str, strlen(str), &server_ipaddr);
      for(j = 0; j< LINKADDR_SIZE; ++j){
        msg[j] = linkaddr_node_addr.u8[j];
      }
      msg[2] =  server_ipaddr.u8[14];
      msg[3] =  server_ipaddr.u8[15];
      
      msg[4] =  count_app1 & 0xFF;
      msg[5] = (count_app1 >> 8)& 0xFF;
      msg[6] =  tsch_current_asn.ls4b & 0xFF;
      msg[7] = (tsch_current_asn.ls4b >> 8)& 0xFF;
      msg[8] = (tsch_current_asn.ls4b >> 16)& 0xFF;
      msg[9] = (tsch_current_asn.ls4b >> 24)& 0xFF;
      
      simple_udp_sendto(&udp_conn1, msg, sizeof(msg), &server_ipaddr);
      
      count_app1++;
    //} else {
      //LOG_INFO("Not reachable yet\n");
    //}

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL1);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_app2_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer initial_timer;
  static uint8_t msg[2*LINKADDR_SIZE+6];
  //static char str[32];
  uip_ipaddr_t server_ipaddr;
  uip_ip6addr(&server_ipaddr,  0xfe80, 0, 0, 0, 0, 0xff, 0xfe00, 0x1);

  
  PROCESS_BEGIN();
  
#if SDN_ENABLE
  app_qos_request(&udp_conn2, &app_qos, TRAFFIC_PERIOD2, RELIABILITY, MAX_DELAY, PACKET_SIZE, BURST_SIZE);
#endif
  /* Initialize UDP connection */
  //udp_conn2.qos_attr = app_qos;
  
  etimer_set(&initial_timer, CLOCK_SECOND);
  while(num_from_controller_rx_slots == 0){   
    PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
    etimer_reset(&initial_timer);
  }
  //LOG_INFO("num from cells %d\n", num_from_controller_rx_slots);
  
  
  etimer_set(&periodic_timer, 10*CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
  
  simple_udp_register(&udp_conn2, UDP_APP2_PORT, &server_ipaddr,
                      UDP_SERVER_PORT, udp_rx_callback);
   
   
                      
  etimer_set(&periodic_timer, CLOCK_SECOND);
  while(linkaddr_cmp(&udp_conn2.udp_conn->flow_id, &linkaddr_null)){
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }
  
  
  
  if(linkaddr_cmp(&udp_conn2.udp_conn->flow_id, &flow_id_best_effort)){
    int fid_exsit;
    etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
    while((fid_exsit = sdn_is_flowid_exist_in_table(&flow_id_best_effort)) == 0){
      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }
      
 
  //etimer_set(&periodic_timer, 2400*CLOCK_SECOND);
  //PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));    
                  
  etimer_set(&periodic_timer, SEND_INTERVAL2);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    //if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      if(count_app1 > 300) {
      LOG_INFO("Sending data %u to sink with asn 0x%lx ", count_app2, tsch_current_asn.ls4b);
      //LOG_INFO_6ADDR(&server_ipaddr);
      LOG_INFO_("\n");
      //snprintf(str, sizeof(str), "hello %d", count_app2);
      //simple_udp_sendto(&udp_conn2, str, strlen(str), &server_ipaddr);
      for(k = 0; k< LINKADDR_SIZE; ++k){
        msg[k] = linkaddr_node_addr.u8[k];
      }
      msg[2] =  server_ipaddr.u8[14];
      msg[3] =  server_ipaddr.u8[15];
      
      msg[4] =  count_app2 & 0xFF;
      msg[5] = (count_app2 >> 8)& 0xFF;
      msg[6] =  tsch_current_asn.ls4b & 0xFF;
      msg[7] = (tsch_current_asn.ls4b >> 8)& 0xFF;
      msg[8] = (tsch_current_asn.ls4b >> 16)& 0xFF;
      msg[9] = (tsch_current_asn.ls4b >> 24)& 0xFF;
      
      simple_udp_sendto(&udp_conn2, msg, sizeof(msg), &server_ipaddr);
      
      count_app2++;
    //} else {
      //LOG_INFO("Not reachable yet\n");
    //}
    }
    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL2);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(seqnum_reset_process, ev, data)
{ 
  static int find_asn = 0;
  static struct etimer timer;
  
  PROCESS_BEGIN();
  // 0x668a0
  // 0x3A980 40 min
  // 57E40   60 min
  etimer_set(&timer, CLOCK_SECOND/100);
  while(!find_asn) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    if(tsch_current_asn.ls4b > 0xa7E40) {
        tsch_schedule_print();
        count_app1 = 1;
        find_asn = 1;
    }
    etimer_reset(&timer);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


