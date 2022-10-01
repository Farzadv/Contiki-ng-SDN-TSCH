/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         Code for the simple-udp module.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup simple-udp
 * @{
 */
#include "contiki.h"
#include "net/netstack.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>

//veisi
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sdn/sdn.h"
#include "net/mac/tsch/sdn/sdn-handle.h"
#include "net/mac/tsch/sdn/sdn-flow.h"

#if SDN_ENABLE
#include "net/mac/tsch/sdn/sdn-flow-request.h"
#endif


#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "SDN"
#define LOG_LEVEL SDN_CLIENT_APP_LOG_LEVEL

PROCESS(simple_udp_process, "Simple UDP process");
static uint8_t started = 0;
static uint8_t databuffer[UIP_BUFSIZE];


//veisi
#if SDN_ENABLE
static struct socket udp_socket;
static struct simple_udp_connection *temp_udp_conn;
#endif
/*---------------------------------------------------------------------------*/
static void
init_simple_udp(void)
{
  if(started == 0) {
    process_start(&simple_udp_process, NULL);
    started = 1;
  }
}
/*---------------------------------------------------------------------------*/
int
simple_udp_send(struct simple_udp_connection *c,
                const void *data, uint16_t datalen)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          &c->remote_addr, UIP_HTONS(c->remote_port));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
simple_udp_sendto(struct simple_udp_connection *c,
                  const void *data, uint16_t datalen,
                  const uip_ipaddr_t *to)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          to, UIP_HTONS(c->remote_port));
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
simple_udp_sendto_port(struct simple_udp_connection *c,
		       const void *data, uint16_t datalen,
		       const uip_ipaddr_t *to,
		       uint16_t port)
{
  if(c->udp_conn != NULL) {
    uip_udp_packet_sendto(c->udp_conn, data, datalen,
                          to, UIP_HTONS(port));
  }
  return 0;
}
/*--------------------------------veisi--------------------------------------*/
#if SDN_ENABLE
void 
sdn_app_flow_id_response(struct socket *req_udp_socket, const linkaddr_t *flow_id, const int rep_period)
{ 
  if(req_udp_socket != NULL && flow_id != NULL){
    if(uip_ipaddr_cmp(&udp_socket.ripaddr, &req_udp_socket->ripaddr) && udp_socket.rport == req_udp_socket->rport &&
      udp_socket.lport == req_udp_socket->lport) {           
      linkaddr_copy(&temp_udp_conn->udp_conn->flow_id, flow_id);
      temp_udp_conn->udp_conn->rep_period = rep_period;
      LOG_INFO("SIMPLE-UDP: register a new flow id for app: ");
      LOG_INFO_LLADDR(flow_id);
      LOG_INFO_("\n");
      //LOG_INFO("SIMPLE-UDP: register a new flow id for app 1 \n");
    }
  }
}
#endif
/*--------------------------------veisi--------------------------------------*/
#if SDN_ENABLE
int 
app_qos_request(struct simple_udp_connection *c,
                struct app_qos_attr *qos,
                uint16_t traffic_period,
                uint16_t reliability, 
                uint16_t max_delay, 
                uint16_t packet_size,
                uint16_t burst_size)
{ 
 if(qos != NULL){
   qos->traffic_period = traffic_period;
   qos->reliability = reliability;
   qos->max_delay = max_delay;
   qos->packet_size = packet_size;
   qos->burst_size = burst_size;
 }
 if(c != NULL){
   c->qos_attr = qos; 
   //LOG_INFO("SIMPLE-UDP: max delay %d \n", c->qos_attr->max_delay);
 }
  return 1;
}
#endif
/*---------------------------------------------------------------------------*/
int
simple_udp_register(struct simple_udp_connection *c,
                    uint16_t local_port,
                    uip_ipaddr_t *remote_addr,
                    uint16_t remote_port,
                    simple_udp_callback receive_callback)
{

  init_simple_udp();

  c->local_port = local_port;
  c->remote_port = remote_port;
  if(remote_addr != NULL) {
    uip_ipaddr_copy(&c->remote_addr, remote_addr);
  }
  c->receive_callback = receive_callback;

  PROCESS_CONTEXT_BEGIN(&simple_udp_process);
  c->udp_conn = udp_new(remote_addr, UIP_HTONS(remote_port), c);
  if(c->udp_conn != NULL && local_port) {
    udp_bind(c->udp_conn, UIP_HTONS(local_port));
  }
  /*TODO I suppose that application knows the flow_id and set it in the "struct simple_udp_connection"
   but this must be changed later; UDP layer is in charge of specifiyng the flow_id for application according the socket info.
   in this function (simple_udp_register) UDP layer must request the SDN layer to get the flow_id.
   it should be happened in the case that the best-effort flowid is not enough
   */
 
#if SDN_ENABLE
    temp_udp_conn = c;
    udp_socket.rport = remote_port;
    udp_socket.lport = local_port;
    if(remote_addr != NULL) {
      uip_ipaddr_copy(&udp_socket.ripaddr, remote_addr);
      uip_ipaddr_copy(&udp_socket.lipaddr, remote_addr); //TODO must be changed; just to fill the srcipaddr section ... 
    }
    /* request flow_id from SDN layer */
    linkaddr_t *flow_id = sdn_app_find_flow_id(&udp_socket, c->qos_attr);
    if(flow_id != NULL){
      linkaddr_copy(&c->udp_conn->flow_id, flow_id); 
      LOG_INFO("SIMPLE-UDP: return flow id \n");
    }   
#endif
  PROCESS_CONTEXT_END();

  if(c->udp_conn == NULL) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(simple_udp_process, ev, data)
{
  struct simple_udp_connection *c;
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {

      /* An appstate pointer is passed to use from the IP stack
         through the 'data' pointer. We registered this appstate when
         we did the udp_new() call in simple_udp_register() as the
         struct simple_udp_connection pointer. So we extract this
         pointer and use it when calling the reception callback. */
      c = (struct simple_udp_connection *)data;

      /* Defensive coding: although the appstate *should* be non-null
         here, we make sure to avoid the program crashing on us. */
      if(c != NULL) {

        /* If we were called because of incoming data, we should call
           the reception callback. */
        if(uip_newdata()) {
          /* Copy the data from the uIP data buffer into our own
             buffer to avoid the uIP buffer being messed with by the
             callee. */
          memcpy(databuffer, uip_appdata, uip_datalen());

          /* Call the client process. We use the PROCESS_CONTEXT
             mechanism to temporarily switch process context to the
             client process. */
          if(c->receive_callback != NULL) {
            PROCESS_CONTEXT_BEGIN(c->client_process);
            c->receive_callback(c,
                                &(UIP_IP_BUF->srcipaddr),
                                UIP_HTONS(UIP_UDP_BUF->srcport),
                                &(UIP_IP_BUF->destipaddr),
                                UIP_HTONS(UIP_UDP_BUF->destport),
                                databuffer, uip_datalen());
            PROCESS_CONTEXT_END();
          }
        }
      }
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/** @} */
