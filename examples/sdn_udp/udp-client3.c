#include "contiki.h"
//#include "net/routing/routing.h"
#include "random.h"
//#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/mac/tsch/tsch.h"
#include "sys/energest.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-flow.h"
#include "net/mac/tsch/sdn/sdn-statis.h"

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_CLIENT_APP_LOG_LEVEL

/* App QoS requirements: */
#define TRAFFIC_PERIOD  (APP_INT / 10) //per timeslot
#define MAX_DELAY       0
#define PACKET_SIZE     0
#define RELIABILITY     99
#define BURST_SIZE      0

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	1020
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		((APP_INT / 1000) * CLOCK_SECOND)

//static linkaddr_t dest_addr = {{ 0x00, 0x01 }};


static struct simple_udp_connection udp_conn;
static struct app_qos_attr app_qos;
static linkaddr_t app_dest;
int j = 0;
static uint16_t count = 1;
static int print_schedule = 0;
static uint16_t first_timeslot;
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
//PROCESS(seqnum_reset_process, "seqnum reset process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / (ENERGEST_SECOND/1000));
}
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
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer initial_timer;
  static int dummy_size = 52; //if change, change following value 52+6 = 58
  static uint8_t msg[2*LINKADDR_SIZE+58];
  static int tx_count_in_sf = 0;
  //static char str[32];
  uip_ipaddr_t server_ipaddr;
  uip_ip6addr(&server_ipaddr,  0xfe80, 0, 0, 0, 0, 0xff, 0xfe00, 0x1);


  
  PROCESS_BEGIN();
#if SDN_ENABLE
  app_qos_request(&udp_conn, &app_qos, TRAFFIC_PERIOD, RELIABILITY, MAX_DELAY, PACKET_SIZE, BURST_SIZE);
