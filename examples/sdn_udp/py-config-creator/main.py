import sys
import shutil
import fileinput
import os
from func_lib import *
from conf_tmp import *
import random
import numpy as np

user_home_path = os.path.expanduser('~')

print('====> INPUT PARAMITER    =   ', str(sys.argv))
param_input = sys.argv

sf_size = get_val_from_input_array(param_input, "sf_size")
sf_rep_period = get_val_from_input_array(param_input, "sf_rep_period")
ctrl_sf_size = get_val_from_input_array(param_input, "ctrl_sf_size")
node_num = get_val_from_input_array(param_input, "node_num")
eb_perid = get_val_from_input_array(param_input, "eb_perid")
server_num = get_val_from_input_array(param_input, "server_num")
client_bkg_num = get_val_from_input_array(param_input, "client_bkg_num")
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

sf_offs_list_6 = [1, 5, 3, 2, 6, 4]
sf_offs_list_12 = [1, 9, 5, 3, 10, 6, 2, 11, 7, 4, 12, 8]
sf_offs_list_18 = [1, 11, 9, 5, 12, 3, 13, 7, 14, 2, 15, 10, 6, 16, 4, 17, 8, 18]
sf_offs_list_24 = [1, 17, 9, 5, 18, 10, 3, 19, 11, 6, 20, 12, 2, 21, 13, 7, 22, 14, 4, 23, 15, 8, 24, 16]
sf_offs_list_30 = [1, 17, 9, 18, 5, 19, 10, 20, 3, 21, 11, 22, 7, 23, 15, 2, 24, 12, 25, 6, 26, 13, 27, 4, 28, 14, 29, 8, 30, 16]

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

replaceall(user_home_path + "/contiki-ng/os/net/mac/tsch/sdn/sdn-sink.c", "POS_ARRAY", "static float POS_ARRAY[" + str(node_num) +"][2] = " + str(cfile_pos_array) +';'+ '\n')

sf_offs_list = []
if node_num < 6:
    sf_offs_list = sf_offs_list_6
elif node_num < 10:
    sf_offs_list = sf_offs_list_12
elif node_num < 16:
    sf_offs_list = sf_offs_list_18
elif node_num < 21:
    sf_offs_list = sf_offs_list_24
elif node_num < 30:
    sf_offs_list = sf_offs_list_30

sf_offs_list_len = int((node_num/int(((eb_perid/10) / sf_size)) + 2)) * int(((eb_perid/10) / sf_size))
empty_list_to_append = [-1 for i in range(sf_offs_list_len - len(sf_offs_list))]
sf_offs_list = sf_offs_list + empty_list_to_append
print(sf_offs_list_len)
cfile_sf_offs_list = str(sf_offs_list)
cfile_sf_offs_list = cfile_sf_offs_list.replace('[', '{')
cfile_sf_offs_list = cfile_sf_offs_list.replace(']', '}')
print(' sf offs list', cfile_sf_offs_list)
#replaceall(user_home_path + "/contiki-ng/os/net/mac/tsch/sdn/sdn-sink.c", "shared_cell_shuffle_list", \
#           "static int shared_cell_shuffle_list[" + str(sf_offs_list_len) +"] = " + str(cfile_sf_offs_list) +';'+ '\n')

# #########################################################

msf_pos_array = ''
for index, pos in enumerate(position_array):
    msf_pos_array = msf_pos_array + '<pos' + str("{:02d}".format(index)) + '|' + str(pos[0]) + '|p' + str("{:02d}".format(index)) + '|' + str(pos[1]) + 'pos' + str("{:02d}".format(index)) + '> '

top_log = open('topo_graph', 'a')
top_log.write('pos'+'-net'+str(node_num)+'-lqr'+str(rx_success)+'-it'+str(itr_num)+'='+ msf_pos_array +'\n')
top_log.close()

num_shared_cell = 1
print("====> NODE's SHARED CELL NUM    =    ", num_shared_cell)

sdn_template = base_template
orch_template = base_template

# ################# create critical node ID list ##############

critic_id_list = list(range(2, node_num + 1))
bkg_id_list = []
# for i in range(client_critic_num):
#     find = 0
#     while find == 0:
#         r = random.randint(2, node_num)
#         if r in critic_id_list:
#             find = 0
#         else:
#             critic_id_list.append(r)
#             find = 1

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
                                      +str(ctrl_sf_size)+",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range) \
                                      +",NETWORK_SIZE="+str(node_num)+",NETWORK_RADIUS="+str(x_radius))

    if clint_bf_exis > 0:
        mote_type += replace_conf
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[1])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/sdn_udp/udp-client.c")
        mote_type = mote_type.replace("MAKE_COMMAND", "make udp-client.cooja TARGET=cooja DEFINES=SINK=0,APP_INT=" \
                                      +str(bkg_traffic_period)+",BF_TS_NUM="+str(bkg_timeslot_num)+",SDN_CONF_DATA_SLOTFRAME_SIZE=" \
                                      +str(sf_size)+",SDN_CONF_SF_REP_PERIOD="+str(sf_rep_period)+",SDN_CONF_CONTROL_SLOTFRAME_SIZE=" \
                                      +str(ctrl_sf_size)+",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range) \
                                      +",NETWORK_SIZE="+str(node_num)+",NETWORK_RADIUS="+str(x_radius))

    if clint_nbf_exis > 0:
        mote_type += replace_conf
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[2])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/sdn_udp/udp-client3.c")
        mote_type = mote_type.replace("MAKE_COMMAND", "make udp-client3.cooja TARGET=cooja DEFINES=SINK=0,APP_INT=" \
                                      +str(critic_traffic_period)+",BF_TS_NUM="+str(bkg_timeslot_num)+",SDN_CONF_DATA_SLOTFRAME_SIZE=" \
                                      +str(sf_size)+",SDN_CONF_SF_REP_PERIOD="+str(sf_rep_period)+",SDN_CONF_CONTROL_SLOTFRAME_SIZE=" \
                                      +str(ctrl_sf_size)+",SDN_CONF_SIMULATION_RX_SUCCESS="+str(rx_success)+",TX_RANGE="+str(tx_range) \
                                      +",NETWORK_SIZE="+str(node_num)+",NETWORK_RADIUS="+str(x_radius))

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


