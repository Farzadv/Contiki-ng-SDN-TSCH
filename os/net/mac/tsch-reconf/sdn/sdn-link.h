


#ifndef _SDN_LINK_H_
#define _SDN_LINK_H_


#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"

void sdn_link_init();
void sdn_update_shared_cell_usage(const int sf_off, const int timeslot);
int sdn_is_shared_cell_occupied(const int sf_off, const int timeslot);
#endif
