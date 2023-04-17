


#ifndef _SDN_STATIS_H_
#define _SDN_STATIS_H_


#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"

void sdn_cell_used_inti(void);
void print_sf_cell_usage(uint16_t handle);
void print_max_sf_cell_usage(void);
void print_never_used_cell_num(void);
#endif
