import sys
import shutil
import fileinput
import os
from func_lib import *
from conf_tmp import *
import random


print('====> INPUT PARAMITER    =   ', str(sys.argv))
param_input = sys.argv

sf_size = get_val_from_input_array(param_input, "sf_size")
sf_rep_period = get_val_from_input_array(param_input, "sf_rep_period")
ctrl_sf_size = get_val_from_input_array(param_input, "ctrl_sf_size")
node_num = get_val_from_input_array(param_input, "node_num")
server_num = get_val_from_input_array(param_input, "server_num")
client_bkg_num = get_val_from_input_array(param_input, "client_bkg_num")
sh_cell_coef = get_val_from_input_array(param_input, "sh_cell_coef")
client_critic_num = get_val_from_input_array(param_input, "client_critic_num")
bkg_traffic_period = get_val_from_input_array(param_input, "bkg_traffic_period")  # second
critic_traffic_period = get_val_from_input_array(param_input, "critic_traffic_period")  # second
bkg_timeslot_num = get_val_from_input_array(param_input, "bkg_timeslot_num")
tx_range = get_val_from_input_array(param_input, "tx_range")  # meter
intf_range = get_val_from_input_array(param_input, "intf_range")  # meter
tx_success = get_val_from_input_array(param_input, "tx_success")
rx_success = get_val_from_input_array(param_input, "rx_success")
x_radius = get_val_from_input_array(param_input, "x_radius")  # meter
y_radius = get_val_from_input_array(param_input, "y_radius")  # meter
sim_time_sdn = get_val_from_input_array(param_input, "sim_time_sdn")  # sdn-simulation time in ms
sim_time_orch = get_val_from_input_array(param_input, "sim_time_orch")  # orch-simulation time in ms
itr_num = get_val_from_input_array(param_input, "itr_num")  # orch-simulation time in ms

mtype_num = []

if server_num != 0:
    mtype_num.append("mtype" + str(random.randint(100, 1000)))
else:
    mtype_num.append("zero_val")

if client_bkg_num != 0:
    mtype_num.append("mtype" + str(random.randint(100, 1000)))
else:
    mtype_num.append("zero_val")

if client_critic_num != 0:
    mtype_num.append("mtype" + str(random.randint(100, 1000)))
else:
    mtype_num.append("zero_val")

print("====> MTYPE VALUES    =    ", mtype_num)

position_array = []
fail_cntr = 0
while len(position_array) < node_num:
    fail_cntr += 1
    print("position array fail!"+str(fail_cntr))
    position_array = create_network_graph(node_num, x_radius, y_radius, tx_range)

print("====> NODE's LOCATION ARRAY    =    ", position_array)

# #########################################################
new_pos_array = []
for i in position_array:
    temp = []
    for j in i:
        temp.append(round(j, 2))
    new_pos_array.append(temp)

cfile_pos_array = str(new_pos_array)
cfile_pos_array = cfile_pos_array.replace('[', '{')
cfile_pos_array = cfile_pos_array.replace(']', '}')


def replaceall(file, searchexp, replaceexp):
    find = 0
    for line in fileinput.input(file, inplace=1):
        tem_line = ''
        if searchexp in line and find == 0:
            tem_line = replaceexp
            find = 1
        else:
            tem_line = line
        sys.stdout.write(tem_line)

replaceall("/home/fvg/contiki-ng/os/net/mac/tsch/sdn/sdn-sink.c", "POS_ARRAY", "static float POS_ARRAY[" + str(node_num) +"][2] = " + str(cfile_pos_array) +';'+ '\n')

# #########################################################

msf_pos_array = ''
for index, pos in enumerate(position_array):
    msf_pos_array = msf_pos_array + '<pos' + str("{:02d}".format(index)) + '|' + str(pos[0]) + '|p' + str("{:02d}".format(index)) + '|' + str(pos[1]) + 'pos' + str("{:02d}".format(index)) + '> '

top_log = open('topo_graph', 'a')
top_log.write('pos'+'-net'+str(node_num)+'-lqr'+str(rx_success)+'-it'+str(itr_num)+'='+ msf_pos_array +'\n')
top_log.close()

num_shared_cell = int(math.ceil(sh_cell_coef * node_num))
print("====> NODE's SHARED CELL NUM    =    ", num_shared_cell)

