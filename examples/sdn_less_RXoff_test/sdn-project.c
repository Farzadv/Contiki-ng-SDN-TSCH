/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
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

/**
 * \file
 *         NullNet unicast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (1 * CLOCK_SECOND)

#if LINKADDR_SIZE == 8
static linkaddr_t dest_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static linkaddr_t dest_addr2 =         {{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static linkaddr_t dest_addr3 =         {{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#else
static linkaddr_t dest_addr =         {{ 0x01, 0x00 }};
static linkaddr_t dest_addr2 =         {{ 0x02, 0x00 }};
static linkaddr_t dest_addr3 =         {{ 0x03, 0x00 }};
#endif

#if MAC_CONF_WITH_TSCH
#include "sdn-project.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#if LINKADDR_SIZE == 8
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#else
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00 }};
#endif

#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  if(len == sizeof(unsigned)) {
    unsigned count;
 //   int i;
//    for(i=0; i<2; i++){
  //    printf("this is for testing the slot off \n");
    //}
    if(linkaddr_cmp(&dest_addr2, &linkaddr_node_addr)){
      NETSTACK_NETWORK.output(&dest_addr);
    }
    memcpy(&count, data, sizeof(count));
 //   LOG_INFO("Received %u from ", count);
  //  LOG_INFO_LLADDR(src);
    //LOG_INFO_("\n");
  }
}
/*----------------------------------veisi-------------------------------------*/
  void
  create_initial_schedule_sink(int sf_size, int sh_slot_num, int initial_ch_off)
  {
/*
this function initialize the slotframe by getting the initial values from controller (if SINK) 
or from a neighbor throgh EB
this function lets a novel node to gets initial schedule and send its request to a joined neighbor
this sets the first timeslot as the shared slot.

- controller must specify the number and position of shared slots in ths slotframe
Notice: novel node just is able to detect the first timeslot of slotframe. may there are other 
shared slots over the slotframe but novel cannot be informed about their position in the slotframe

*/

    // struct tsch_slotframe *sf;
    tsch_schedule_remove_all_slotframes();
    //we consider slotframe_handle = 0
    int sf_handle = 0;
    sf0 = tsch_schedule_add_slotframe(sf_handle, sf_size);
    int i;
    int sh_slot_indx = (int)(sf_size/sh_slot_num);

    int j = 0;
    for (i=0 ; i<sh_slot_num; i++){

	    tsch_schedule_add_link(sf0,
	    LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING,
	    LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
	    j, initial_ch_off, flow_id_shared_cell, 1);


      j = sh_slot_indx + j;
    }
      if(linkaddr_cmp(&dest_addr, &linkaddr_node_addr)){
	    tsch_schedule_add_link(sf0,
	    LINK_OPTION_RX,
	    LINK_TYPE_NORMAL, &dest_addr,
	    4, initial_ch_off, flow_id_shared_cell, 1);
      }
      if(linkaddr_cmp(&dest_addr2, &linkaddr_node_addr)){
	    tsch_schedule_add_link(sf0,
	    LINK_OPTION_TX,
	    LINK_TYPE_NORMAL, &dest_addr,
	    4, initial_ch_off, flow_id_shared_cell, 1);
      }
      if(linkaddr_cmp(&dest_addr2, &linkaddr_node_addr)){
	    tsch_schedule_add_link(sf0,
	    LINK_OPTION_RX,
	    LINK_TYPE_NORMAL, &dest_addr2,
	    3, initial_ch_off, flow_id_shared_cell, 1);
      }
      if(linkaddr_cmp(&dest_addr3, &linkaddr_node_addr)){
	    tsch_schedule_add_link(sf0,
	    LINK_OPTION_TX,
	    LINK_TYPE_NORMAL, &dest_addr2,
	    3, initial_ch_off, flow_id_shared_cell, 1);
      }
  }
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */




/* if SINK -> get the initial schedule frome controller*/
/*
TODO the 3 following values must be set by controller. Sink node must get these values at the initialization time 
  controller must specify the number and position of shared slots in ths slotframe
*/
	int slotframe_size = 10;
	int shared_slot_num = 2; 
	int initial_ch_offset = 0;

	create_initial_schedule_sink(slotframe_size, shared_slot_num, initial_ch_offset);
        NETSTACK_MAC.on();



  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

  if(!linkaddr_cmp(&dest_addr, &linkaddr_node_addr)) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      while (!tsch_is_associated){
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      }
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      LOG_INFO("Sending %u to ", count);
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");
      if(linkaddr_cmp(&dest_addr2, &linkaddr_node_addr)){
        NETSTACK_NETWORK.output(&dest_addr);
      }
      if(linkaddr_cmp(&dest_addr3, &linkaddr_node_addr)){
        NETSTACK_NETWORK.output(&dest_addr2);
      }

      count++;
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
