/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "udp-server.h"
#include "sys/energest.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-flow.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-statis.h"
#include "net/mac/tsch/sdn/sdn-algo.h"
#include <math.h>

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_SERVER_APP_LOG_LEVEL

#define WITH_SERVER_REPLY  0
#define UDP_APP1_PORT	       1020
#define UDP_APP2_PORT	       1030
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;
static int should_print_sch = 0;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
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
  static uint32_t sent_asn;
  static uint16_t seq_num;
  linkaddr_t src_addr;
  linkaddr_t dest_addr;
  
  src_addr.u8[0] = (uint8_t)data[0];
  src_addr.u8[1] = (uint8_t)data[1];
  
  dest_addr.u8[0] = (uint8_t)data[2];
  dest_addr.u8[1] = (uint8_t)data[3];
  
  seq_num =  (uint8_t)data[4] |
             (uint8_t)data[5] << 8;
  
  sent_asn = (uint8_t)data[6] |
             (uint8_t)data[7] << 8 |
             (uint8_t)data[8] << 16 |
             (uint8_t)data[9] << 24;
  LOG_INFO("11111111 |sr %d%d sr|d %d%d d|s %u s|as 0x%lx as|ar 0x%lx ar|c %d c|p %d p|", src_addr.u8[0], src_addr.u8[1], dest_addr.u8[0], dest_addr.u8[1], seq_num, sent_asn, tsch_current_asn.ls4b, packetbuf_attr(PACKETBUF_ATTR_CHANNEL), sender_port);
  LOG_INFO_("\n");
  
  if(!should_print_sch && tsch_current_asn.ls4b > SDN_PRINT_ASN) {
    //tsch_schedule_print();
    sdn_schedule_stat_print();
    print_max_sf_cell_usage();
    print_never_used_cell_num();
    //print_global_table();
    should_print_sch = 1;
  }

  //LOG_INFO_("from");
  //LOG_INFO_6ADDR(sender_addr);
#if WITH_SERVER_REPLY
  /* send back the same string to the client as an echo reply */
  LOG_INFO("Sending response.\n");
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */

}
/*----------------------------------veisi-------------------------------------*/
void
create_initial_schedule_sink(void)
{
    // struct tsch_slotframe *sf;
    tsch_schedule_remove_all_slotframes();
    //we consider slotframe_handle = 0
    int ctrl_sf_handle = 0;
    //int data_sf_handle = 1;
    int initial_ch_off = 0;
    sf0 = tsch_schedule_add_slotframe(ctrl_sf_handle, SDN_DATA_SLOTFRAME_SIZE);
    //sf1 = tsch_schedule_add_slotframe(data_sf_handle, SDN_DATA_SLOTFRAME_SIZE);
    
    sf0->num_shared_cell = sdn_num_shared_cell;
    
    int ts_offs = sdn_get_sink_ts_eb_offset();
    if(ts_offs != -1) {
      sf0->max_sf_offs = get_sf_offset_from_ts(ts_offs);
      sf0->max_sf_offs = ((sf0->max_sf_offs != -1) ? sf0->max_sf_offs : 0); 
    } else {
      sf0->max_sf_offs = 0;
    }
    
    //printf("sink sf offs from ts: %d \n", sf0->max_sf_offs);
    int i;
    struct tsch_neighbor *n;
    
    //int num_shared_cell_in_rep = sf0->num_shared_cell / (SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD);
    //printf("num shared cell: %d %d \n", sdn_num_shared_cell, num_shared_cell_in_rep);
    //int j = 0;
    //int k;
    
    
  int num_rep = (int)(SDN_DATA_SLOTFRAME_SIZE / SDN_SF_REP_PERIOD);
  int num_sh_cell_in_rep = (int)ceil((float)((float)sdn_num_shared_cell / (float)num_rep));
  printf("num_sh_cell_in_rep: %d \n", num_sh_cell_in_rep);
#define SHARED_CELL_LIST_LEN_TMP  DIST_UNIFORM_LEN  
  const int list_shared_cell_len = (int)(num_sh_cell_in_rep * num_rep);
  static int list_of_shared_cell[SHARED_CELL_LIST_LEN_TMP];
  
  for(i=0; i<SHARED_CELL_LIST_LEN_TMP; i++) {
    list_of_shared_cell[i] = -1;
    //printf("list_of_shared_cell %d len %d\n", list_of_shared_cell[i], list_shared_cell_len);
  }
  
  
  static struct list_dist_uniform list_dist;
  list_dist.len = list_shared_cell_len;
  
  for(i=0; i<DIST_UNIFORM_LEN; i++) {
    list_dist.list[i] = 0;
  }
  sdn_distribute_list_uniform(&list_dist);
  
  int m;
  for(i=1; i<=sdn_num_shared_cell; i++) {  
    for(m=0; m<list_shared_cell_len; m++) {
      if(list_dist.list[m] == i) {
        list_of_shared_cell[i-1] = ((m / num_sh_cell_in_rep) * (SDN_SF_REP_PERIOD)) + 
                                   ((m % num_sh_cell_in_rep) * (int)(SDN_SF_REP_PERIOD / num_sh_cell_in_rep));        
      }
    }
  }
 
    
  for(i=0; i<SHARED_CELL_LIST_LEN_TMP; i++) {
    if(list_of_shared_cell[i] != -1) {
      tsch_schedule_add_link(sf0,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING,
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      list_of_shared_cell[i], initial_ch_off, 1);


        //if (link.link_option == LINK_OPTION_TX){

      sdn_add_flow_enrty(&flow_id_shared_cell, ctrl_sf_handle, list_of_shared_cell[i], initial_ch_off, 0);

      n = tsch_queue_add_nbr(&flow_id_shared_cell);
      if(n != NULL) {
        n->tx_links_count++;
        //if(!(link.link_option & LINK_OPTION_SHARED)) {
        //  n->dedicated_tx_links_count++;
        //}
      }
        //} 
    } 
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();
  
  static int find_asn = 0;
  static struct etimer timer;

  tsch_set_coordinator(1);
  //NETSTACK_MAC.off();
  create_initial_schedule_sink();
  NETSTACK_MAC.on();
//#endif
  
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_APP1_PORT, udp_rx_callback);
                      
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_APP2_PORT, udp_rx_callback);
                      
  etimer_set(&timer, CLOCK_SECOND/10);
  while(!find_asn) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    if(tsch_current_asn.ls4b > SDN_PRINT_ASN) {
        //tsch_schedule_print();
        //count = 1;
        find_asn = 1;
    }
    etimer_reset(&timer);
  }
  
  energest_flush();

  printf("\nENGY after join->ID[%d%d]", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  printf(" CPU:%lus LPM:%lus DEEPLPM:%lus Tot_time:%lus",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
  printf(" Radio LISTEN:%lus TRANSMIT:%lus OFF:%lus ]]\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
