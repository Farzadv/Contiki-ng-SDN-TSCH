/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 */

/**
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/* USB serial takes space, free more space elsewhere */
//#define SICSLOWPAN_CONF_FRAG 0
//#define UIP_CONF_BUFFER_SIZE 160
#define CONTIKI_TARGET_COOJA 1
/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0x81a5

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 1

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
//#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 20

#undef TSCH_CONF_DEFAULT_HOPPING_SEQUENCE
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_16_16
//#define TSCH_CONF_JOIN_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_1_1

#define TSCH_CONF_AUTOSELECT_TIME_SOURCE 0
/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL 0
#define TSCH_PACKET_CONF_EB_WITH_SLOTFRAME_AND_LINK 1
#define TSCH_CONF_INIT_SCHEDULE_FROM_EB 1

#define TSCH_CONF_EB_PERIOD (1506 * CLOCK_SECOND / 100)
#define TSCH_CONF_KEEPALIVE_TIMEOUT (40 * CLOCK_SECOND)

/* just in the case we use the uncontrolled EB sending we increase the leaving parameters -> we have high collision */ 
/*
#if SDN_CONF_UNCONTROLLED_EB_SENDING
#define TSCH_CONF_KEEPALIVE_TIMEOUT (50 * CLOCK_SECOND)
#define TSCH_CONF_DESYNC_THRESHOLD  (4 * TSCH_MAX_KEEPALIVE_TIMEOUT)
#endif
*/

#define TSCH_SCHEDULE_CONF_MAX_LINKS  1500
/* I add the following line to enable sender node to pars enhanced ACK 
   In the case we use short address, without enabeling the following line 
   receiver cannot pars Ack packet*/ 
#define TSCH_PACKET_CONF_EACK_WITH_SRC_ADDR 1
#define LINKADDR_CONF_SIZE 2

// Not sure why I add these 2 lines?
//#define QUEUEBUF_CONF_NUM                  128
#if SINK
#define TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR     32
#else
#define TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR     16
#endif
/*******************************************************/
/******************* UIP Configure  ********************/
/*******************************************************/
#define UIP_CONF_TCP 0

#define UIP_CONF_ND6_SEND_RA   0
#define UIP_CONF_ND6_SEND_NS   0
#define UIP_CONF_ND6_SEND_NA   0

#define SICSLOWPAN_CONF_FRAG 0
//#define SDN_CONF_DATA_SLOTFRAME_SIZE     (30 * 100)+1
//#define SDN_CONF_NUM_SHARED_CELL         (30 * 10)
//#define UIP_CONF_ND6_AUTOFILL_NBR_CACHE 0

/*******************************************************/
/******************* Configure SDN  ********************/
/*******************************************************/
#define SDN_PRINT_ASN    0xE7EF0
#define SDN_CONF_ENABLE 1
#define SDN_CONF_SHARE_SLOT_POSITION 1                      // if =1 -> node fine shared slots positions. if =0 -> just minimal schedule 
#define SDN_CONF_NBR_TABLE 1
#define SDN_CONF_LINK_METRIC EB_NUM
#define SDN_CONF_REPORT_PERIOD   (20 * 1506 / 100)         
//#define SDN_CONF_NUM_SHARED_CELL_IN_REP  20               // num shared cells per sub-SF repetition
//#define SDN_CONF_SHARED_CONTROL_PLANE       1 
//#define SDN_CONF_SHARED_FROM_CTRL_FLOW    1
//#define SDN_CONF_UNCONTROLLED_EB_SENDING    1

#define SDN_CONF_MAX_CELLS_PER_HOP  10

#define ENERGEST_CONF_ON 1

/*
To enable MDPI test: 
enable "SDN_MDPI_TEST"
set the admission threshold of MDPI to 0.4 and the else to 0.6
run command: java -Xshare:on -jar ../../tools/cooja/dist/cooja.jar -nogui=config-mdpi.csc -contiki=../../ -log-key=112233
*/
#define SDN_MDPI_TEST    0





/* enable reconf test */
#define SDN_CONF_RECONF_MODE  1


/* SDN LOG Level */
#define SDN_LOG_LEVEL                  LOG_LEVEL_INFO
#define SDN_INOUT_LOG_LEVEL            LOG_LEVEL_WARN
#define SDN_HANDLE_LOG_LEVEL           LOG_LEVEL_INFO
#define SDN_FLOW_LOG_LEVEL             LOG_LEVEL_INFO
#define SDN_ALGO_LOG_LEVEL             LOG_LEVEL_INFO
#define SDN_REQ_FLOW_LOG_LEVEL         LOG_LEVEL_INFO
#define SDN_SINK_LOG_LEVEL             LOG_LEVEL_INFO
#define SDN_CLIENT_APP_LOG_LEVEL       LOG_LEVEL_WARN
#define SDN_SERVER_APP_LOG_LEVEL       LOG_LEVEL_INFO
#define SDN_STATIS_LOG_LEVEL           LOG_LEVEL_INFO
#define TSCH_LOG_SDN_PER_SLOT          0     // enable per slot log

// LOG_LEVEL_NONE         0 /* No log */
// LOG_LEVEL_ERR          1 /* Errors */
// LOG_LEVEL_WARN         2 /* Warnings */
// LOG_LEVEL_INFO         3 /* Basic info */
// LOG_LEVEL_DBG          4 /* Detailled debug */
/*******************************************************/
/************* Other system configuration **************/
/*******************************************************/
//LOG_LEVEL_DBG
/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_WARN
#define TSCH_LOG_CONF_PER_SLOT                     1

//#define TSCH_LOG_CONF_QUEUE_LEN    16

#endif /* PROJECT_CONF_H_ */
