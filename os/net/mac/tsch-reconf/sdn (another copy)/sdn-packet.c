
#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "lib/memb.h"
#include <stdio.h>

/* memory allocation to SDN packets */
#define SDN_PACKET_MEMB_SIZE 100
MEMB(sdn_packet_memb, struct sdn_packet, SDN_PACKET_MEMB_SIZE);
/*----------------------------------------------------------------------*/
/* create sdn packet */
struct sdn_packet* 
create_sdn_packet(void){
  struct sdn_packet *p = NULL;
  p = memb_alloc(&sdn_packet_memb);
  return p;
}
/*----------------------------------------------------------------------*/
/* deallocate memory to created sdn packet */
void 
packet_deallocate(struct sdn_packet* p)
{
  if(p != NULL) {
    memb_free(&sdn_packet_memb, p);
  }
}
/*----------------------------------------------------------------------*/
void 
sdn_packet_init(void)
{ 
  memb_init(&sdn_packet_memb);
}
/*----------------------------------------------------------------------*/


