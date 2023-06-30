


#ifndef SDN_CONF_H_
#define SDN_CONF_H_



/* Enable SDN node to find the position of the shared cells in the slotframe */
#ifdef SDN_CONF_SHARE_SLOT_POSITION
#define SDN_SHARE_SLOT_POSITION SDN_CONF_SHARE_SLOT_POSITION
#else
#define SDN_SHARE_SLOT_POSITION 0
#endif


/* the ratio threshold for selecting best neighbors */
#ifdef SDN_CONF_TRSHLD_BETS_NBRS
#define SDN_TRSHLD_START_DISCOVERY   SDN_CONF_TRSHLD_BETS_NBRS
#else
#define SDN_TRSHLD_START_DISCOVERY   0.84
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
#define SDN_REPORT_PERIOD   60     // CLOCK SECOND
#endif

/* Max ch_off len */
#ifdef SDN_CONF_MAX_CH_OFF_NUM
#define SDN_MAX_CH_OFF_NUM SDN_CONF_MAX_CH_OFF_NUM
#else 
#define SDN_MAX_CH_OFF_NUM   16     // CLOCK SECOND
#endif

/* Max cells can be allocated for a link */
#ifdef SDN_CONF_MAX_CELLS_PER_HOP
#define SDN_MAX_CELLS_PER_HOP SDN_CONF_MAX_CELLS_PER_HOP
#else 
#define SDN_MAX_CELLS_PER_HOP   10  
#endif

/* Link metric to be reported */
#ifdef SDN_CONF_LINK_METRIC
#define SDN_LINK_METRIC SDN_CONF_LINK_METRIC
#else
#define SDN_LINK_METRIC EB_NUM
#endif

#define MAX_MAC_RETRANSMISSION    15

/* Control plane slotframe size */
#ifdef SDN_CONF_CONTROL_SLOTFRAME_SIZE
#define SDN_CONTROL_SLOTFRAME_SIZE   SDN_CONF_CONTROL_SLOTFRAME_SIZE
#else
#define SDN_CONTROL_SLOTFRAME_SIZE   30
#endif


/* Data plane slotframe size */
#ifdef SDN_CONF_DATA_SLOTFRAME_SIZE
#define SDN_DATA_SLOTFRAME_SIZE    SDN_CONF_DATA_SLOTFRAME_SIZE
#else
#define SDN_DATA_SLOTFRAME_SIZE 3061
#endif

/* Repetition period inside the data SF */
#ifdef SDN_CONF_SF_REP_PERIOD
#define SDN_SF_REP_PERIOD SDN_CONF_SF_REP_PERIOD
#else
#define SDN_SF_REP_PERIOD   61     // this value should not exceed than 255, in that case we need 2 byte for timeslot in packet
#endif


/* number of shared cells in a single repetition */
/*
#ifdef SDN_CONF_NUM_SHARED_CELL_IN_REP
#define SDN_NUM_SHARED_CELL_IN_REP SDN_CONF_NUM_SHARED_CELL_IN_REP
#else
#define SDN_NUM_SHARED_CELL_IN_REP 2
#endif

*/
/* number of shared cells */
/*
#ifdef SDN_CONF_NUM_SHARED_CELL
#define SDN_NUM_SHARED_CELL SDN_CONF_NUM_SHARED_CELL
#else
#define SDN_NUM_SHARED_CELL (SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD)*SDN_NUM_SHARED_CELL_IN_REP
#endif
*/

/* handle from-controller flow-id through shared cell */
#ifdef SDN_CONF_SHARED_FROM_CTRL_FLOW
#define SDN_SHARED_FROM_CTRL_FLOW   SDN_CONF_SHARED_FROM_CTRL_FLOW
#else
#define SDN_SHARED_FROM_CTRL_FLOW   0
#endif

/* handle control traffic over shared cells */
#ifdef SDN_CONF_SHARED_CONTROL_PLANE
#define SDN_SHARED_CONTROL_PLANE SDN_CONF_SHARED_CONTROL_PLANE
#else
#define SDN_SHARED_CONTROL_PLANE   0
#endif


/* do I reconfig the network */
#ifdef SDN_CONF_RECONF_MODE
#define SDN_RECONF_MODE  SDN_CONF_RECONF_MODE
#else
#define SDN_RECONF_MODE  0
#endif


/* handle control and EB traffic over shared cells without any control */
#ifdef SDN_CONF_UNCONTROLLED_EB_SENDING
#define SDN_UNCONTROLLED_EB_SENDING     SDN_CONF_UNCONTROLLED_EB_SENDING
#else
#define SDN_UNCONTROLLED_EB_SENDING     0
#endif

/* Max SDN packet len: in SDN layer befor adding 6 byte SDN-IEs */
#ifdef SDN_CONF_MAX_PACKET_LEN
#define SDN_MAX_PACKET_LEN     SDN_CONF_MAX_PACKET_LEN
#else 
#define SDN_MAX_PACKET_LEN   110
#endif

/* RX success: it is simulator parameter (COOJA): use it for calculation of ideal number of cells for a flow */
#ifdef SDN_CONF_SIMULATION_RX_SUCCESS
#define SDN_SIMULATION_RX_SUCCESS     SDN_CONF_SIMULATION_RX_SUCCESS
#else 
#define SDN_SIMULATION_RX_SUCCESS     1.0
#endif

/* SDN_CONF_WITH_PAYLOAD_TERMINATION_IE */
#ifdef SDN_CONF_WITH_PAYLOAD_TERMINATION_IE
#define SDN_WITH_PAYLOAD_TERMINATION_IE SDN_CONF_WITH_PAYLOAD_TERMINATION_IE
#else 
#define SDN_WITH_PAYLOAD_TERMINATION_IE 0
#endif


#endif







