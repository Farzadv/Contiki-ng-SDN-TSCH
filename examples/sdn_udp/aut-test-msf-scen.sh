#!/bin/bash


#chmod +x ~/contiki-ng/examples/sdn_udp/py-config-creator/main.py 

# bkg_ts must be devidible to SF size
# must be carefull about number of timeslots ==> lower "SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD" exp: 3000/100 = 30 max ts

# 1- traffic period must be multiplication of SF_REP_PERIOD. for exp: sf_size = 3001, SF_REP_PERIOD=100 => traffic periods: 100, 300,
# 500, 600, 1000, 1500, 3000
# 2- also traffic period must be devidible by (SF_REP_NUM * SF_REP_PERIOD)

#########################################################################
# clean log directory
if [ -d "msf-statistics/log" ] 
then
    rm -r msf-statistics/log 
fi

if [ ! -d "msf-statistics/log" ] 
then
    mkdir msf-statistics/log 
    echo "create log directory"
fi

# remove COOJA.testlog file
if [ -f "COOJA.testlog" ] 
then
		rm COOJA.testlog
fi

######################################################################### 
NODE_NUM_LIST=(2 3 4)                        # network size included Sink
NODE_CRITIC_NUM=(1 2 3)                       # number of critic nodes
BKG_TS_NUM=(1)                           # number of besteffort timeslot that each node has 
ITER_PER_CONF=10                        # number of iteration for each config.csc
SRVR_NUM=1                              # number of server
LQR_LIST=(0.95 0.84 0.71)                                # list of link quality ratio


LIST_SIZE=${#NODE_NUM_LIST[*]}            # traffic array len
BKG_TS_LIST_SIZE=${#BKG_TS_NUM[*]}        # traffic array len
LQR_LIST_SIZE=${#LQR_LIST[*]}

echo "LIST_SIZE val $LIST_SIZE"
k=0
while [[ $k -le $(($LQR_LIST_SIZE-1)) ]]
do
		j=0
		while [[ $j -le $(($LIST_SIZE-1)) ]]
		do 
				# mkdir simulation-log/sdn-tsch/node_num_${NODE_NUM_LIST[j]}							
				# repeat a given simulation for n times with different seed
				i=1
				while [[ $i -le $ITER_PER_CONF ]]
				do
						python3 ~/contiki-ng/examples/sdn_udp/msf-scen-config-create/main.py \
						sf_size=[251] \
						ctrl_sf_size=[30] \
						sf_rep_period=[251] \
						node_num=[${NODE_NUM_LIST[j]}] \
						server_num=[$SRVR_NUM] \
						client_bkg_num=[$((${NODE_NUM_LIST[j]}-${NODE_CRITIC_NUM[j]}-SRVR_NUM))] \
						client_critic_num=[${NODE_CRITIC_NUM[j]}] \
						bkg_traffic_period=[2.51] \
						critic_traffic_period=[2.51] \
						bkg_timeslot_num=[1] \
						tx_range=[50.0] \
						intf_range=[100.0] \
						tx_success=[1.0] \
						rx_success=[${LQR_LIST[k]}] \
						x_radius=[250] \
						y_radius=[250] \
						sim_time_sdn=[8800000]
																		
						# update random seed in each iteration
						python3 ~/contiki-ng/examples/sdn_udp/rand_seed_gen_py/main.py tsch-sdn
						
						java -Xshare:on -jar ../../tools/cooja/dist/cooja.jar -nogui=config.csc -contiki=../../	
						if [ -f "COOJA.testlog" ] 
						then
								mv ~/contiki-ng/examples/sdn_udp/COOJA.testlog \
					      ~/contiki-ng/examples/sdn_udp/msf-statistics/log/sdn-net-${NODE_NUM_LIST[j]}-itr-$i-lqr${LQR_LIST[k]}.testlog
						fi
						
						i=$(($i+1))	
											
				done
				
				j=$(($j+1))	
								
		done
		k=$(($k+1))
done	
###################################################################################

