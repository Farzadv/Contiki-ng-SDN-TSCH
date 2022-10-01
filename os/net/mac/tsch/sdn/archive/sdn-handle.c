
#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/mac/tsch/tsch.h"
#include "net/nbr-table.h"
#include "lib/memb.h"
#include <malloc.h>
#include <stdio.h>

#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-flow-request.h"
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
sdn_handle_config_packet(struct sdn_packet *p, uint16_t len, const linkaddr_t *src_addr)
{
  int is_first_node_in_src_route = 0;
  int is_middle_node_in_src_route = 0;
  int is_last_node_in_src_route = 0;
  int position_in_src_route_list;
  int need_to_send_by_to_ctrl_fid = 0;
  linkaddr_t config_flow_id;
  linkaddr_t addr;
  linkaddr_t next_hop_addr;
  linkaddr_t prvs_hop_addr;
  int counter = CONF_LIST_OF_NODE_IN_PATH_INDEX;
  int i;
  for(i=0; i<p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]; i++){
    uint8_t j;
    for(j = 0; j< LINKADDR_SIZE; ++j){
      addr.u8[j] = p->payload[counter];
      counter++;
    }
    printf("sdn-handle: config process-> track 1 -> i = %d\n",i);
    if(linkaddr_cmp(&addr, &linkaddr_node_addr)){
      // first node in list of source routing
      if(i==0){
        if(linkaddr_cmp(src_addr, &linkaddr_null)){
          printf("sdn-handle: config process-> track 2\n");
          is_first_node_in_src_route = 1;
          position_in_src_route_list = i+1;
          for(j = 0; j< LINKADDR_SIZE; ++j){
            next_hop_addr.u8[j] = p->payload[counter + j];
          }
          printf("sdn-handle: config process-> first node in list\n");
          counter = counter + (((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) -1) * LINKADDR_SIZE);
          break;
        }
      }
      // last node in list of source routing
      else if((i+1)==p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]){
        is_last_node_in_src_route = 1;
        position_in_src_route_list = i+1;
        for(j = 0; j< LINKADDR_SIZE; ++j){
          prvs_hop_addr.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
         printf("sdn-handle: config process-> last node in list\n");
        break;
      }
      // middle node in list of source routing
      else{
        for(j = 0; j< LINKADDR_SIZE; ++j){
          prvs_hop_addr.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
        if(linkaddr_cmp(&prvs_hop_addr, src_addr)){
          is_middle_node_in_src_route = 1;
          position_in_src_route_list = i+1;
          for(j = 0; j< LINKADDR_SIZE; ++j){
            next_hop_addr.u8[j] = p->payload[counter + j];
          }
           printf("sdn-handle: config process-> midlle node in list\n");
          counter = counter + (((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - position_in_src_route_list) * LINKADDR_SIZE);
          
          break;
        }
      }
    }
  }
  
  if((is_first_node_in_src_route || is_middle_node_in_src_route) && ((p->typ & 0xf0)>>4 == 0)){
    int counter_index = CONF_LIST_OF_NODE_IN_PATH_INDEX;
    linkaddr_t curr_addr;
    for(i=1; i<position_in_src_route_list; i++){
      uint8_t j;
      for(j = 0; j< LINKADDR_SIZE; ++j){
        curr_addr.u8[j] = p->payload[counter_index];
        counter_index++;
      }
      if(linkaddr_cmp(&curr_addr, &next_hop_addr)){
        need_to_send_by_to_ctrl_fid ++;
      }
    }
  }
  
  if((is_first_node_in_src_route || is_middle_node_in_src_route) && ((p->typ & 0xf0)>>4 == 1)){
    int flow_id_index = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
    config_flow_id.u8[0] = p->payload[flow_id_index];
    config_flow_id.u8[1] = p->payload[flow_id_index + 1];
  }
  
  if(is_first_node_in_src_route){
    int flow_id_index;
    int sf_id_index;
    struct sdn_link link;
    int next_num_cell_per_hop;

    next_num_cell_per_hop = p->payload[counter+(position_in_src_route_list - 1)];
    if(next_num_cell_per_hop > 0){
      counter = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
      flow_id_index = counter;
      counter = counter + 2;
      sf_id_index = counter;
      counter++;
      /* read neighbor address */
      linkaddr_copy(&link.addr, &next_hop_addr);
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
      /* read SF ID */
      link.sf = p->payload[sf_id_index];
      for(i=0; i<next_num_cell_per_hop; i++){
        link.slot = p->payload[counter];
        counter++;
        link.channel_offset = p->payload[counter]; 
        counter++;
    
        tsch_schedule_add_link(sf0,
                               LINK_OPTION_RX,
	                       LINK_TYPE_NORMAL, &link.addr, link.slot, 
                               link.channel_offset, 1);  
      }
      tsch_schedule_print();
    } else {
      printf("sdn-handle: first node in list->only forward \n");
    }
  }

  if(is_last_node_in_src_route){
    struct tsch_neighbor *n;
    int flow_id_index;
    int sf_id_index;
    struct sdn_link link;
    int prvs_num_cell_per_hop;
    int sum_lapsed_cell = 0;
    
    prvs_num_cell_per_hop = p->payload[counter+(position_in_src_route_list - 2)];
    if(prvs_num_cell_per_hop > 0){
      for(i=0; i<(position_in_src_route_list - 2); i++){
        sum_lapsed_cell = p->payload[counter+i] + sum_lapsed_cell;
      }
      counter = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
      flow_id_index = counter;
      counter = counter + 2;
      sf_id_index = counter;
      counter++;
      /* read neighbor address */
      linkaddr_copy(&link.addr, &prvs_hop_addr);
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
      /* read SF ID */
      link.sf = p->payload[sf_id_index];
      link.link_option = LINK_OPTION_TX;
      counter = counter + (sum_lapsed_cell * 2);
      for(i=0; i<prvs_num_cell_per_hop; i++){
        link.slot = p->payload[counter];
        counter++;
        link.channel_offset = p->payload[counter]; 
        counter++;
    
        tsch_schedule_add_link(sf0,
                               LINK_OPTION_TX,
	                       LINK_TYPE_NORMAL, &link.addr, link.slot, 
                               link.channel_offset, 1);
       
      
        if (link.link_option == LINK_OPTION_TX){
        /* add flow to the flow table */
        sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
        /* add NBR and update tx link for it */
        n = tsch_queue_add_nbr(&link.flow_id);
          if(n != NULL) {
            n->tx_links_count++;
            if(!(link.link_option & LINK_OPTION_SHARED)) {
              n->dedicated_tx_links_count++;
              printf("sdn-handle: install data flow entry 1 \n");
            }
          }  
        }  
      }//for
      if(p->payload[CONF_REQUEST_NUM_INDEX] > 0){
        sdn_response_for_flow_id(p->payload[CONF_REQUEST_NUM_INDEX], &link.flow_id);
      }
      tsch_schedule_print();
    }	  
  }

  if(is_middle_node_in_src_route){
    struct tsch_neighbor *n;
    int flow_id_index;
    int sf_id_index;
    struct sdn_link link;
    int next_num_cell_per_hop;
    int prvs_num_cell_per_hop;
    
    prvs_num_cell_per_hop = p->payload[counter+(position_in_src_route_list - 2)];
    next_num_cell_per_hop = p->payload[counter+(position_in_src_route_list - 1)];
    if(prvs_num_cell_per_hop > 0 && next_num_cell_per_hop > 0){
      int sum_lapsed_cell = 0;
      for(i=0; i<(position_in_src_route_list - 2); i++){
        sum_lapsed_cell = p->payload[counter+i] + sum_lapsed_cell;
      }
      counter = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
      flow_id_index = counter;
      counter = counter + 2;
      sf_id_index = counter;
      counter++;
      /* read neighbor address */
      linkaddr_copy(&link.addr, &prvs_hop_addr);
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
      /* read SF ID */
      link.sf = p->payload[sf_id_index];
      counter = counter + (sum_lapsed_cell * 2);
      for(i=0; i<prvs_num_cell_per_hop; i++){
        link.link_option = LINK_OPTION_TX;
        link.slot = p->payload[counter];
	counter++;
	link.channel_offset = p->payload[counter]; 
	counter++;
		    
	tsch_schedule_add_link(sf0,
		               LINK_OPTION_TX,
			       LINK_TYPE_NORMAL, &link.addr, link.slot, 
		               link.channel_offset, 1);
		               
        if (link.link_option == LINK_OPTION_TX){
          /* add flow to the flow table */
	  sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
	  /* add NBR and update tx link for it */
	  n = tsch_queue_add_nbr(&link.flow_id);
	  if(n != NULL) {
	    n->tx_links_count++;
	    if(!(link.link_option & LINK_OPTION_SHARED)) {
	      n->dedicated_tx_links_count++;
              printf("sdn-handle: install data flow entry 1 \n");
            }
          }  
        }  
      }
      tsch_schedule_print();
      // next hop config
      linkaddr_copy(&link.addr, &next_hop_addr);
      for(i=0; i<next_num_cell_per_hop; i++){
        link.slot = p->payload[counter];
	counter++;
	link.channel_offset = p->payload[counter]; 
	counter++;
	    
	tsch_schedule_add_link(sf0,
	                       LINK_OPTION_RX,
		               LINK_TYPE_NORMAL, &link.addr, link.slot, 
	                       link.channel_offset, 1); 
	                       
	if(!sdn_is_joined && linkaddr_cmp(&link.flow_id, &flow_id_from_controller)){
	  sdn_is_joined = 1;
        }
        
	//count slots coresspond to from_controller flow id
        if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)){
	  num_from_controller_rx_slots++;
	  printf("sdn-handle: add number of from_ctrl fid \n");
        }  
      }
      tsch_schedule_print();
    } else if(prvs_num_cell_per_hop == 0 && next_num_cell_per_hop > 0){
      int sum_lapsed_cell = 0;
      for(i=0; i<(position_in_src_route_list - 1); i++){
        sum_lapsed_cell = p->payload[counter+i] + sum_lapsed_cell;
      }
      counter = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
      flow_id_index = counter;
      counter = counter + 2;
      sf_id_index = counter;
      counter++;
      /* read neighbor address */
      linkaddr_copy(&link.addr, &next_hop_addr);
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
      /* read SF ID */
      link.sf = p->payload[sf_id_index];
      counter = counter + (sum_lapsed_cell * 2);
      for(i=0; i<next_num_cell_per_hop; i++){
        link.slot = p->payload[counter];
	counter++;
	link.channel_offset = p->payload[counter]; 
        counter++;
	    
	tsch_schedule_add_link(sf0,
	                       LINK_OPTION_RX,
		               LINK_TYPE_NORMAL, &link.addr, link.slot, 
	                       link.channel_offset, 1);  
	                       
	if(!sdn_is_joined && linkaddr_cmp(&link.flow_id, &flow_id_from_controller)){
	  sdn_is_joined = 1;
        }
        
	 //count slots coresspond to from_controller flow id
        if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)){
	  num_from_controller_rx_slots++;
	  printf("sdn-handle: add number of from_ctrl fid \n");
        } 

      }
      tsch_schedule_print();
    } else {
      printf("sdn-handle: no next nor prvs config \n");
    }		    
  }
  
  // relay the Config packet
  if(((p->typ & 0xf0)>>4 == 1) && (is_first_node_in_src_route || is_middle_node_in_src_route)){
    if(linkaddr_cmp(&config_flow_id, &flow_id_to_controller) && 
      (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) == position_in_src_route_list){
      printf("config for novel node: fid->to_ctrl, befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_shared_cell, 1, NULL);
    } else if(linkaddr_cmp(&config_flow_id, &flow_id_from_controller) && 
             (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 2) == position_in_src_route_list){
      printf("config for novel node: fid->from_ctrl, 2 befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_shared_cell, 1, NULL);
    } else if(linkaddr_cmp(&config_flow_id, &flow_id_from_controller) && 
             (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) == position_in_src_route_list){
      printf("config for novel node: fid->from_ctrl, 1 befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_to_controller, 1, NULL);
    } else{
      printf("config for novel node: fid->from_ctrl, relay middle node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_from_controller, 1, NULL);
    }  
  } else if(((p->typ & 0xf0)>>4 == 0) && (is_first_node_in_src_route || is_middle_node_in_src_route)){
    if(need_to_send_by_to_ctrl_fid > 0){
      printf("config for join node: fid->to_ctrl\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_to_controller, 1, NULL);
    } else{
      printf("config for join node: fid->from_ctrl\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_from_controller, 1, NULL);
    }
  } else{
    printf("confuse to relay config packet\n");
  }

 
  if(is_last_node_in_src_route){
    printf("sdn-handle: last node -> send ack \n");
    send_config_ack(p->payload[CONF_SEQ_NUM_INDEX]);
  }

}
/*---------------------------------------------------------------------------*/
int 
send_config_ack(int seq_num)
{
  struct sdn_packet *ack = create_sdn_packet();
  if(ack != NULL){
    uint8_t j;
    int len = 0;
    ack->typ = CONFIG_ACK;
    len++;
    
    for(j = 0; j<LINKADDR_SIZE; ++j){
      ack->payload[ACK_SENDER_ADDR_INDEX+j] = linkaddr_node_addr.u8[j];
      len++;
    }
    
    ack->payload[ACK_SEQ_NUM_INDEX] = seq_num;
    len++;
#if SINK
    handle_config_ack_packet(ack);
#else 
    packetbuf_clear();
    packetbuf_copyfrom((uint8_t *)ack, len);
    sdn_output(&linkaddr_null, &flow_id_to_controller, 1, NULL);
#endif
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
void 
sdn_handle_config_data_plane_path(struct sdn_packet *p, uint16_t len)
{
  int is_first_node = 0;
  int is_middle_node = 0;
  int is_last_node = 0;
  int position_in_list = 0;
  int counter = C_LIST_OF_NODE_IN_PATH_INDEX;
  linkaddr_t addr;
  linkaddr_t addr_to;
  linkaddr_t addr_from;
  int i;
  for (i=0; i<p->payload[C_NUM_NODE_IN_PATH_INDEX]; i++){
    uint8_t j;
    for (j = 0; j< LINKADDR_SIZE; ++j){
      addr.u8[j] = p->payload[counter];
      counter++;
    }
    if(linkaddr_cmp(&addr, &linkaddr_node_addr)){
      //first node in list of source routing
      if(i==0 && (p->payload[C_NUM_NODE_IN_PATH_INDEX] > 1)){
        is_first_node = 1;
        for (j = 0; j< LINKADDR_SIZE; ++j){
          addr_from.u8[j] = p->payload[counter + j];
        }
        counter = counter + (((p->payload[C_NUM_NODE_IN_PATH_INDEX]) -1) * LINKADDR_SIZE);
      } 
      // last node in list of source routing
      else if((i+1)==p->payload[C_NUM_NODE_IN_PATH_INDEX]){
        is_last_node = 1;
        position_in_list = i+1;
        for (j = 0; j< LINKADDR_SIZE; ++j){
          addr_to.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
      }
      // middle node in list of source routing
      else {
        is_middle_node = 1;
        position_in_list = i+1;
        for (j = 0; j< LINKADDR_SIZE; ++j){
          addr_from.u8[j] = p->payload[counter + j];
        }
        for (j = 0; j< LINKADDR_SIZE; ++j){
          addr_to.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
        counter = counter + (((p->payload[C_NUM_NODE_IN_PATH_INDEX]) - position_in_list) * LINKADDR_SIZE);
      }
      break;
    }
  }
  
  if(is_first_node){
  
    struct tsch_neighbor *n;
    int flow_id_index = counter;
    counter = counter + 2;
    int sf_id_index = counter;
    counter++;
    int num_cell_index = counter;
    struct sdn_link link;
    
    /* read neighbor address */
    linkaddr_copy(&link.addr, &addr_from);
    /* read SF ID */
    link.sf = p->payload[sf_id_index];
    for(i=0; i<p->payload[num_cell_index]; i++){
      counter++;
      link.slot = p->payload[counter];
      counter++;
      link.channel_offset = p->payload[counter]; 
      counter++;
      link.link_option = reverse_linkoption(p->payload[counter] & 0x0f); 
      link.link_type = (p->payload[counter] & 0xf0)>>4;
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
   
      tsch_schedule_add_link(sf0,
                             link.link_option,
	                     link.link_type, &link.addr, link.slot, 
                             link.channel_offset, 1);

      if (link.link_option == LINK_OPTION_TX){
        /* add flow to the flow table */
        sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
        /* add NBR and update tx link for it */
        n = tsch_queue_add_nbr(&link.flow_id);
        if(n != NULL) {
          n->tx_links_count++;
          if(!(link.link_option & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count++;
            printf("sdn-handle: install data flow entry 1 \n");
          }
        }  
      }  
    }//for
  }//is_first_node
  
  if(is_middle_node){
  
    struct tsch_neighbor *n;
    int flow_id_index = counter;
    counter = counter + 2;
    int sf_id_index = counter;
    counter++;
    int num_cell_index = counter;
    struct sdn_link link;
    
    /* read neighbor address */
    linkaddr_copy(&link.addr, &addr_to);
    /* read SF ID */
    link.sf = p->payload[sf_id_index];
    counter = counter + ((position_in_list -2) * (p->payload[num_cell_index]) * 3);
    // install flow_id + schedule addr_to
    for(i=0; i<p->payload[num_cell_index]; i++){
      counter++;
      link.slot = p->payload[counter];
      counter++;
      link.channel_offset = p->payload[counter]; 
      counter++;
      link.link_option = p->payload[counter] & 0x0f; 
      link.link_type = (p->payload[counter] & 0xf0)>>4;
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
   
      tsch_schedule_add_link(sf0,
                             link.link_option,
	                     link.link_type, &link.addr, link.slot, 
                             link.channel_offset, 1);

      if (link.link_option == LINK_OPTION_TX){
        /* add flow to the flow table */
        sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
        /* add NBR and update tx link for it */
        n = tsch_queue_add_nbr(&link.flow_id);
        if(n != NULL) {
          n->tx_links_count++;
          if(!(link.link_option & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count++;
            printf("sdn-handle: install data flow entry 1 \n");
          }
        }  
      }  
    }//for
    
    // install flow + schedule addr_from
    /* read neighbor address */
    linkaddr_copy(&link.addr, &addr_from);
    for(i=0; i<p->payload[num_cell_index]; i++){
      counter++;
      link.slot = p->payload[counter];
      counter++;
      link.channel_offset = p->payload[counter]; 
      counter++;
      link.link_option = reverse_linkoption(p->payload[counter] & 0x0f); 
      link.link_type = (p->payload[counter] & 0xf0)>>4;
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
   
      tsch_schedule_add_link(sf0,
                             link.link_option,
	                     link.link_type, &link.addr, link.slot, 
                             link.channel_offset, 1);

      if (link.link_option == LINK_OPTION_TX){
        /* add flow to the flow table */
        sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
        /* add NBR and update tx link for it */
        n = tsch_queue_add_nbr(&link.flow_id);
        if(n != NULL) {
          n->tx_links_count++;
          if(!(link.link_option & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count++;
            printf("sdn-handle: install data flow entry 1 \n");
          }
        }  
      }  
    }//for     
  }//is_middle_node

  if(is_last_node){
    struct tsch_neighbor *n;
    int flow_id_index = counter;
    counter = counter + 2;
    int sf_id_index = counter;
    counter++;
    int num_cell_index = counter;
    struct sdn_link link;
    
    /* read neighbor address */
    linkaddr_copy(&link.addr, &addr_to);
    /* read SF ID */
    link.sf = p->payload[sf_id_index];
    counter = counter + ((position_in_list -2) * (p->payload[num_cell_index]) * 3);
    // install flow_id + schedule addr_to
    for(i=0; i<p->payload[num_cell_index]; i++){
      counter++;
      link.slot = p->payload[counter];
      counter++;
      link.channel_offset = p->payload[counter]; 
      counter++;
      link.link_option = p->payload[counter] & 0x0f; 
      link.link_type = (p->payload[counter] & 0xf0)>>4;
      /* read flow ID */
      linkaddr_t flow_id;
      flow_id.u8[0] = p->payload[flow_id_index];
      flow_id.u8[1] = p->payload[flow_id_index + 1];
      linkaddr_copy(&link.flow_id, &flow_id);
      LOG_INFO_LLADDR(&link.flow_id);
      LOG_INFO_("\n");
      tsch_schedule_add_link(sf0,
                             link.link_option,
	                     link.link_type, &link.addr, link.slot, 
                             link.channel_offset, 1);

      if (link.link_option == LINK_OPTION_TX){
        /* add flow to the flow table */
        sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot, link.channel_offset, 0);
        /* add NBR and update tx link for it */
        n = tsch_queue_add_nbr(&link.flow_id);
        if(n != NULL) {
          n->tx_links_count++;
          if(!(link.link_option & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count++;
            printf("sdn-handle: install data flow entry 1 \n");
          }
        }  
      }  
    }//for
    sdn_response_for_flow_id(p->payload[CONF_REQUEST_NUM_INDEX], &link.flow_id);
  }//is_last_node

  if(is_middle_node || is_first_node){
    packetbuf_clear();
    packetbuf_copyfrom((uint8_t *)p, len);
    sdn_output(&addr_from, &flow_id_from_controller, 1, NULL);
    printf("sdn-handle: relay open path packet \n");
  }
}
/*---------------------------------------------------------------------------*/
void 
sdn_handle_config_control_plane(struct sdn_packet* p, uint16_t len)
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
         sdn_output(&addr, &flow_id_shared_cell, 1, NULL);
       } else {
         // I must set the real receiver address here
         printf("config is not for novel node\n");
         packetbuf_clear();
         packetbuf_copyfrom((uint8_t *)p, len);
         sdn_output(&addr, &flow_id_from_controller, 1, NULL);
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
       sdn_output(&addr, &flow_id_from_controller, 1, NULL);
       printf("sdn-handle: must look up in flow table \n");
     }

}
/*---------------------------------------------------------------------------*/
void 
sdn_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id)
{
/* this code is for sink node to send config packet for nodes that are not joined to the network */
#if SINK  
  int i;
  struct sdn_packet *p = create_sdn_packet();
  memcpy((uint8_t*)p, buf, len);

  printf("count %u\n", p->typ);
  for (i=0; i<len-1; i++){
    printf("count %u\n", p->payload[i]);
  }
  
  if ((p->typ & 0x0f) == CONFIG_ACK){
   printf("sdn-handle: receive config ack \n");
   handle_config_ack_packet(p);
  }
  
 if ((p->typ & 0x0f) == REQUEST){
   printf("sdn-handle: receive request flow \n");
/*   struct sdn_packet *data_config;
   if(p->payload[1] == 0x03){
     data_config = create_data_confing_node_new(p->payload[REQ_NUM_OF_REQ]);
     sdn_handle_config_packet(data_config, 18, &linkaddr_null);
     packet_deallocate(data_config);
   }*/
 }
 if ((p->typ & 0x0f) == REPORT){
     int need_to_config = 1;
     int is_register = is_registered(p);
     struct sdn_packet_info *created_conf = NULL;
     linkaddr_t conf_flow_id;

   switch(is_register){
     case 1:
       printf("sdn-handle: register node\n");
       need_to_config = 0;
       break;

     case 0:
       printf("sdn-handle: not register node: case 0\n");          
       linkaddr_copy(&conf_flow_id, &flow_id_to_controller);
       break;

     case 2:
       printf("sdn-handle: has to_ctrl, has not from-ctrl: case 2\n");       
       linkaddr_copy(&conf_flow_id, &flow_id_from_controller);
       break;

     case 4:
       printf("sdn-handle: register, no best effort: case 4\n");      
       linkaddr_copy(&conf_flow_id, &flow_id_best_effort);
       break;
   }
   
   if(need_to_config){
     created_conf = sdn_create_confing_packet(p, &conf_flow_id);
     if(created_conf != NULL){
       printf("created conf-best len %u\n", created_conf->len);
       for (i=0; i<created_conf->len -1; i++){
         printf("created config-best %u\n", created_conf->packet.payload[i]);
       }
       sdn_handle_config_packet(&created_conf->packet, created_conf->len, &linkaddr_null);
       print_global_table();
     }  
   }        
 } 
 if((p->typ & 0x0f) == CONFIG){
   sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
 }
 packet_deallocate(p);
#endif

/* process of sdn packets: normal nodes */
#if !SINK
 if(linkaddr_cmp(&flow_id_from_controller, flow_id) || linkaddr_cmp(&flow_id_shared_cell, flow_id)){
   struct sdn_packet *p = create_sdn_packet();
   memcpy((uint8_t*)p, buf, len);
   if((p->typ & 0x0f) == CONFIG){
     sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
   }
   packet_deallocate(p);
 } else if(linkaddr_cmp(&flow_id_to_controller, flow_id) && sdn_is_joined){
   struct sdn_packet *p = create_sdn_packet();
   memcpy((uint8_t*)p, buf, len);
   int num_entry = 0;
   if((p->typ & 0x0f) == CONFIG){
     printf("sdn-handle: receive a data config\n");
     sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
   } else if ((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0){
     printf("sdn-handle: middle node received a report packet\n");
     packetbuf_clear();
     packetbuf_copyfrom((uint8_t *)buf, len);
     sdn_output(&linkaddr_null, flow_id, 1, NULL);
   } else {
     printf("sdn-handle: normal node received a non valid sdn type 0\n");
   }
   packet_deallocate(p);
 } else {
   printf("sdn-handle: normal node received a non valid sdn type\n");
 }
#endif
}
/*---------------------------------------------------------------------------*/
void 
sdn_data_handle(const linkaddr_t *flow_id)
{
#if !SINK
  if(!linkaddr_cmp(&flow_id_best_effort, flow_id)){
    int is_my_app_flow_id = 0;
    if ((is_my_app_flow_id = sdn_is_my_app_flow_id(flow_id)) > 0){
      NETSTACK_NETWORK.input();
    } else {
      int num_entry = 0;
      if ((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0){
        printf("sdn-handle: middle node received a data packet 1\n");
        uint8_t *pkt[127]; 
        uint8_t pkt_size = packetbuf_datalen();
        memcpy((uint8_t *)&pkt, packetbuf_dataptr(), packetbuf_datalen());
        packetbuf_clear();
        packetbuf_copyfrom((uint8_t *)pkt, pkt_size);

        sdn_output(&linkaddr_null, flow_id, 0, NULL);
      }
    }
  } else {
    int num_entry = 0;
    if ((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0){
      printf("sdn-handle: middle node received a data packet\n");
      uint8_t *pkt[127]; 
      uint8_t pkt_size = packetbuf_datalen();
      memcpy((uint8_t *)&pkt, packetbuf_dataptr(), packetbuf_datalen());
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)pkt, pkt_size);

      sdn_output(&linkaddr_null, flow_id, 0, NULL);
    }
  }
#endif

#if SINK
  LOG_INFO("sdn-inout: data ie flowid ");
  LOG_INFO_LLADDR(flow_id);
  LOG_INFO_("\n");
  NETSTACK_NETWORK.input();
#endif
}
























