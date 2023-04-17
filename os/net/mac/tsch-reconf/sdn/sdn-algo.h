


#ifndef _SDN_ALGO_H_
#define _SDN_ALGO_H_


#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"

struct list_dist_uniform * sdn_distribute_list_uniform(struct list_dist_uniform *req_list);
int get_sf_offset_from_ts(int timeslot);
#endif