sdn_template = base_template
orch_template = base_template

##################### create critical node ID list ################
critic_id_list = []
bkg_id_list = []
for i in range(client_critic_num):
    find = 0
    while find == 0:
        r = random.randint(2, node_num)
        if r in critic_id_list:
            find = 0
        else:
            critic_id_list.append(r)
            find = 1

print("critic_id_list  ====> "+str(critic_id_list))

top_log_id = open('topo_graph', 'a')
top_log_id.write('nid-crit'+'-net'+str(node_num)+'-lqr'+str(rx_success)+'-it'+str(itr_num)+'='+str(critic_id_list)+'\n')
top_log_id.close()

for i in range(node_num -1):
    bkg_id_list.append(i+2)

for i in critic_id_list:
    if i in bkg_id_list:
        bkg_id_list.remove(i)

print("bkg_id_list ====> "+str(bkg_id_list))
top_log_id = open('topo_graph', 'a')
top_log_id.write('nid-bkg'+'-net'+str(node_num)+'-lqr'+str(rx_success)+'-it'+str(itr_num)+'='+str(bkg_id_list)+'\n')
top_log_id.close()
###################################################


def set_mote_type(conf_key, replace_conf, srv_exis, clint_bf_exis, clint_nbf_exis):
    mote_type = ''
    cooja_mtype = 0
    if srv_exis > 0:
        mote_type += replace_conf
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[0])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/sdn_udp/udp-server.c")
        mote_type = mote_type.replace("MAKE_COMMAND", "make udp-server.cooja TARGET=cooja DEFINES=SINK=1"+",BF_TS_NUM=" \
                                      +str(bkg_timeslot_num)+",SDN_CONF_DATA_SLOTFRAME_SIZE="+str(sf_size) \
                                      +",SDN_CONF_SF_REP_PERIOD="+str(sf_rep_period)+",SDN_CONF_CONTROL_SLOTFRAME_SIZE=" \
                                      +str(ctrl_sf_size)+",SDN_CONF_NUM_SHARED_CELL_IN_REP="+str(num_shared_cell) \
                                      +",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range))

    if clint_bf_exis > 0:
        mote_type += replace_conf
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[1])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/sdn_udp/udp-client.c")
        mote_type = mote_type.replace("MAKE_COMMAND", "make udp-client.cooja TARGET=cooja DEFINES=SINK=0,APP_INT=" \
                                      +str(bkg_traffic_period)+",BF_TS_NUM="+str(bkg_timeslot_num)+",SDN_CONF_DATA_SLOTFRAME_SIZE=" \
                                      +str(sf_size)+",SDN_CONF_SF_REP_PERIOD="+str(sf_rep_period)+",SDN_CONF_CONTROL_SLOTFRAME_SIZE=" \
                                      +str(ctrl_sf_size)+",SDN_CONF_NUM_SHARED_CELL_IN_REP="+str(num_shared_cell) \
                                      +",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range))

    if clint_nbf_exis > 0:
        mote_type += replace_conf
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[2])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/sdn_udp/udp-client3.c")
        mote_type = mote_type.replace("MAKE_COMMAND", "make udp-client3.cooja TARGET=cooja DEFINES=SINK=0,APP_INT=" \
                                      +str(critic_traffic_period)+",BF_TS_NUM="+str(bkg_timeslot_num)+",SDN_CONF_DATA_SLOTFRAME_SIZE=" \
                                      +str(sf_size)+",SDN_CONF_SF_REP_PERIOD="+str(sf_rep_period)+",SDN_CONF_CONTROL_SLOTFRAME_SIZE=" \
                                      +str(ctrl_sf_size)+",SDN_CONF_NUM_SHARED_CELL_IN_REP="+str(num_shared_cell) \
                                      +",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range))

    return sdn_template.replace(conf_key, mote_type)


def add_motes(conf_key, replace_conf):
    mote_type = ''
    node_id = 0
    if conf_key in sdn_template:
        p = -1
        for i in range(server_num):
            mote_type += replace_conf
            node_id += 1
            p += 1
            mote_type = mote_type.replace("X_POSITION", str(position_array[p][0]))
            mote_type = mote_type.replace("Y_POSITION", str(position_array[p][1]))
            mote_type = mote_type.replace("Z_POSITION", str(0.0))
            mote_type = mote_type.replace("MOTE_ID", str(node_id))
            mote_type = mote_type.replace("MOT_TYPE", mtype_num[0])

        for i in bkg_id_list:
            mote_type += replace_conf
            mote_type = mote_type.replace("X_POSITION", str(position_array[i-1][0]))
            mote_type = mote_type.replace("Y_POSITION", str(position_array[i-1][1]))
            mote_type = mote_type.replace("Z_POSITION", str(0.0))
            mote_type = mote_type.replace("MOTE_ID", str(i))
            mote_type = mote_type.replace("MOT_TYPE", mtype_num[1])

        for i in critic_id_list:
            mote_type += replace_conf
            mote_type = mote_type.replace("X_POSITION", str(position_array[i-1][0]))
            mote_type = mote_type.replace("Y_POSITION", str(position_array[i-1][1]))
            mote_type = mote_type.replace("Z_POSITION", str(0.0))
            mote_type = mote_type.replace("MOTE_ID", str(i))
            mote_type = mote_type.replace("MOT_TYPE", mtype_num[2])

        return sdn_template.replace(conf_key, mote_type)


def add_mote_id(conf_key, replace_conf):
    mote_type = ''
    node_id = -1
    if conf_key in sdn_template:
        for i in range(node_num):
            mote_type += replace_conf
            node_id += 1
            mote_type = mote_type.replace("MOTE_ID2", str(node_id))

        return sdn_template.replace(conf_key, mote_type)


def set_radio_medium(conf_key, replace_conf):
    mote_type = ''
    if conf_key in sdn_template:
        mote_type += replace_conf
        mote_type = mote_type.replace("TX_RANGE", str(tx_range))
        mote_type = mote_type.replace("INT_RANGE", str(intf_range))
        mote_type = mote_type.replace("TX_SUCCESS", str(tx_success))
        mote_type = mote_type.replace("RX_SUCCESS", str(rx_success))

        return sdn_template.replace(conf_key, mote_type)


def set_simulation_time(conf_key):
    if conf_key in sdn_template:
        return sdn_template.replace(conf_key, str(sim_time_sdn))


def update_rand_seed():
    key_word = "<randomseed>"
    rand_seed = random.randint(1, 9)
    for i in range(5):
        rand_seed = (rand_seed * 10) + random.randint(1, 9)

    replace_word = "<randomseed>" + str(rand_seed) + "</randomseed>"
    top_log_seed = open('topo_graph', 'a')
    top_log_seed.write('seed' + '-net' + str(node_num) + '-lqr' + str(rx_success) + '-it' + str(itr_num) + '=[' + str(
        rand_seed) + ']\n')
    top_log_seed.close()

    return sdn_template.replace(key_word, replace_word)

# ################################################ TSCH-SDN ##################################################

sdn_template = set_radio_medium("QAZWSX", radio_medium)
sdn_template = set_mote_type("QWERTY", mote_type_id, server_num, client_bkg_num, client_critic_num)
sdn_template = add_motes("ASDFGH", mote_spec)
sdn_template = add_mote_id("ZXCVBN", mote_id)
sdn_template = set_simulation_time("EDCRFV")
sdn_template = update_rand_seed()

file_w = open("/home/fvg/contiki-ng/examples/sdn_udp/optimal-shared-cell/config.csc", "w+")
file_w.write(sdn_template)
file_w.close()

if os.path.exists("/home/fvg/contiki-ng/examples/sdn_udp/config.csc"):
    os.remove("/home/fvg/contiki-ng/examples/sdn_udp/config.csc")
else:
    print("config.csc does not exist in directory.../sdn_udp/")
newPath = shutil.copy('/home/fvg/contiki-ng/examples/sdn_udp/optimal-shared-cell/config.csc', '/home/fvg/contiki-ng/examples/sdn_udp')
print("====> CONFIG-SDN file is created and added to .../sdn_udp/ directory")

# newPath = shutil.copy('/home/fvg/contiki-ng/examples/sdn_udp/topo_graph', '/home/fvg/msf-contiki/contiki-ng/examples/6tisch/msf')