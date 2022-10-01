#!/bin/bash


#chmod +x /home/fvg/contiki-ng/examples/sdn_udp/py-config-creator/main.py 

# bkg_ts must be devidible to SF size
# must be carefull about number of timeslots ==> lower "SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD" exp: 3000/100 = 30 max ts

# 1- traffic period must be multiplication of SF_REP_PERIOD. for exp: sf_size = 3001, SF_REP_PERIOD=100 => traffic periods: 100, 300,
# 500, 600, 1000, 1500, 3000
# 2- also traffic period must be devidible by (SF_REP_NUM * SF_REP_PERIOD)

#########################################################################

# create sim-plot directory
if [ -d "/home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log" ] 
then
    rm -r /home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log
fi


if [ ! -d "/home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log" ] 
then
    mkdir mkdir /home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log
fi



# remove COOJA.testlog file
if [ -f "COOJA.testlog" ] 
then
		rm COOJA.testlog
fi

# remove topo graph file
if [ -f "topo_graph_density" ] 
then
		rm topo_graph_density
fi
######################################################################### 
NODE_NUM_LIST=(25)                        # network size included Sink
NODE_CRITIC_NUM=(24)                       # number of critic nodes
BKG_TS_NUM=(1)                           # number of besteffort timeslot that each node has
ORCH_SF_LEN=(101)                 # orch sf size 
LQR_LIST=(0.84)  
RADIUS_LIST=(20.0)                 # given network size=25 & max_hop=4 and max_nbr=10 and tx_range=50 --> max coef_of_radius=8 => radius=200
ITER_PER_CONF=5                           # number of iteration for each config.csc
SRVR_NUM=1                                # number of server

LIST_SIZE=${#NODE_NUM_LIST[*]}            # traffic array len
BKG_TS_LIST_SIZE=${#BKG_TS_NUM[*]}        # traffic array len
LQR_LIST_LEN_SIZE=${#LQR_LIST[*]}       # orch sf list len
ORCH_SF_LEN_SIZE=${#ORCH_SF_LEN[*]}       # orch sf list len
RADIUS_LIST_SIZE=${#RADIUS_LIST[*]}       

echo "LIST_SIZE val $LIST_SIZE"

j=0
while [[ $j -le $(($LIST_SIZE-1)) ]]
do 
		k=0
		while [[ $k -le $(($LQR_LIST_LEN_SIZE-1)) ]]
		do									
				m=0
				while [[ $m -le $(($RADIUS_LIST_SIZE-1)) ]]
		    do
						# repeat a given simulation for n times with different seed
						i=1
						while [[ $i -le $ITER_PER_CONF ]]
						do
								python3 /home/fvg/contiki-ng/examples/sdn_udp/density-config-creator/main.py \
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
								x_radius=[${RADIUS_LIST[m]}] \
								y_radius=[${RADIUS_LIST[m]}] \
								itr=[$i] \
								sim_time_sdn=[10000000] \
								sim_time_orch=[3000000] \
								itr_num=[$i]   # 50 min sim len and ASN time is 30
														
								########################### TSCH-SDN ############################# 
								# update random seed in each iteration
								#python3 /home/fvg/contiki-ng/examples/sdn_udp/rand_seed_gen_py/main.py tsch-sdn
								
								java -Xshare:on -jar ../../tools/cooja/dist/cooja.jar -nogui=config.csc -contiki=../../	
								if [ -f "COOJA.testlog" ] 
								then
										cp COOJA.testlog /home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log
										mv /home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log/COOJA.testlog \
											 /home/fvg/contiki-ng/examples/sdn_udp/density-statistics/log/sdn-net-${NODE_NUM_LIST[j]}-itr-$i-lqr${LQR_LIST[k]}-radius-${RADIUS_LIST[m]}.testlog

								fi

								i=$(($i+1))	
													
						done
				
						m=$(($m+1))
						
				done
				
				k=$(($k+1))
				
		done	
		
		j=$(($j+1))	
						
done	
###################################################################################
echo "simulation is over!"


