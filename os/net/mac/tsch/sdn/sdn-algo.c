
#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-statis.h"
#include "net/mac/tsch/sdn/sdn-algo.h"
#include "lib/memb.h"
#include <stdio.h>
#include <math.h>


#include "sys/log.h"
#define LOG_MODULE "SDN_ALGO"
#define LOG_LEVEL SDN_ALGO_LOG_LEVEL



void dist_non_eb_shared_cells(int num_cell);



static int non_eb_cell_list[50];
/*----------------------------------------------------------------------*/
void
dist_non_eb_shared_cells(int num_cell)
{ 
  int list_size = num_cell;
  int i, j;
  for (i=0; i<list_size; i++) {
    non_eb_cell_list[i] = 0;
  }
  non_eb_cell_list[0] = 1;
  int empty_space = 1;
  int glob_count = 2;
  int itera = 0;
  while( empty_space != 0 && itera < 10) {
    itera++;
    int max_empty_space = 0;
    int space_count = 0;
    int rep_same_space = 0;
    

    for(i=0; i<list_size; i++){
      if(non_eb_cell_list[i] == 0){ 
        space_count++;
      }   
      if(non_eb_cell_list[i] != 0 || i == list_size - 1){
          
        if(space_count > max_empty_space){
          max_empty_space = space_count;
          rep_same_space++;
        }  
        else if(space_count == max_empty_space && space_count != 0) {
          rep_same_space++;
        }
        
        space_count = 0;
      }    
    }
    

    for( j=0; j<rep_same_space; j++){
      for(i=0; i<list_size; i++) {
        if(non_eb_cell_list[i] == 0) {
          space_count++;
        }  
        if(non_eb_cell_list[i] != 0) {
          space_count = 0;
        }
        if(space_count == max_empty_space) {
        //printf("find space %d %d \n", i, max_empty_space);
        /*        
          if(i == list_size - 1) {
            //printf("inside %d %f \n", i, ceil((float)(i + i - max_empty_space) / 2));
            non_eb_cell_list[(int)ceil((float)(i + i - max_empty_space) / 2)] = glob_count;
            glob_count++;
          }
          else {
        */
            //printf("x val %d \n", (int)ceil((float)(i + 1 + i - max_empty_space) / 2));
            non_eb_cell_list[(int)ceil((float)(i + 1 + i - max_empty_space) / 2)] = glob_count;
            glob_count++;
            space_count = 0;
        //  }
        }
      }
    }
    
    if(max_empty_space == 0) {
        empty_space = 0;
    }
    //printf(" max space: %d rep: %d \n",max_empty_space, rep_same_space);  
  }
  /*
  for(i=0; i<list_size; i++) {
    printf(" %d", non_eb_cell_list[i]);
  }  
  printf("\n");
  */
}
/*----------------------------------------------------------------------*/
struct list_dist_uniform *
sdn_distribute_list_uniform(struct list_dist_uniform *req_list)
{ 
  if (req_list == NULL) { 
    return NULL;
  }
  if(req_list->len > DIST_UNIFORM_LEN) {
    printf("SDN-ALGO: list size exeeds max len \n");
    return NULL;
  }
  
  
  
  int list_size = req_list->len;
  int list_size_noneb = list_size - NETWORK_SIZE;
  dist_non_eb_shared_cells(list_size_noneb);
  
  int i, j;
  int my_list[list_size];
  for (i=0; i<list_size; i++) {
    my_list[i] = 0;
  }
  my_list[0] = 1;
  int empty_space = 1;
  int glob_count = 2;
  int itera = 0;
  while( empty_space != 0 && itera < 10) {
    itera++;
    int max_empty_space = 0;
    int space_count = 0;
    int rep_same_space = 0;
    

    for(i=0; i<list_size; i++){
      if(my_list[i] == 0){ 
        space_count++;
      }   
      if(my_list[i] != 0 || i == list_size - 1){
          
        if(space_count > max_empty_space){
          max_empty_space = space_count;
          rep_same_space++;
        }  
        else if(space_count == max_empty_space && space_count != 0) {
          rep_same_space++;
        }
        
        space_count = 0;
      }    
    }
    

    for( j=0; j<rep_same_space; j++){
      for(i=0; i<list_size; i++) {
        if(my_list[i] == 0) {
          space_count++;
        }  
        if(my_list[i] != 0) {
          space_count = 0;
        }
        if(space_count == max_empty_space) {
        //printf("find space %d %d \n", i, max_empty_space);
        /*        
          if(i == list_size - 1) {
            //printf("inside %d %f \n", i, ceil((float)(i + i - max_empty_space) / 2));
            my_list[(int)ceil((float)(i + i - max_empty_space) / 2)] = glob_count;
            glob_count++;
          }
          else {
        */
            //printf("x val %d \n", (int)ceil((float)(i + 1 + i - max_empty_space) / 2));
            my_list[(int)ceil((float)(i + 1 + i - max_empty_space) / 2)] = glob_count;
            glob_count++;
            space_count = 0;
        //  }
        }
      }
    }
    
    if(max_empty_space == 0) {
        empty_space = 0;
    }
    //printf(" max space: %d rep: %d \n",max_empty_space, rep_same_space);
   
  }
  
  int tmp_list[list_size_noneb];
  int tmp_index = 0;
  for(i=0; i<list_size; i++) {
    if(my_list[i] > NETWORK_SIZE) {
      tmp_list[tmp_index] = i;
      tmp_index++;
    }
  }
  
  // dist. non eb shared cells
  for(i=0; i<list_size_noneb; i++) {
    my_list[tmp_list[i]] = non_eb_cell_list[i] + NETWORK_SIZE;
  }
  
  
  for(i=0; i<list_size; i++) {
    //printf(" %d", my_list[i]);
    req_list->list[i] = my_list[i];
  }  
  //printf("\n");
  
  return req_list;
  
}
/*----------------------------------------------------------------------*/
int
get_sf_offset_from_ts(int timeslot)
{
  int i;
  int num_rep = (int)(SDN_DATA_SLOTFRAME_SIZE / SDN_SF_REP_PERIOD);
  int num_sh_cell_in_rep = (int)ceil((float)((float)sf0->num_shared_cell / (float)num_rep));
  const int list_shared_cell_len = (int)(num_sh_cell_in_rep * num_rep);
  
 
  static struct list_dist_uniform list_dist;
  list_dist.len = list_shared_cell_len;
  
  for(i=0; i<DIST_UNIFORM_LEN; i++) {
    list_dist.list[i] = 0;
  }
  sdn_distribute_list_uniform(&list_dist);
  
  int m;
  int ts;
 
  for(m=0; m<list_shared_cell_len; m++) {
    ts = ((m / num_sh_cell_in_rep) * (SDN_SF_REP_PERIOD)) + 
         ((m % num_sh_cell_in_rep) * (int)(SDN_SF_REP_PERIOD / num_sh_cell_in_rep)); 
    if(ts == timeslot) {
      //printf("find sf offs from ts: %d \n", list_dist.list[m]);
      return list_dist.list[m];
    }
  }   
  return -1;
}
/*----------------------------------------------------------------------*/
























