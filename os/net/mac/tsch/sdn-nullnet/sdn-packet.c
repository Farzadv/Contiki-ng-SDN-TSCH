








#include "contiki.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "lib/memb.h"
#include <stdio.h>




/* memory allocation for SDN packets */
#define MEMB_SIZE 10
MEMB(packet_memb, struct sdn_packet, MEMB_SIZE);

struct sdn_packet * a;
/* ---------------------------------------------------------- */ 
static struct sdn_packet * packet_allocate(void);

/*----------------------------------------------------------------------*/
/* create report packet */
struct sdn_packet* 
create_sdn_packet(void){
  struct sdn_packet *p = packet_allocate();
  return p;
}
/*----------------------------------------------------------------------*/
/* allocate memory to created sdn packet */
static struct sdn_packet*
packet_allocate(void)
{
//  packet_deallocate(a);
  struct sdn_packet *p = NULL;
  p = memb_alloc(&packet_memb);
  a = p;
  return p;
}
/*----------------------------------------------------------------------*/
/* deallocate memory to created sdn packet */
void 
packet_deallocate(struct sdn_packet* p)
{
  if(p != NULL) {
    memb_free(&packet_memb, p);
    printf("deallocate packet \n");
  }
}
/*----------------------------------------------------------------------*/



