
#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-statis.h"
#include "net/mac/tsch/sdn/sdn-link.h"
#include "net/mac/tsch/sdn/sdn-algo.h"
#include "lib/memb.h"
#include <stdio.h>


#include "sys/log.h"
#define LOG_MODULE "SDN_LINK"
#define LOG_LEVEL SDN_LINK_LOG_LEVEL

#define max_sf_offset_num      (int)((TSCH_EB_PERIOD/10) / SDN_DATA_SLOTFRAME_SIZE)
static int shared_cell_alloc_list[max_sf_offset_num][SDN_DATA_SLOTFRAME_SIZE];

/*----------------------------------------------------------------------*/
/* update the list if when rceive a EB in a shared cell */
void
sdn_update_shared_cell_usage(const int sf_off, const int timeslot)
{
  if(!tsch_is_locked()){
    if(shared_cell_alloc_list[sf_off][timeslot] < 0) {
      shared_cell_alloc_list[sf_off][timeslot] = 1;
      //printf("update sdn shared link: sf off: %d, ts: %d \n", sf_off, timeslot);
    }
  }
}
/*----------------------------------------------------------------------*/
/* initialize the list of shared cell occupation */
void
sdn_link_init()
{
  int i;
  int j;
  for(i=0; i<max_sf_offset_num; i++) {
    for(j=0; j<SDN_DATA_SLOTFRAME_SIZE; j++) {
      shared_cell_alloc_list[i][j] = -1;
    }
  }
  //printf("init sdn link done! %d \n", shared_cell_alloc_list[0][1]);
}
/*----------------------------------------------------------------------*/
/* check wheather this timeslot is lready used by a node to send EB or no? */
/*int 
sdn_is_shared_cell_occupied(const int sf_off, const int timeslot)
{
  if(!tsch_is_locked()){
    //find max used sf_off and ts_offs
    int i;
    int j;
    int max_sf_offs;
    int max_ts_offs;
    for(i=0; i<max_sf_offset_num; i++) {
      for(j=0; j<SDN_DATA_SLOTFRAME_SIZE; j++) {
        if(shared_cell_alloc_list[i][j] == 1){
          max_sf_offs = i;
          max_ts_offs = j;
        }
      }
    }
    //
  
  
    if(sf_off < max_sf_offs) {
      //printf("shared cell is used: %d \n", shared_cell_alloc_list[sf_off][timeslot]);
      return 1;
    } else if(sf_off == max_sf_offs && timeslot <= max_ts_offs) {
      return 1;
    }
  }
  
  return 0;
}
*/
/*----------------------------------------------------------------------*/
/* check wheather this timeslot is lready used by a node to send EB or no? */
/*int 
sdn_is_shared_cell_occupied(const int sf_off, const int timeslot)
{
  if(!tsch_is_locked()){
    if(shared_cell_alloc_list[sf_off][timeslot] == 1) {
      //printf("shared cell is used: %d \n", shared_cell_alloc_list[sf_off][timeslot]);
      return 1;
    }
  }
  
  return 0;
}*/
/*----------------------------------------------------------------------*/
/* check wheather this timeslot is lready used by a node to send EB or no? */
int 
sdn_is_shared_cell_occupied(const int sf_off, const int timeslot)
{
  if(!tsch_is_locked()){
    int sf_offs = get_sf_offset_from_ts(timeslot);
    if(sf_offs <= sdn_max_used_sf_offs) {
      return 1;
    }
  }  
  return 0;
}
/*----------------------------------------------------------------------*/