################################################# TSCH-SDN ##################################################
sdn_template = set_radio_medium("QAZWSX", radio_medium)
sdn_template = set_mote_type("QWERTY", mote_type_id, server_num, client_bkg_num, client_critic_num)
sdn_template = add_motes("ASDFGH", mote_spec)
sdn_template = add_mote_id("ZXCVBN", mote_id)
sdn_template = set_simulation_time("EDCRFV")
sdn_template = update_rand_seed()

file_w = open(user_home_path + "/contiki-ng/examples/sdn_udp/py-config-creator/config.csc", "w+")
file_w.write(sdn_template)
file_w.close()

if os.path.exists(user_home_path + "/contiki-ng/examples/sdn_udp/config.csc"):
    os.remove(user_home_path + "/contiki-ng/examples/sdn_udp/config.csc")
else:
    print("config.csc does not exist in directory.../sdn_udp/")
newPath = shutil.copy(user_home_path + "/contiki-ng/examples/sdn_udp/py-config-creator/config.csc", user_home_path + "/contiki-ng/examples/sdn_udp")
print("====> CONFIG-SDN file is created and added to .../sdn_udp/ directory")

newPath = shutil.copy(user_home_path + "/contiki-ng/examples/sdn_udp/topo_graph", user_home_path + "/msf-contiki/contiki-ng/examples/6tisch/msf")

# ###########################################################################################################

def set_orch_mote_type(conf_key, replace_conf_srv, replace_conf_client, replace_critic_client, srv_exis, clint_bf_exis, clint_nbf_exis):
    mote_type = ''
    cooja_mtype = 0
    if srv_exis > 0:
        mote_type += replace_conf_srv
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[0])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/orch_udp/udp-server.c")


    if clint_bf_exis > 0:
        mote_type += replace_conf_client
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[1])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/orch_udp/udp-client.c")

    if clint_nbf_exis > 0:
        mote_type += replace_critic_client
        cooja_mtype += 1
        mote_type = mote_type.replace("MTYPE_NUM", mtype_num[2])
        mote_type = mote_type.replace("COOJA_MOTE_TYPE", "Cooja Mote Type #" + str(cooja_mtype))
        mote_type = mote_type.replace("FIRMWAR_DIR", "[CONTIKI_DIR]/examples/orch_udp/udp-client-critic.c")

    return orch_template.replace(conf_key, mote_type)

def add_orch_motes(conf_key, replace_conf):
    mote_type = ''
    node_id = 0
    if conf_key in orch_template:
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

        return orch_template.replace(conf_key, mote_type)


def add_orch_mote_id(conf_key, replace_conf):
    mote_type = ''
    node_id = -1
    if conf_key in orch_template:
        for i in range(node_num):
            mote_type += replace_conf
            node_id += 1
            mote_type = mote_type.replace("MOTE_ID2", str(node_id))

        return orch_template.replace(conf_key, mote_type)

def set_orch_radio_medium(conf_key, replace_conf):
    mote_type = ''
    if conf_key in orch_template:
        mote_type += replace_conf
        mote_type = mote_type.replace("TX_RANGE", str(tx_range))
        mote_type = mote_type.replace("INT_RANGE", str(intf_range))
        mote_type = mote_type.replace("TX_SUCCESS", str(tx_success))
        mote_type = mote_type.replace("RX_SUCCESS", str(rx_success))

        return orch_template.replace(conf_key, mote_type)

def set_orch_simulation_time(conf_key):
    if conf_key in orch_template:
        return orch_template.replace(conf_key, str(sim_time_orch))
############################################ TSCH-ORCHESTRA ################################################
orch_template = set_orch_radio_medium("QAZWSX", radio_medium)
orch_template = set_orch_mote_type("QWERTY", mote_type_id_orch_srv, mote_type_id_orch_client, mote_type_id_orch_client_critic, server_num, client_bkg_num, client_critic_num)
orch_template = add_orch_motes("ASDFGH", mote_spec)
orch_template = add_orch_mote_id("ZXCVBN", mote_id)
orch_template = set_orch_simulation_time("EDCRFV")
file_w = open(user_home_path + "/contiki-ng/examples/sdn_udp/py-config-creator/config_orch_sf.csc", "w+")
file_w.write(orch_template)
file_w.close()

if os.path.exists(user_home_path + "/contiki-ng/examples/sdn_udp/config_orch_sf.csc"):
    os.remove(user_home_path + "/contiki-ng/examples/sdn_udp/config_orch_sf.csc")
else:
    print("config.csc does not exist in directory.../sdn_udp/")
newPath = shutil.copy(user_home_path + "/contiki-ng/examples/sdn_udp/py-config-creator/config_orch_sf.csc", user_home_path + "/contiki-ng/examples/sdn_udp")
print("====> CONFIG-ORCHESTRA file is created and added to .../sdn_udp/ directory")

if os.path.exists(user_home_path + "/contiki-ng/examples/sdn_udp/config_orch.csc"):
    os.remove(user_home_path + "/contiki-ng/examples/sdn_udp/config_orch.csc")

file = open(user_home_path + "/contiki-ng/examples/sdn_udp/config_orch.csc", "w+")
file.close()
############################################################################################################