#endif
  /* Initialize UDP connection */
  //udp_conn.qos_attr = app_qos;
  int en_join_is_printed = 0;
  etimer_set(&initial_timer, CLOCK_SECOND);
  while(num_from_controller_rx_slots == 0){   
    PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
    etimer_reset(&initial_timer);
    if(tsch_current_asn.ls4b > SDN_PRINT_ASN && !en_join_is_printed){
      en_join_is_printed = 1;
      energest_flush();

        printf("\nENGY right join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
        printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time:%lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
        printf(" Radio LISTEN:%lus TRANSMIT:%lus OFF:%lus ]]0x%x>>\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)),
                      tsch_current_asn.ls4b);
    }
  }
  //LOG_INFO("num from cells %d\n", num_from_controller_rx_slots);
  
  energest_flush();

        printf("\nENGY right join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
        printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time:%lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
        printf(" Radio LISTEN:%lus TRANSMIT:%lus OFF:%lus ]]0x%x>>\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)),
                      tsch_current_asn.ls4b);
  
  
  etimer_set(&periodic_timer, (1200 + (random_rand() % 200))*CLOCK_SECOND);  // 15 min initial delay to converge
  PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
  
  
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, &server_ipaddr,
                      UDP_SERVER_PORT, udp_rx_callback);
   
   
                      
  etimer_set(&periodic_timer, CLOCK_SECOND);
  while(linkaddr_cmp(&udp_conn.udp_conn->flow_id, &linkaddr_null)){
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }
  
  
  
  if(linkaddr_cmp(&udp_conn.udp_conn->flow_id, &flow_id_best_effort)){
    int fid_exsit;
    etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
    while((fid_exsit = sdn_is_flowid_exist_in_table(&flow_id_best_effort)) == 0){
      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }
      
  
  /* print energy until joining to SDN */
  energest_flush();

  printf("\nENGY before join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time: %lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
  printf(" Radio LISTEN: %lus TRANSMIT:%lus OFF:%lus ]]0x%x>>\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)),
                      tsch_current_asn.ls4b);
  
  first_timeslot = sdn_find_first_timeslot_in_slotframe(&udp_conn.udp_conn->flow_id);
  printf("first timeslot in sf: %d, rep_period: %d \n", first_timeslot, udp_conn.udp_conn->rep_period);
  
  //etimer_set(&periodic_timer, 2400*CLOCK_SECOND);
  //PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer)); 
  
  
  app_dest.u8[0] = server_ipaddr.u8[14];
  app_dest.u8[1] = server_ipaddr.u8[15];
     
  /* force app to start generating packet in the begining of SF  */   
  etimer_set(&initial_timer, CLOCK_SECOND/1000);
  while((tsch_current_asn.ls4b % SDN_DATA_SLOTFRAME_SIZE) != 0){   
    PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
    etimer_reset(&initial_timer);
  }               
  
  etimer_set(&initial_timer, (CLOCK_SECOND/100) * (first_timeslot - 1));
  PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
   
  //etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    //if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      //LOG_INFO("Sending data %u to sink with asn 0x%x ", count, tsch_current_asn.ls4b);
      //LOG_INFO_6ADDR(&server_ipaddr);
      //LOG_INFO_("\n");
      //snprintf(str, sizeof(str), "hello %d", count);
      //simple_udp_sendto(&udp_conn, str, strlen(str), &server_ipaddr);
      
      for(j = 0; j<dummy_size; ++j){
        msg[j] = dummy_size - j;
      }
      
      for(j = 0; j< LINKADDR_SIZE; ++j){
        msg[dummy_size + j] = linkaddr_node_addr.u8[j];
      }
      msg[dummy_size + 2] =  server_ipaddr.u8[14];
      msg[dummy_size + 3] =  server_ipaddr.u8[15];
      
      msg[dummy_size + 4] =  count & 0xFF;
      msg[dummy_size + 5] = (count >> 8)& 0xFF;
      msg[dummy_size + 6] =  tsch_current_asn.ls4b & 0xFF;
      msg[dummy_size + 7] = (tsch_current_asn.ls4b >> 8)& 0xFF;
      msg[dummy_size + 8] = (tsch_current_asn.ls4b >> 16)& 0xFF;
      msg[dummy_size + 9] = (tsch_current_asn.ls4b >> 24)& 0xFF;
      
      
      
      udp_conn.udp_conn->app_seq = count;
      linkaddr_copy(&udp_conn.udp_conn->app_src_lladdr, &linkaddr_node_addr);
      linkaddr_copy(&udp_conn.udp_conn->app_dest_lladdr, &app_dest);
      
      
      //printf("msg %d \n ", msg[4]);
      //count = msg[3];
      simple_udp_sendto(&udp_conn, msg, sizeof(msg), &server_ipaddr);
      
      count++;
      if(count == 300){
        tsch_print_nbr_table();
      }
      if(tsch_current_asn.ls4b > SDN_PRINT_ASN && print_schedule == 0) {
        //tsch_schedule_print();
        sdn_schedule_stat_print();
        print_max_sf_cell_usage();
        print_never_used_cell_num();
        print_schedule = 1;
        
        /* print energy after joining to SDN */
        energest_flush();

        printf("\nENGY after join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
        printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time:%lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
        printf(" Radio LISTEN:%lus TRANSMIT:%lus OFF:%lus ]]0x%x>>\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)),
                      tsch_current_asn.ls4b);
      }
    //} else {
      //LOG_INFO("Not reachable yet\n");
    //}

    if(tx_count_in_sf == (SDN_DATA_SLOTFRAME_SIZE / udp_conn.udp_conn->rep_period)) {
      energest_flush();

      printf("\nENGY iter join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
      printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time:%lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
      printf(" Radio LISTEN:%lus TRANSMIT:%lus OFF:%lus ]]0x%x>>\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)),
                      tsch_current_asn.ls4b);
    
      /* set timer */
      etimer_set(&initial_timer, CLOCK_SECOND/1000);
      while((tsch_current_asn.ls4b % SDN_DATA_SLOTFRAME_SIZE) != 0){   
        PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
        etimer_reset(&initial_timer);
      }
    
      etimer_set(&initial_timer, (CLOCK_SECOND/100) * (first_timeslot - 1));
      PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
      tx_count_in_sf = 1;
      //etimer_set(&periodic_timer, SEND_INTERVAL);
    } else{
      etimer_set(&initial_timer, (CLOCK_SECOND/100) * udp_conn.udp_conn->rep_period);
      PROCESS_WAIT_UNTIL(etimer_expired(&initial_timer));
      tx_count_in_sf++;
    }
    
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/*PROCESS_THREAD(seqnum_reset_process, ev, data)
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
    if(tsch_current_asn.ls4b > 0xe7E40) {
        //tsch_schedule_print();
        //count = 1;
        find_asn = 1;
    }
    etimer_reset(&timer);
  }
  PROCESS_END();
} */
/*---------------------------------------------------------------------------*/


