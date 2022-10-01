


#ifndef SDN_CONF_H_
#define SDN_CONF_H_



/* Enable SDN node to find the position of the shared cells in the slotframe */
#ifdef SDN_CONF_SHARE_SLOT_POSITION
#define SDN_SHARE_SLOT_POSITION SDN_CONF_SHARE_SLOT_POSITION
#else
#define SDN_SHARE_SLOT_POSITION 0
#endif


/* Enable SDN node to construct a neighbor table based received EBs */
#ifdef SDN_CONF_NBR_TABLE
#define SDN_NBR_TABLE SDN_CONF_NBR_TABLE
#else
#define SDN_NBR_TABLE 1
#endif

/* Maximum number of flows */
#ifdef SDN_CONF_MAX_FLOW_NUM
#define SDN_MAX_FLOW_NUM SDN_CONF_MAX_FLOW_NUM 
#else 
#define SDN_MAX_FLOW_NUM  8 
#endif


/* The maximum number of outgoing packets for each flow
 // Must be power of two to enable atomic ringbuf operations. 
#ifdef SDN_CONF_QUEUE_NUM_PER_FLOW
#define SDN_QUEUE_NUM_PER_FLOW SDN_CONF_QUEUE_NUM_PER_FLOW
#else 
#define SDN_QUEUE_NUM_PER_FLOW  4 // valid values: 2, 4, 8, 16, ...
#endif
*/

/* report period conf */
#ifdef SDN_CONF_REPORT_PERIOD
#define SDN_REPORT_PERIOD SDN_CONF_REPORT_PERIOD
#else 
#define SDN_REPORT_PERIOD   5     // CLOCK SECOND
#endif

/* Link metric to be reported */

#ifdef SDN_CONF_LINK_METRIC
#define SDN_LINK_METRIC SDN_CONF_LINK_METRIC
#else
#define SDN_LINK_METRIC EB_NUM
#endif

/* SDN_CONF_WITH_PAYLOAD_TERMINATION_IE */
#ifdef SDN_CONF_WITH_PAYLOAD_TERMINATION_IE
#define SDN_WITH_PAYLOAD_TERMINATION_IE SDN_CONF_WITH_PAYLOAD_TERMINATION_IE
#else 
#define SDN_WITH_PAYLOAD_TERMINATION_IE 0
#endif


#endif







