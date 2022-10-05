#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include <math.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  0
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (30 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
int j = 0;
static uint16_t count = 1;
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
PROCESS(seqnum_reset_process, "seqnum reset process");
AUTOSTART_PROCESSES(&udp_client_process, &seqnum_reset_process);
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

  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint8_t msg[2*LINKADDR_SIZE+6];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      
      for(j = 0; j< LINKADDR_SIZE; ++j){
        msg[j] = linkaddr_node_addr.u8[j];
      }
      
      msg[2] =  dest_ipaddr.u8[14];
      msg[3] =  dest_ipaddr.u8[15];
      
      msg[4] =  count & 0xFF;
      msg[5] = (count >> 8)& 0xFF;
      msg[6] =  tsch_current_asn.ls4b & 0xFF;
      msg[7] = (tsch_current_asn.ls4b >> 8)& 0xFF;
      msg[8] = (tsch_current_asn.ls4b >> 16)& 0xFF;
      msg[9] = (tsch_current_asn.ls4b >> 24)& 0xFF;
      
      simple_udp_sendto(&udp_conn, msg, sizeof(msg), &dest_ipaddr);
      
      count++;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* exponential traffic generator */
    int r = random_rand() % 100;
    if(r == 0){
      r = 1;
    }
    float rng = (float)r / 100;
    int next_time = (int)(-log(rng) * APP_INT);
    //printf("next time traffic: %d s , rng %f r %d \n", next_time, rng, r);
    etimer_set(&periodic_timer, (next_time * CLOCK_SECOND));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(seqnum_reset_process, ev, data)
{ 
  static int find_asn = 0;
  static struct etimer timer;
  
  PROCESS_BEGIN();
  //0x668a0
  //0x41EB0  45 min
  //3A980    40 min
  //2BF20    30 min
  etimer_set(&timer, CLOCK_SECOND/100);
  while(!find_asn) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    if(tsch_current_asn.ls4b > 0x2BF20) {
      tsch_schedule_print();
      count = 1;
      find_asn = 1;
    }
    etimer_reset(&timer);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
