
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
#include "net/mac/tsch/sdn/sdn-algo.h"

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_HANDLE_LOG_LEVEL



/*---------------------------------------------------------------------------*/
void 
sdn_handle_config_packet(struct sdn_packet *p, uint16_t len, const linkaddr_t *src_addr)
{
  int is_first_node_in_src_route = 0;
  int is_middle_node_in_src_route = 0;
  int is_last_node_in_src_route = 0;
  int position_in_src_route_list;
  int need_to_send_by_to_ctrl_fid = 0;
  struct tsch_slotframe *sf;
  linkaddr_t config_flow_id;
  linkaddr_t addr;
  linkaddr_t next_hop_addr;
  linkaddr_t prvs_hop_addr;
  int counter = CONF_LIST_OF_NODE_IN_PATH_INDEX;
  int i;
  uint8_t m;
  int repe_period;
  for(i=0; i<p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]; i++) {
    uint8_t j;
    for(j = 0; j< LINKADDR_SIZE; ++j) {
      addr.u8[j] = p->payload[counter];
      counter++;
    }
    LOG_INFO("sdn-handle: config process-> track 1 -> i = %d\n",i);
    if(linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      // first node in list of source routing
      if(i==0) {
        if(linkaddr_cmp(src_addr, &linkaddr_null)) {
          LOG_INFO("sdn-handle: config process-> track 2\n");
          is_first_node_in_src_route = 1;
          position_in_src_route_list = i+1;
          for(j = 0; j< LINKADDR_SIZE; ++j) {
            next_hop_addr.u8[j] = p->payload[counter + j];
          }
          LOG_INFO("sdn-handle: config process-> first node in list\n");
          counter = counter + (((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) -1) * LINKADDR_SIZE);
          break;
        }
      }
      // last node in list of source routing
      else if((i+1)==p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) {
        is_last_node_in_src_route = 1;
        position_in_src_route_list = i+1;
        for(j = 0; j< LINKADDR_SIZE; ++j) {
          prvs_hop_addr.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
         LOG_INFO("sdn-handle: config process-> last node in list\n");
        break;
      }
      // middle node in list of source routing
      else{
        for(j = 0; j< LINKADDR_SIZE; ++j) {
          prvs_hop_addr.u8[j] = p->payload[(counter-(2 *LINKADDR_SIZE)) +j];
        }
        if(linkaddr_cmp(&prvs_hop_addr, src_addr)) {
          is_middle_node_in_src_route = 1;
          position_in_src_route_list = i+1;
          for(j = 0; j< LINKADDR_SIZE; ++j) {
            next_hop_addr.u8[j] = p->payload[counter + j];
          }
           LOG_INFO("sdn-handle: config process-> midlle node in list\n");
          counter = counter + (((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - position_in_src_route_list) * LINKADDR_SIZE);
          
          break;
        }
      }
    }
  }
  
  if((is_first_node_in_src_route || is_middle_node_in_src_route) && ((p->typ & 0xf0)>>4 == 0)) {
    int counter_index = CONF_LIST_OF_NODE_IN_PATH_INDEX;
    linkaddr_t curr_addr;
    for(i=1; i<position_in_src_route_list; i++) {
      uint8_t j;
      for(j = 0; j< LINKADDR_SIZE; ++j) {
        curr_addr.u8[j] = p->payload[counter_index];
        counter_index++;
      }
      if(linkaddr_cmp(&curr_addr, &next_hop_addr)) {
        need_to_send_by_to_ctrl_fid ++;
      }
    }
  }
  
  if((is_first_node_in_src_route || is_middle_node_in_src_route) && ((p->typ & 0xf0)>>4 == 1)) {
    int flow_id_index = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
    config_flow_id.u8[0] = p->payload[flow_id_index];
    config_flow_id.u8[1] = p->payload[flow_id_index + 1];
  }
  
  if(is_first_node_in_src_route) {
    int flow_id_index;
    int sf_id_index;
    uint16_t repe_num;
    struct sdn_link link;
    
    int next_num_cell_per_hop_install = (p->payload[counter+(position_in_src_route_list - 1)]) & 0x0f;
    int next_num_cell_per_hop_uninstall = ((p->payload[counter+(position_in_src_route_list - 1)]) & 0xf0) >> 4;
    
    int uninstall_cell_index = counter + (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) + 3; // 3 is offset of flow-id and SF
    
    
    /* specify install cell index */
    int x;
    int sum_lapsed_cell_uninstall_tot = 0;
    for(x=0; x < p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1; x++) {
      sum_lapsed_cell_uninstall_tot = ((p->payload[counter + x] & 0xf0) >> 4) + sum_lapsed_cell_uninstall_tot;
    }
    int install_cell_index = uninstall_cell_index + (sum_lapsed_cell_uninstall_tot * CONFIG_CELL_SIZE);
    
    
    
    
    if(next_num_cell_per_hop_install > 0 || next_num_cell_per_hop_uninstall > 0) {
      
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
      if(link.sf == 0) {
        sf = sf0;
      }
      else if(link.sf == 1) {
        sf = sf1;
      } 
      else{
        sf = sf0;
      }
      
      repe_period = p->payload[CONF_REPETION_PERIOD + 1];
      repe_period = (repe_period <<8) + p->payload[CONF_REPETION_PERIOD];
      if(repe_period<1) {
        repe_period = SDN_DATA_SLOTFRAME_SIZE;
      }
   
      repe_num = SDN_DATA_SLOTFRAME_SIZE/repe_period;
      
      for(m=0; m<repe_num; m++) {  
         
        /* uninstall cells */
        counter = uninstall_cell_index;
        for(i=0; i<next_num_cell_per_hop_uninstall; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
          link.channel_offset = p->payload[counter]; 
          counter++;
    
          tsch_schedule_remove_link_by_timeslot(sf, link.slot + (m * repe_period), link.channel_offset);

        }
        
        /* install cells */
        counter = install_cell_index;
        for(i=0; i<next_num_cell_per_hop_install; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
          link.channel_offset = p->payload[counter]; 
          counter++;
    
          tsch_schedule_add_link(sf,
                                 LINK_OPTION_RX,
	                         LINK_TYPE_NORMAL, &link.addr, link.slot + (m * repe_period), 
                                 link.channel_offset, 1);  
        }
      }
      
      printf("install rx cells: node[%d%d], flow id[%d], num cell[%d], asn[0x%x]]\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
            flow_id.u8[0], (repe_num * next_num_cell_per_hop_install), tsch_current_asn.ls4b);
      
      //tsch_schedule_print();
    } else {
      LOG_INFO("sdn-handle: first node in list->only forward \n");
    }
  }

  if(is_last_node_in_src_route) {
    struct tsch_neighbor *n;
    int flow_id_index;
    int sf_id_index;
    static struct sdn_link link;
    int sum_lapsed_cell_install = 0;
    int sum_lapsed_cell_uninstall = 0;
    uint16_t repe_num;
    
    int prvs_num_cell_per_hop_install = p->payload[counter+(position_in_src_route_list - 2)] & 0x0f;
    int prvs_num_cell_per_hop_uninstall = (p->payload[counter + (position_in_src_route_list - 2)] & 0xf0) >> 4;
    
    int uninstall_cell_index = counter + (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) + 3; // 3 is offset of flow-id and SF
    
    /* specify install cell index */
    int x;
    int sum_lapsed_cell_uninstall_tot = 0;
    for(x=0; x < p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1; x++) {
      sum_lapsed_cell_uninstall_tot = ((p->payload[counter + x] & 0xf0) >> 4) + sum_lapsed_cell_uninstall_tot;
    }
    int install_cell_index = uninstall_cell_index + (sum_lapsed_cell_uninstall_tot * CONFIG_CELL_SIZE);
    
    
    if(prvs_num_cell_per_hop_install > 0 || prvs_num_cell_per_hop_uninstall > 0) {
    
      for(i=0; i<(position_in_src_route_list - 2); i++) {
        sum_lapsed_cell_install = (p->payload[counter+i] & 0x0f) + sum_lapsed_cell_install;
        sum_lapsed_cell_uninstall = ((p->payload[counter+i] & 0xf0) >> 4) + sum_lapsed_cell_uninstall;
      }
      
      
      
      counter = counter + ((p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX]) - 1);
      //printf("check 1: sum_lapsed_cell:%d, counter: %d , pcounter:%d\n", sum_lapsed_cell, counter, p->payload[counter]);
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
      if(link.sf == 0) {
        sf = sf0;
      }
      else if(link.sf == 1) {
        sf = sf1;
      } 
      else{
        sf = sf0;
      }
      link.link_option = LINK_OPTION_TX;
      
      
      repe_period = p->payload[CONF_REPETION_PERIOD + 1];
      repe_period = (repe_period <<8) + p->payload[CONF_REPETION_PERIOD];
      if(repe_period<1) {
        repe_period = SDN_DATA_SLOTFRAME_SIZE;
      }
      repe_num = SDN_DATA_SLOTFRAME_SIZE/repe_period;
      //LOG_INFO("sdn-handle: config repe_period: %d, repe_num:%d \n", repe_period, repe_num);
      for(m=0; m<repe_num; m++) {
        
        /* uinstall cells */
        counter = uninstall_cell_index + (sum_lapsed_cell_uninstall * CONFIG_CELL_SIZE);
        for(i=0; i<prvs_num_cell_per_hop_uninstall; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
          link.channel_offset = p->payload[counter]; 
          counter++;
          
          /* remove flow cells from the scheduling table */
          tsch_schedule_remove_link_by_timeslot(sf, link.slot + (m * repe_period), link.channel_offset);
          
          /* remove flow-id from the flow table */
          sdn_remove_flow_entry(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);  // set priority to 0
        
        }
        
        /* install cells */
        counter = install_cell_index + (sum_lapsed_cell_install * CONFIG_CELL_SIZE);
        for(i=0; i<prvs_num_cell_per_hop_install; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
          link.channel_offset = p->payload[counter]; 
          counter++;
          //LOG_INFO("sdn-handle: config slot: %d, m:%d \n", link.slot, m);
          tsch_schedule_add_link(sf,
                                 LINK_OPTION_TX,
	                         LINK_TYPE_NORMAL, &link.addr, link.slot + (m * repe_period), 
                                 link.channel_offset, 1);
       

          if(sdn__sf_offset_to_send_eb == -1 && linkaddr_cmp(&link.flow_id, &flow_id_to_controller)) {
	    sdn__sf_offset_to_send_eb = p->payload[CONF_EB_SF_OFFS_INDEX + 1];
	    sdn__sf_offset_to_send_eb = (sdn__sf_offset_to_send_eb <<8) + p->payload[CONF_EB_SF_OFFS_INDEX];
	    sdn__ts_offset_to_send_eb = p->payload[CONF_EB_TS_OFFS_INDEX + 1];
	    sdn__ts_offset_to_send_eb = (sdn__ts_offset_to_send_eb <<8) + p->payload[CONF_EB_TS_OFFS_INDEX];
	    //printf("node sf off: %d %d\n", sdn__sf_offset_to_send_eb, sdn__ts_offset_to_send_eb);
          }
          
          int sf_offs = get_sf_offset_from_ts(sdn__ts_offset_to_send_eb);
          if(sf_offs != -1 && sf_offs > sf0->max_sf_offs) {
            sf0->max_sf_offs = sf_offs;
            sdn_max_used_sf_offs = sf_offs;
            //printf("update sf offs: %d\n", sf_offs);
          }
          
          
          if (link.link_option == LINK_OPTION_TX) {
          /* add flow to the flow table */
          sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);  // set priority to 0
          /* add NBR and update tx link for it */
          n = tsch_queue_add_nbr(&link.flow_id);
            if(n != NULL) {
              n->tx_links_count++;
              if(!(link.link_option & LINK_OPTION_SHARED)) {
                n->dedicated_tx_links_count++;
                //LOG_INFO("sdn-handle: install data flow entry 1 \n");
              }
            }  
          }  
        
          if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)) {
	    sdn_associate_choff_to_nbr(&link.addr, link.channel_offset);
	    //LOG_INFO("sdn-handle: assoc ch_off to nbr addr \n");
          }
        
        }//for
      }
      if(p->payload[CONF_REQUEST_NUM_INDEX] > 0) {
        sdn_response_for_flow_id(p->payload[CONF_REQUEST_NUM_INDEX], &link.flow_id, repe_period);
      }
      //tsch_schedule_print();
    }	  
  }

  if(is_middle_node_in_src_route) {
    struct tsch_neighbor *n;
    int flow_id_index;
    int sf_id_index;
    struct sdn_link link;
    uint16_t repe_num;
    
    int sum_lapsed_cell_install_prvs = 0;
    int sum_lapsed_cell_uninstall_prvs = 0;
    int sum_lapsed_cell_install_next = 0;
    int sum_lapsed_cell_uninstall_next = 0;
    
    int prvs_num_cell_per_hop_install = (p->payload[counter+(position_in_src_route_list - 2)]) & 0x0f;
    int prvs_num_cell_per_hop_uninstall = ((p->payload[counter+(position_in_src_route_list - 2)]) & 0xf0) >> 4;
    
    int next_num_cell_per_hop_install = (p->payload[counter+(position_in_src_route_list - 1)]) & 0x0f;
    int next_num_cell_per_hop_uninstall = ((p->payload[counter+(position_in_src_route_list - 1)]) & 0xf0) >> 4;
    
    
    int uninstall_cell_index = counter + (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) + 3; // 3 is offset of flow-id and SF
    
    /* specify install cell index */
    int x;
    int sum_lapsed_cell_uninstall_tot = 0;
    for(x=0; x < p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1; x++) {
      sum_lapsed_cell_uninstall_tot = ((p->payload[counter + x] & 0xf0) >> 4) + sum_lapsed_cell_uninstall_tot;
    }
    int install_cell_index = uninstall_cell_index + (sum_lapsed_cell_uninstall_tot * CONFIG_CELL_SIZE);
    
    for(i=0; i<(position_in_src_route_list - 2); i++) {
      sum_lapsed_cell_install_prvs = (p->payload[counter+i] & 0x0f) + sum_lapsed_cell_install_prvs;
      sum_lapsed_cell_uninstall_prvs = ((p->payload[counter+i] & 0xf0) >> 4) + sum_lapsed_cell_uninstall_prvs;
    }
    
    for(i=0; i<(position_in_src_route_list - 1); i++) {
      sum_lapsed_cell_install_next = (p->payload[counter+i] & 0x0f) + sum_lapsed_cell_install_next;
      sum_lapsed_cell_uninstall_next = ((p->payload[counter+i] & 0xf0) >> 4) + sum_lapsed_cell_uninstall_next;
    }
      
    if((prvs_num_cell_per_hop_install > 0 || prvs_num_cell_per_hop_uninstall > 0)  && (next_num_cell_per_hop_install > 0 || next_num_cell_per_hop_uninstall > 0)) {

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
      if(link.sf == 0) {
        sf = sf0;
      }
      else if(link.sf == 1) {
        sf = sf1;
      } 
      else{
        sf = sf0;
      }
      
      repe_period = p->payload[CONF_REPETION_PERIOD + 1];
      repe_period = (repe_period <<8) + p->payload[CONF_REPETION_PERIOD];
      if(repe_period<1) {
        repe_period = SDN_DATA_SLOTFRAME_SIZE;
      }

      repe_num = SDN_DATA_SLOTFRAME_SIZE/repe_period;
      
      for(m=0; m<repe_num; m++) {
      
        /* uinstall cells */
        counter = uninstall_cell_index + (sum_lapsed_cell_uninstall_prvs * CONFIG_CELL_SIZE);
        for(i=0; i<prvs_num_cell_per_hop_uninstall; i++) {
          link.link_option = LINK_OPTION_TX;
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
	  counter++;
	  
	  /* remove flow cells from the scheduling table */
          tsch_schedule_remove_link_by_timeslot(sf, link.slot + (m * repe_period), link.channel_offset);    
		               
          /* remove flow-id from the flow table */
          sdn_remove_flow_entry(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);  // set priority to 0  
        }
      
        /* install cells */
        counter = install_cell_index + (sum_lapsed_cell_install_prvs * CONFIG_CELL_SIZE);
        for(i=0; i<prvs_num_cell_per_hop_install; i++) {
          link.link_option = LINK_OPTION_TX;
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
	  counter++;
		    
	  tsch_schedule_add_link(sf,
		               LINK_OPTION_TX,
			       LINK_TYPE_NORMAL, &link.addr, link.slot + (m * repe_period), 
	     	               link.channel_offset, 1);
		               
          if(link.link_option == LINK_OPTION_TX) {
            /* add flow to the flow table */
	    sdn_add_flow_enrty(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);
	    /* add NBR and update tx link for it */
	    n = tsch_queue_add_nbr(&link.flow_id);
	    if(n != NULL) {
	      n->tx_links_count++;
	      if(!(link.link_option & LINK_OPTION_SHARED)) {
	        n->dedicated_tx_links_count++;
                //LOG_INFO("sdn-handle: install data flow entry 1 \n");
              }
            }  
          }  
        }
      }
      //tsch_schedule_print();
      // next hop config
      linkaddr_copy(&link.addr, &next_hop_addr);
      
      for(m=0; m<repe_num; m++) {
       
        /* uninstall cells */
        counter = uninstall_cell_index + (sum_lapsed_cell_uninstall_next * CONFIG_CELL_SIZE);
        for(i=0; i<next_num_cell_per_hop_uninstall; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
	  counter++;
	  
	  /* remove flow cells from the scheduling table */
          tsch_schedule_remove_link_by_timeslot(sf, link.slot + (m * repe_period), link.channel_offset);
          
          /* remove flow-id from the flow table */
          //sdn_remove_flow_entry(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);  // set priority to 0
	                       
        }
      
        /* install cells */    
        counter = install_cell_index + (sum_lapsed_cell_install_next * CONFIG_CELL_SIZE);
        for(i=0; i<next_num_cell_per_hop_install; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
	  counter++;
	    
	  tsch_schedule_add_link(sf,
	                       LINK_OPTION_RX,
		               LINK_TYPE_NORMAL, &link.addr, link.slot + (m * repe_period), 
	                       link.channel_offset, 1); 
	                       
	  int fid_to_exsit = sdn_is_flowid_exist_in_table(&flow_id_to_controller);             
	  if(!sdn_is_joined && fid_to_exsit > 0 && linkaddr_cmp(&link.flow_id, &flow_id_from_controller)) {
	    sdn_is_joined = 1;
	    tsch_queue_update_time_source(&link.addr);
	    printf("sdn-handle: fix time source as SDN parent \n");
          } else if (!sdn_is_joined && num_from_controller_rx_slots > 0 && linkaddr_cmp(&link.flow_id, &flow_id_to_controller)) {
            sdn_is_joined = 1;
            tsch_queue_update_time_source(&link.addr);
            printf("sdn-handle: fix time source as SDN parent \n");
          }
        
	  //count slots coresspond to from_controller flow id
          if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)) {
	    num_from_controller_rx_slots++;
	    LOG_INFO("sdn-handle: add number of from_ctrl fid \n");
          }  
        }
      }
      
      printf("install rx cells: node[%d%d], flow id[%d], num cell[%d], asn[0x%x]]\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
            flow_id.u8[0], (repe_num * next_num_cell_per_hop_install), tsch_current_asn.ls4b);
      
      //tsch_schedule_print();
    } else if(prvs_num_cell_per_hop_install == 0 && (next_num_cell_per_hop_install > 0 || next_num_cell_per_hop_uninstall > 0)) {

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
      if(link.sf == 0) {
        sf = sf0;
      }
      else if(link.sf == 1) {
        sf = sf1;
      } 
      else{
        sf = sf0;
      }
      
      repe_period = p->payload[CONF_REPETION_PERIOD + 1];
      repe_period = (repe_period <<8) + p->payload[CONF_REPETION_PERIOD];
      
      if(repe_period<1) {
        repe_period = SDN_DATA_SLOTFRAME_SIZE;
      }

      repe_num = SDN_DATA_SLOTFRAME_SIZE/repe_period;

      for(m=0; m<repe_num; m++) {
        /* uinstall cells */
        counter = uninstall_cell_index + (sum_lapsed_cell_uninstall_next * CONFIG_CELL_SIZE);
        for(i=0; i<next_num_cell_per_hop_uninstall; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
          counter++;
	    
	  /* remove flow cells from the scheduling table */
          tsch_schedule_remove_link_by_timeslot(sf, link.slot + (m * repe_period), link.channel_offset);
          
          /* remove flow-id from the flow table */
          //sdn_remove_flow_entry(&link.flow_id, link.sf, link.slot + (m * repe_period), link.channel_offset, 0);  // set priority to 0
	                       
        }
        
        /* install cells */ 
        counter = install_cell_index + (sum_lapsed_cell_install_next * CONFIG_CELL_SIZE);
        for(i=0; i<next_num_cell_per_hop_install; i++) {
          link.slot = p->payload[counter + 1];
          link.slot = (link.slot <<8) + p->payload[counter];
          counter = counter + 2;
	  link.channel_offset = p->payload[counter]; 
          counter++;
	    
	  tsch_schedule_add_link(sf,
	                       LINK_OPTION_RX,
		               LINK_TYPE_NORMAL, &link.addr, link.slot + (m * repe_period), 
	                       link.channel_offset, 1);  
	                       
	  int fid_to_exsit = sdn_is_flowid_exist_in_table(&flow_id_to_controller);             
	  if(!sdn_is_joined && fid_to_exsit > 0 && linkaddr_cmp(&link.flow_id, &flow_id_from_controller)) {
	    sdn_is_joined = 1;
	    tsch_queue_update_time_source(&link.addr);
	    printf("sdn-handle: fix time source as SDN parent \n");
          } else if (!sdn_is_joined && num_from_controller_rx_slots > 0 && linkaddr_cmp(&link.flow_id, &flow_id_to_controller)) {
            sdn_is_joined = 1;
            tsch_queue_update_time_source(&link.addr);
            printf("sdn-handle: fix time source as SDN parent \n");
          } 
        
	   //count slots coresspond to from_controller flow id
          if(linkaddr_cmp(&link.flow_id, &flow_id_from_controller)) {
	    num_from_controller_rx_slots++;
	    LOG_INFO("sdn-handle: add number of from_ctrl fid \n");
          } 

        }
      }
      
      printf("install rx cells: node[%d%d], flow id[%d], num cell[%d], asn[0x%x]]\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
            flow_id.u8[0], (repe_num * next_num_cell_per_hop_install), tsch_current_asn.ls4b);
      
      //tsch_schedule_print();
    } else {
      LOG_ERR("sdn-handle: no next nor prvs config \n");
    }		    
  }
  
  // relay the Config packet
  int fid_to_ctrl_exsit;
  if(((p->typ & 0xf0)>>4 == 1) && (is_first_node_in_src_route || is_middle_node_in_src_route)) {
    if(linkaddr_cmp(&config_flow_id, &flow_id_to_controller) && 
      (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) == position_in_src_route_list) {
      LOG_INFO("config for novel node: fid->to_ctrl, befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_shared_cell, 1, NULL);
      
      
    } else if(linkaddr_cmp(&config_flow_id, &flow_id_from_controller) && 
             (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 2) == position_in_src_route_list) {
      LOG_INFO("config for novel node: fid->from_ctrl, 2 befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_shared_cell, 1, NULL);
      
      
    } else if(linkaddr_cmp(&config_flow_id, &flow_id_from_controller) && 
             (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) == position_in_src_route_list &&
             (fid_to_ctrl_exsit = sdn_is_flowid_exist_in_table(&flow_id_to_controller)) > 0) {
             
      LOG_INFO("config for novel node: fid->from_ctrl, 1 befor last node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_to_controller, 1, NULL);
      
      
    } else if(linkaddr_cmp(&config_flow_id, &flow_id_from_controller) && 
             (p->payload[CONF_NUM_SOURCE_ROUTING_NODE_INDEX] - 1) == position_in_src_route_list &&
             (fid_to_ctrl_exsit = sdn_is_flowid_exist_in_table(&flow_id_to_controller)) == 0) {
             
      LOG_INFO("config for novel node: fid->from_ctrl, 1 befor last node: no to-cont so handle on shar\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_shared_cell, 1, NULL);  
      
        
    } else{
      LOG_INFO("config for novel node: fid->from_ctrl, relay middle node\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_from_controller, 1, NULL);
    }  
    
    
  } else if(((p->typ & 0xf0)>>4 == 0) && (is_first_node_in_src_route || is_middle_node_in_src_route)) {
    if(need_to_send_by_to_ctrl_fid > 0) {
      LOG_INFO("config for join node: fid->to_ctrl\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_to_controller, 1, NULL);
    } else{
      LOG_INFO("config for join node: fid->from_ctrl\n");
      packetbuf_clear();
      packetbuf_copyfrom((uint8_t *)p, len);
      sdn_output(&next_hop_addr, &flow_id_from_controller, 1, NULL);
    }
  } else{
    LOG_INFO("confuse to relay config packet\n");
  }

 
  if(is_last_node_in_src_route) {
    LOG_INFO("sdn-handle: last node -> send ack \n");
    send_config_ack(p->payload[CONF_SEQ_NUM_INDEX]);
  }

}
/*---------------------------------------------------------------------------*/
int 
send_config_ack(int seq_num)
{
  struct sdn_packet *ack = create_sdn_packet();
  if(ack != NULL) {
    uint8_t j;
    int len = 0;
    ack->typ = CONFIG_ACK;
    len++;
    
    for(j = 0; j<LINKADDR_SIZE; ++j) {
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
sdn_control_packet_handle(const uint8_t *buf, uint16_t len, const linkaddr_t *flow_id)
{
/* this code is for sink node to send config packet for nodes that are not joined to the network */
#if SINK  
 if(buf != NULL && flow_id != NULL && len > 0 && len < 115) {
   sdn_send_packet_to_controller(buf, len, flow_id);
 }
#endif

/* process of sdn packets: normal nodes */
#if !SINK
 if(linkaddr_cmp(&flow_id_from_controller, flow_id) || linkaddr_cmp(&flow_id_shared_cell, flow_id)) {
   struct sdn_packet *p = create_sdn_packet();
   memcpy((uint8_t*)p, buf, len);
   if((p->typ & 0x0f) == CONFIG) {
     sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
   }
   packet_deallocate(p);
 } else if(linkaddr_cmp(&flow_id_to_controller, flow_id) && sdn_is_joined) {
   struct sdn_packet *p = create_sdn_packet();
   memcpy((uint8_t*)p, buf, len);
   int num_entry = 0;
   if((p->typ & 0x0f) == CONFIG) {
     LOG_INFO("sdn-handle: receive a data config\n");
     sdn_handle_config_packet(p, len, packetbuf_addr(PACKETBUF_ADDR_SENDER));
   } else if ((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0) {
     LOG_INFO("sdn-handle: middle node received a report packet\n");
     packetbuf_clear();
     packetbuf_copyfrom((uint8_t *)p, len);
     sdn_output(&linkaddr_null, flow_id, 1, NULL);
   } else {
     LOG_INFO("sdn-handle: normal node received a non valid sdn type 0\n");
   }
   packet_deallocate(p);
 } else {
   LOG_INFO("sdn-handle: normal node received a non valid sdn type\n");
 }
#endif
}
/*---------------------------------------------------------------------------*/
void 
sdn_data_packet_handle(const linkaddr_t *flow_id)
{
  if(!linkaddr_cmp(&flow_id_best_effort, flow_id)) {
    int is_my_app_flow_id = 0;
    if((is_my_app_flow_id = sdn_is_flowid_exist_in_table(flow_id)) == 0) {
      LOG_INFO("sdn-handle: dest of packet -> pass to app\n");
      NETSTACK_NETWORK.input();
    } else {
      int num_entry = 0;
      if((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0) {
        LOG_INFO("sdn-handle: middle node received a data packet 1\n");
        
        int i;
        uint8_t *pkt[127]; 
        uint8_t p[127];
        int p_index = packetbuf_datalen() - 4;  // subtract ASN part from end of packet
        uint8_t pkt_size = packetbuf_datalen();
        uint16_t app_seqno;
        linkaddr_t app_src;
        linkaddr_t app_dest;
        
        memcpy((uint8_t *)&pkt, packetbuf_dataptr(), packetbuf_datalen());
        memcpy((uint8_t *)&p, packetbuf_dataptr(), packetbuf_datalen());
        packetbuf_clear();
        packetbuf_copyfrom((uint8_t *)pkt, pkt_size);
        
        
        /******** extract app packet info for statistics *******/        
        /*LOG_INFO("sdn-handle: data fid cont\n");
        for(i=0; i<pkt_size; i++) {
          LOG_INFO("%d \n", p[i]);
        }*/
        
        //index of app seq num
        p_index = p_index-2;           
        app_seqno = (uint8_t)p[p_index] |
                    (uint8_t)p[p_index+1]<<8;
        
        //app dest addr
        p_index = p_index - LINKADDR_SIZE;
        
        for(i = 0; i< LINKADDR_SIZE; ++i){
          app_dest.u8[i] = (uint8_t)p[p_index + i];
        }
        
        //app src addr
        p_index = p_index - LINKADDR_SIZE;
        
        for(i = 0; i< LINKADDR_SIZE; ++i){
          app_src.u8[i] = (uint8_t)p[p_index + i];
        }
        
        packetbuf_set_addr(PACKETBUF_ADDR_APP_SENDER, &app_src);
        packetbuf_set_addr(PACKETBUF_ADDR_APP_RECEIVER, &app_dest);
        packetbuf_set_attr(PACKETBUF_ATTR_APP_SEQNO, app_seqno);
        /***************************************************/
        
        sdn_output(&linkaddr_null, flow_id, 0, NULL);        
      }
    }
  } else {
#if SINK
    LOG_INFO("sdn-inout: data ie flowid ");
    LOG_INFO_LLADDR(flow_id);
    LOG_INFO_("\n");
    NETSTACK_NETWORK.input();
#else 
    int num_entry = 0;
    if((num_entry = sdn_is_flowid_exist_in_table(flow_id)) > 0) {
      LOG_INFO("sdn-handle: middle node received a data packet 1\n");
        
        int i;
        uint8_t *pkt[127]; 
        uint8_t p[127];
        int p_index = packetbuf_datalen() - 4;  // subtract ASN part from end of packet
        uint8_t pkt_size = packetbuf_datalen();
        uint16_t app_seqno;
        linkaddr_t app_src;
        linkaddr_t app_dest;
        
        memcpy((uint8_t *)&pkt, packetbuf_dataptr(), packetbuf_datalen());
        memcpy((uint8_t *)&p, packetbuf_dataptr(), packetbuf_datalen());
        packetbuf_clear();
        packetbuf_copyfrom((uint8_t *)pkt, pkt_size);
        
        
        /******** extract app packet info for statistics *******/        
        /*LOG_INFO("sdn-handle: data fid cont\n");
        for(i=0; i<pkt_size; i++) {
          LOG_INFO("%d \n", p[i]);
        }*/
        
        //index of app seq num
        p_index = p_index-2;           
        app_seqno = (uint8_t)p[p_index] |
                    (uint8_t)p[p_index+1]<<8;
        
        //app dest addr
        p_index = p_index - LINKADDR_SIZE;
        
        for(i = 0; i< LINKADDR_SIZE; ++i){
          app_dest.u8[i] = (uint8_t)p[p_index + i];
        }
        
        //app src addr
        p_index = p_index - LINKADDR_SIZE;
        
        for(i = 0; i< LINKADDR_SIZE; ++i){
          app_src.u8[i] = (uint8_t)p[p_index + i];
        }
        
        packetbuf_set_addr(PACKETBUF_ADDR_APP_SENDER, &app_src);
        packetbuf_set_addr(PACKETBUF_ADDR_APP_RECEIVER, &app_dest);
        packetbuf_set_attr(PACKETBUF_ATTR_APP_SEQNO, app_seqno);
        /***************************************************/
        
        sdn_output(&linkaddr_null, flow_id, 0, NULL);
    }
#endif
  }
}
/*---------------------------------------------------------------------------*/
void
sdn_input_packet_statistic(const linkaddr_t *flow_id)
{
  if(flow_id == NULL) {
    return;
  }
  
  if(!linkaddr_cmp(&flow_id_best_effort, flow_id)) {       
    int i; 
    uint8_t p[127];
    int p_index = packetbuf_datalen() - 4;  // subtract ASN part from end of packet
    uint16_t app_seqno;
    linkaddr_t app_src;
    linkaddr_t app_dest;

    const linkaddr_t *mac_src = packetbuf_addr(PACKETBUF_ADDR_SENDER);
    const linkaddr_t *mac_dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
        
    memcpy((uint8_t *)&p, packetbuf_dataptr(), packetbuf_datalen());
             
    /******** extract app packet info for statistics *******/        
    /*LOG_INFO("sdn-handle: data fid cont\n");
    for(i=0; i<pkt_size; i++) {
      LOG_INFO("%d \n", p[i]);
    }*/
        
    //index of app seq num
    p_index = p_index-2;           
    app_seqno = (uint8_t)p[p_index] |
                  (uint8_t)p[p_index+1]<<8;
        
    //app dest addr
    p_index = p_index - LINKADDR_SIZE;
        
    for(i = 0; i< LINKADDR_SIZE; ++i){
      app_dest.u8[i] = (uint8_t)p[p_index + i];
    }
        
    //app src addr
    p_index = p_index - LINKADDR_SIZE;
        
    for(i = 0; i< LINKADDR_SIZE; ++i){
      app_src.u8[i] = (uint8_t)p[p_index + i];
    }
        
    LOG_WARN("!!RX!! |t%d%d|r%d%d|f%d%d|s%d%d|d%d%d|n%d|j\n", mac_src->u8[0], mac_src->u8[1], mac_dest->u8[0], mac_dest->u8[1],
                                                              flow_id->u8[0], flow_id->u8[1], app_src.u8[0], app_src.u8[1], 
                                                              app_dest.u8[0], app_dest.u8[1], app_seqno);
  }
}
/*---------------------------------------------------------------------------*/









