


#ifndef _SDN_PACKET_H_
#define _SDN_PACKET_H_




#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"

struct sdn_packet * create_sdn_packet(void);
void packet_deallocate( struct sdn_packet*);


#endif