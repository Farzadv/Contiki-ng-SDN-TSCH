/**
 * \file
 *         Header file for the SDN Contiki porting.
 * \author
 *
 * The SDN module implements SDN in Contiki.
 */

#ifndef SDN_PROJECT_H_
#define SDN_PROJECT_H_

#include "lib/list.h"
#include "lib/memb.h"

void create_initial_schedule_sink(int sf_size, int sh_slot_off, int initial_ch_off);
//void create_initial_schedule_node(eb_info *, int sh_slot_off);


#endif 
