
#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/mac/tsch/tsch.h"
#include "net/nbr-table.h"
#include <stdio.h>

#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-inout.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#include "sys/log.h"
#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC





int reverse_linkoption(int linkoption);
/*---------------------------------------------------------------------------*/
int 
reverse_linkoption(int linkoption)
{
  if (linkoption == LINK_OPTION_TX){
    return LINK_OPTION_RX;
  }
  else if (linkoption == LINK_OPTION_RX){
    return LINK_OPTION_TX;
  }
  else {
    return linkoption;
  }
}
/*---------------------------------------------------------------------------*/
void 
config_handle(struct sdn_packet* p, uint16_t len)
{
     int is_last_node = 0;
     int is_befor_last_node = 0;
     int is_middle_node = 0;
     int is_tx = 0;
     int is_rx = 0;
     /* here node checks whether is it the destination of config packet or no */
     int counter = C_LIST_OF_NODE_IN_PATH_INDEX;
     linkaddr_t addr;
     int i;

     for (i=0; i<p->payload[C_NUM_NODE_IN_PATH_INDEX]; i++){
       uint8_t j;
       for (j = 0; j< LINKADDR_SIZE; ++j){
         addr.u8[j]= p->payload[counter];
         counter++;
       }
       if(linkaddr_cmp(&addr, &linkaddr_node_addr)){
         printf("sdn-handle: addr match\n");
         if((i+2)==p->payload[C_NUM_NODE_IN_PATH_INDEX]){
           // you are the node one befor last node in the path. you must install schedule
	   is_befor_last_node = 1;
           printf("sdn-handle: one befor last node\n");

	   /* address of next node (last node in the path) */
           for (j = 0; j< LINKADDR_SIZE; ++j){
             addr.u8[j]= p->payload[counter + j];
           }
           counter = counter + LINKADDR_SIZE;
         }
         else if((i+1)==p->payload[C_NUM_NODE_IN_PATH_INDEX]){
           // you are the last node in the path. you must install schedule
	   is_last_node = 1;
           printf("sdn-handle: last node, and i value %d\n", i);

	   /* address of befor node (befor last node in the path) */
           for (j = 0; j< LINKADDR_SIZE; ++j){
             addr.u8[j]= p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
           }
         }
         else {
           // you are a middle node in the path. you must just forward the packet
           is_middle_node = 1;
	   /* address of next hop */
           for (j = 0; j< LINKADDR_SIZE; ++j){
             addr.u8[j]= p->payload[counter + j];
           }
         }
         break;
       }
     }
     /* add schedule */
     if(is_last_node){

       struct tsch_neighbor *n;
       int flow_id_index = counter;
       printf("counter 1 : %d\n", flow_id_index);
       counter = counter + 2;
       int sf_id_index = counter;
       printf("counter 2 : %d\n", sf_id_index);
       counter++;
       int num_cell_index = counter;
       printf("counter 3 : %d\n", num_cell_index);
       struct sdn_link link;

       /* read neighbor address */
       linkaddr_copy(&link.addr, &addr);
       LOG_INFO("nbr addr ");
       LOG_INFO_LLADDR(&link.addr);
       LOG_INFO_("\n");

       /* read SF ID */
       link.sf = p->payload[sf_id_index];

       for(i=0; i<p->payload[num_cell_index]; i++){
	 counter++;
         link.slot = p->payload[counter];
         printf("counter 4 : %d\n", link.slot);
	 counter++;
	 link.channel_offset = p->payload[counter];
         printf("counter 5 : %d\n", link.channel_offset);
	 counter++;
         link.link_option = p->payload[counter] & 0x0f;
         printf("counter 6 : %d\n", link.link_option);
         if(link.link_option == LINK_OPTION_TX){
	   is_tx++;
	 }
         if(link.link_option == LINK_OPTION_RX){
	   is_rx++;
	 } 
         link.link_type = (p->payload[counter] & 0xf0)>>4;
         printf("counter 7 : %d\n", link.link_type);

         /* read flow ID */
         linkaddr_t flow_id;
         flow_id.u8[0] = p->payload[flow_id_index];
         flow_id.u8[1] = p->payload[flow_id_index + 1];
         linkaddr_copy(&link.flow_id, &flow_id);
         printf("counter 8 : \n");
   
         tsch_schedule_add_link(sf0,
                                link.link_option,
	                        link.link_type, &link.addr, link.slot, 
                                link.channel_offset, 1);

	 //count slots coresspond to from_controller flow id
         if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)){
	   num_from_controller_rx_slots++;
         }

         if (link.link_option == LINK_OPTION_TX){
           /* add flow to the flow table */
           sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
           /* add NBR and update tx link for it */
           n = tsch_queue_add_nbr(&link.flow_id);
           if(n != NULL) {
             n->tx_links_count++;
             if(!(link.link_option & LINK_OPTION_SHARED)) {
               n->dedicated_tx_links_count++;
               printf("sdn-handle: install flow entry 1 \n");
             }
           }
           printf("sdn-handle: install flow entry \n");  
         }  
       }

       if(!sdn_is_joined){
         sdn_is_joined = 1;
       }
       tsch_schedule_print();
     }

     if (is_befor_last_node){
       /* send config to next hop */
       //TODO you must look-up over routing table if there is a flow_id (tx) for next hop
       // you must sent the config packe to that destination, if no it means that next hop is a
       // novel node and must send it over shared cells
       if((p->typ & 0xf0)>>4 == 1){
         printf("config for novel node\n");
         packetbuf_clear();
         packetbuf_copyfrom((uint8_t *)p, len);
         sdn_output(&addr, &flow_id_shared_cell, 1);
       } else {
         // I must set the real receiver address here
         printf("config is not for novel node\n");
         packetbuf_clear();
         packetbuf_copyfrom((uint8_t *)p, len);
         sdn_output(&addr, &flow_id_from_controller, 1);
       }

       struct tsch_neighbor *n;
       int flow_id_index = counter;
       printf("counter 1 : %d\n", flow_id_index);
       counter = counter + 2;
       int sf_id_index = counter;
       printf("counter 2 : %d\n", sf_id_index);
       counter++;
       int num_cell_index = counter;
       printf("counter 3 : %d\n", num_cell_index);
       struct sdn_link link;

       /* read neighbor address */
       //TODO I must set link->addr (Neighbor addr) to which NBR? in downward we have multiple receiver
       linkaddr_copy(&link.addr, &addr);
       LOG_INFO("nbr addr ");
       LOG_INFO_LLADDR(&link.addr);
       LOG_INFO_("\n");
       /* read flow ID */
       linkaddr_t flow_id;
       flow_id.u8[0] = p->payload[flow_id_index];
       flow_id.u8[1] = p->payload[flow_id_index + 1];
       linkaddr_copy(&link.flow_id, &flow_id);
       /* read SF ID */
       link.sf = p->payload[sf_id_index];

       for(i=0; i<p->payload[num_cell_index]; i++){
	 counter++;
         link.slot = p->payload[counter];
         printf("counter 4 : %d\n", link.slot);
	 counter++;
	 link.channel_offset = p->payload[counter];
	 printf("counter 5 : %d\n", link.channel_offset);
	 counter++;
	 /* 
	 * PROBLEM: if befor_last_hop node install the schedule of Config packet. it sends the config through installed schedule
	 * while the it must send it over shared cells. So, novel node is not able to receive the config
	 */
         //TODO link.link_option = p->payload[counter] & 0x0f;
         link.link_option = reverse_linkoption(p->payload[counter] & 0x0f);
         printf("counter 6 : %d\n", link.link_option);
         if(link.link_option == LINK_OPTION_TX){
	   is_tx++;
	 }
         if(link.link_option == LINK_OPTION_RX){
	   is_rx++;
	 }
         link.link_type = (p->payload[counter] & 0xf0)>>4;
         printf("counter 7 : %d\n", link.link_type);

         tsch_schedule_add_link(sf0,
                                link.link_option,
	                        link.link_type, &link.addr, link.slot, 
                                link.channel_offset, 1); 

         if (link.link_option == LINK_OPTION_TX){
           sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
           /* add NBR and update tx link for it */
           n = tsch_queue_add_nbr(&link.flow_id);
           if(n != NULL) {
             n->tx_links_count++;
             if(!(link.link_option & LINK_OPTION_SHARED)) {
               n->dedicated_tx_links_count++;
               printf("sdn-handle: install flow entry 1 \n");
             }
           }
         }   
   
       }
       printf("sdn-handle: this is_befor_last_node \n");
       tsch_schedule_print();
     }

     if (is_middle_node){
       //must look up routing table to find a the next hop and find a flow id based on the address of next hop and tag the pkt and send it
       packetbuf_clear();
       packetbuf_copyfrom((uint8_t *)p, len);
       sdn_output(&addr, &flow_id_from_controller, 1);
       printf("sdn-handle: must look up in flow table \n");
     }

}
/*---------------------------------------------------------------------------*/
void 
sdn_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id)
{
  int i;
  struct sdn_packet *p = create_sdn_packet();
  memcpy((uint8_t*)p, buf, len);

  printf("count %u\n", p->typ);
  for (i=0; i<len-1; i++){
    printf("count %u\n", p->payload[i]);
  }

#if SINK
/* this code is for sink node to send config packet for nodes that are not joined to the network */
 //  static linkaddr_t src=         {{ 0x02, 0x00 }};
 if ((p->typ & 0x0f) == REPORT){
     int is_register = is_registered(p);
     struct sdn_packet *config;

   switch(is_register){
     case 1:
       printf("sdn-handle: register node\n");
       break;

     case 0:
       printf("sdn-handle: not register node case 0\n");
       if (p->payload[0] == 0x02){
         config = create_confing();
         config_handle(config, 13);
         packet_deallocate(config);
       }
       if (p->payload[0] == 0x03){
         config = create_confing2();
         config_handle(config, 15);
         packet_deallocate(config);
       }
       break;

     case 2:
       printf("sdn-handle: not register node case 2\n");
       if (p->payload[0] == 0x02){
         config = create_confing1();
         config_handle(config, 13);
         packet_deallocate(config);
       }
       if (p->payload[0] == 0x03){
         config = create_confing3();
         config_handle(config, 15);
         packet_deallocate(config);
       }
       break;

     case 4:
       printf("sdn-handle: register with no best effort\n");
       if (p->payload[0] == 0x02){
         config = create_confing4();
         config_handle(config, 13);
         packet_deallocate(config);
       }
       if (p->payload[0] == 0x03){
         config = create_confing5();
         config_handle(config, 15);
         packet_deallocate(config);
       }
       break;

   }

 } 
#endif

#if !SINK
/* process of sdn packets: normal nodes */
switch((p->typ & 0x0f)){
   case CONFIG:
     config_handle(p, len);
     printf("sdn-handle: add sdn schedule successfully\n");
     break;
   
   case REPORT:
     if (sdn_is_joined){
       printf("sdn-handle: middle node received a report packet\n");
       packetbuf_clear();
       packetbuf_copyfrom((uint8_t *)p, len);
       sdn_output(&linkaddr_null, flow_id, 1);
       break;
     }

   default:
     printf("sdn-handle: normal node received a non valid sdn type\n");
     break;
}//switch
#endif
   packet_deallocate(p);
}
/*---------------------------------------------------------------------------*/

