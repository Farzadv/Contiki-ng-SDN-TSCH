#!/bin/bash


#chmod +x ~/contiki-ng/examples/sdn_udp/py-config-creator/main.py 

# bkg_ts must be devidible to SF size
# must be carefull about number of timeslots ==> lower "SDN_DATA_SLOTFRAME_SIZE/SDN_SF_REP_PERIOD" exp: 3000/100 = 30 max ts

# 1- traffic period must be multiplication of SF_REP_PERIOD. for exp: sf_size = 3001, SF_REP_PERIOD=100 => traffic periods: 100, 300,
# 500, 600, 1000, 1500, 3000
# 2- also traffic period must be devidible by (SF_REP_NUM * SF_REP_PERIOD)

#########################################################################

# create sim-plot directory
if [ -d "~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log" ] 
then
    rm -r ~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log
fi


if [ ! -d "~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log" ] 
then
    mkdir mkdir ~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log
fi



# remove COOJA.testlog file
if [ -f "COOJA.testlog" ] 
then
		rm COOJA.testlog
fi


# remove topo graph file
if [ -f "topo_graph" ] 
then
		rm topo_graph
fi
######################################################################### 
NODE_NUM_LIST=(10)                        # network size included Sink
NODE_CRITIC_NUM=(9)                       # number of critic nodes
BKG_TS_NUM=(1)                           # number of besteffort timeslot that each node has
ORCH_SF_LEN=(101)                 # orch sf size 
LQR_LIST=(0.84)
ITER_PER_CONF=1                           # number of iteration for each config.csc
SH_CELL_COEF_LIST=(0.4)
SRVR_NUM=1                                # number of server


LIST_SIZE=${#NODE_NUM_LIST[*]}            # traffic array len
BKG_TS_LIST_SIZE=${#BKG_TS_NUM[*]}        # traffic array len
LQR_LIST_LEN_SIZE=${#LQR_LIST[*]}       # orch sf list len
ORCH_SF_LEN_SIZE=${#ORCH_SF_LEN[*]}       # orch sf list len
SH_CELL_COEF_SIZE=${#SH_CELL_COEF_LIST[*]}       # orch sf list len

echo "LIST_SIZE val $LIST_SIZE"

n=0
while [[ $n -le $(($SH_CELL_COEF_SIZE-1)) ]]
do
		j=0
		while [[ $j -le $(($LIST_SIZE-1)) ]]
		do 
				k=0
				while [[ $k -le $(($LQR_LIST_LEN_SIZE-1)) ]]
				do									
						# repeat a given simulation for n times with different seed
						i=1
						while [[ $i -le $ITER_PER_CONF ]]
						do
								python3 ~/contiki-ng/examples/sdn_udp/optimal-shared-cell/main.py \
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
								sh_cell_coef=[${SH_CELL_COEF_LIST[n]}] \
								tx_range=[50.0] \
								intf_range=[100.0] \
								tx_success=[1.0] \
								rx_success=[${LQR_LIST[k]}] \
								x_radius=[$((${NODE_NUM_LIST[j]}*360/100))] \
								y_radius=[$((${NODE_NUM_LIST[j]}*360/100))] \
								sim_time_sdn=[8000000] \
								itr_num=[$i]   # 50 min sim len and ASN time is 30
														
								########################### TSCH-SDN ############################# 
								# update random seed in each iteration
								#python3 ~/contiki-ng/examples/sdn_udp/rand_seed_gen_py/main.py tsch-sdn
								
								java -Xshare:on -jar ../../tools/cooja/dist/cooja.jar -nogui=config.csc -contiki=../../	
								if [ -f "COOJA.testlog" ] 
								then
										cp COOJA.testlog ~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log
										mv ~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log/COOJA.testlog \
											 ~/contiki-ng/examples/sdn_udp/optimal-shared-cell-statis/log/sdn-net-${NODE_NUM_LIST[j]}-itr-$i-lqr${LQR_LIST[k]}-shc${SH_CELL_COEF_LIST[n]}.testlog

								fi

								i=$(($i+1))	
													
						done
						
						k=$(($k+1))
						
				done	
				
				j=$(($j+1))	
								
		done 
		
		n=$(($n+1))
		
done
###################################################################################
echo "simulation is over!"


