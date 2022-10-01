

#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/mac/tsch/tsch.h"
#include <stdio.h>

#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-conf.h"
#include "net/mac/tsch/sdn/sdn-packet.h"
#include "net/mac/tsch/sdn/sdn-inout.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-sink.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#include "sys/log.h"
#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC

void sdn_payload_termination_ie(void);
/*---------------------------------------------------------------------------*/
/* sdn payload termination IE */
void
sdn_payload_termination_ie(void)
{
  uint8_t *ptr = packetbuf_dataptr();
  if(ptr[0] == 0x00 && ptr[1] == 0xf8) {
    /* Payload Termination IE is 2 octets long */
    //printf("sdn-inout: sdn payload termination IE\n");
    packetbuf_hdrreduce(2);
  }
}
/*---------------------------------------------------------------------------*/
/* this function is used to add SDN IE for generated packet and send it */
/* sdn output function
 * create IEs, 
 * put the SDN data in the payload IE
*/
int 
sdn_output(const linkaddr_t *dest_addr, const linkaddr_t *flow_id, int is_sdn_content)
{
  struct ieee802154_ies ies;
  int len;
  int sdn_data_len;

  if(is_sdn_content){
    sdn_data_len = packetbuf_datalen();
  } else {
    sdn_data_len = 0;
  }

  /* add SDN IE */
  memset(&ies, 0, sizeof(ies));
  linkaddr_copy(&ies.sdn_ie_flowid, flow_id);
  ies.sdn_ie_content_len = sdn_data_len;
  if (packetbuf_hdralloc(4) !=1 ||
    (len = frame80215e_create_ie_sdn(packetbuf_hdrptr(), 2, &ies)) < 0){
    printf("sdn-inout: sdn ie payload is failed\n");
    return -1;
  }
/* this conf is configured in sdn-conf.h */
#if SDN_WITH_PAYLOAD_TERMINATION_IE
  /* add payload termination IE */
  memset(&ies, 0, sizeof(ies));
  if(is_sdn_content){
    if((len = frame80215e_create_ie_payload_list_termination((uint8_t *)packetbuf_dataptr() + sdn_data_len,
                                                              PACKETBUF_SIZE - packetbuf_totlen(), &ies))<0){
      printf("sdn-inout: sdn ie payload termination is failed\n");
      return -1;
    } 
    packetbuf_set_datalen(packetbuf_datalen() + len);
  } else {
    memmove((uint8_t *)packetbuf_dataptr() + 2, packetbuf_dataptr(), packetbuf_datalen());

    if((len = frame80215e_create_ie_payload_list_termination((uint8_t *)packetbuf_dataptr(),
                                                              PACKETBUF_SIZE - packetbuf_totlen(), &ies))<0){
      printf("sdn-inout: sdn ie payload termination is failed\n");
      return -1;
    } 
    packetbuf_set_datalen(packetbuf_datalen() + len);
  }
#endif
  /* add header termination IE */
  memset(&ies, 0, sizeof(ies));
  if(packetbuf_hdralloc(2) &&
    frame80215e_create_ie_header_list_termination_1(packetbuf_hdrptr(), 2, &ies) < 0) {
    printf("sdn-inout: sdn ie header termination1 is failed\n");
  }

  /* set dest addr */
  if(linkaddr_cmp(&linkaddr_null, dest_addr)){
    linkaddr_t *dest_addr = sdn_get_addr_for_flow_id(flow_id);
    if(dest_addr != NULL){
      packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest_addr);
    } else {
      LOG_INFO("sdn-inout: dest_addr is NULL for flow: ");
      LOG_INFO_LLADDR(flow_id);
      LOG_INFO_("\n");
    }
  } else {
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest_addr);
  }

  /* set attr */
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_METADATA, 1);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  LOG_INFO("sdn-inout: flow id sent report ");
  LOG_INFO_LLADDR(flow_id);
  LOG_INFO_("\n");
  packetbuf_set_addr(PACKETBUF_ADDR_FLOW_ID, flow_id);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

  NETSTACK_MAC.send(NULL, NULL);
  return 0;
}
/*---------------------------------------------------------------------------*/
/* read SDN IEs */
void 
sdn_input(void)
{
  uint8_t *payload_ptr;

  struct ieee802154_ies ies;
  payload_ptr = packetbuf_dataptr();
  printf("sdn-inout: packetbuf datalen: %d\n", packetbuf_datalen());
  if(frame802154e_parse_information_elements(packetbuf_dataptr(), packetbuf_datalen(), &ies) >= 0 &&
    ies.sdn_ie_content_ptr != NULL &&
    ies.sdn_ie_content_len > 0){

    if (ies.is_sdn_data){
      printf("sdn-inout: sdn IE packet len: %d\n", ies.sdn_ie_content_len);
      LOG_INFO("sdn-inout: pkt ie flowid ");
      LOG_INFO_LLADDR(&ies.sdn_ie_flowid);
      LOG_INFO_("\n");
      /* point to the content of payload SDN IE */
      //linkaddr_t flow_id;
      //linkaddr_copy(&flow_id, packetbuf_addr(PACKETBUF_ADDR_FLOW_ID));

      //TODO if it is config packet, pass it to sdn_handle ...||| if SINK -> pass everything
      // if it is report, look up in flow table to relay it
      sdn_handle(ies.sdn_ie_content_ptr, ies.sdn_ie_content_len, &ies.sdn_ie_flowid);
      packetbuf_hdrreduce(ies.sdn_ie_content_ptr - payload_ptr + ies.sdn_ie_content_len);
      sdn_payload_termination_ie();
    } else {
      printf("sdn-inout: it is a DATA packet\n");
      //TODO
      /*
     - look up flow table
     - if find a forward rule -> send-relay(packetbuf_dataptr(), packetbuf_datalen()) -> it sends what it received at biggening
     - if find a rule modify  -> remove the flow id and send payload to upper layer.
      packetbuf_hdrreduce(ies.sdn_ie_content_ptr - payload_ptr);
      sdn_payload_termination_ie();
     */
    packetbuf_hdrreduce(ies.sdn_ie_content_ptr - payload_ptr); // remove IE parts just befor payload termination IE
    sdn_payload_termination_ie(); // remove payload termination IE
    uint8_t p[packetbuf_datalen()];
    int len = packetbuf_datalen();
    memcpy((uint8_t*)&p, packetbuf_dataptr(), packetbuf_datalen());

    int i;
    for (i=0; i<len; i++){
      printf("count pay %u\n", p[i]);   
    }


    }
  } else {
      printf("sdn-inout: unable to read sdn IEs\n");
  }
}
/*---------------------------------------------------------------------------*/
/* relay function that handle the received packet just based on the flow-id */
void 
sdn_relay_send(const linkaddr_t *flow_id)
{
  uint8_t *pkt[127]; 
  uint8_t pkt_size;
  pkt_size = packetbuf_datalen();
  memcpy((uint8_t *)&pkt, packetbuf_dataptr(), packetbuf_datalen());

  packetbuf_clear();
  packetbuf_copyfrom((uint8_t *)pkt, pkt_size);

  packetbuf_set_attr(PACKETBUF_ATTR_MAC_METADATA, 1);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  //TODO sink address must be changed later. it is static and may be set another place
  //packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &sink_address);
  linkaddr_t *dest_addr = sdn_get_addr_for_flow_id(flow_id);
  if(dest_addr != NULL){
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest_addr);
  } else {
    LOG_INFO("sdn-inout: dest_addr is NULL for flow 2: ");
    LOG_INFO_LLADDR(flow_id);
    LOG_INFO_("\n");
  }
  packetbuf_set_addr(PACKETBUF_ADDR_FLOW_ID, flow_id);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  NETSTACK_MAC.send(NULL, NULL);
}
/*---------------------------------------------------------------------------*/

void sdn_init(void)
{
  sdn_flow_init();
#if SINK
  sdn_sink_init();
#endif
}
/*---------------------------------------------------------------------------*/





