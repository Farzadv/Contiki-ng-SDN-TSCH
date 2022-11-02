import math
import os
import numpy as np
import scipy.stats as st
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from statistics import mean
# import glob
import numpy as np

user_home_path = os.path.expanduser('~')

std_state_delay_df = pd.DataFrame(columns=['End-to-End delay(s)', 'Network Solution', 'Network size'])
convergence_delay_df = pd.DataFrame(columns=['End-to-End delay(s)', 'Network Solution', 'Network size'])
flow_config_time_df = pd.DataFrame(columns=['Flow configuration time(s)', 'Network Solution', 'Network size'])
convergence_df = pd.DataFrame(columns=['Convergence time(s)', 'Network Solution', 'Network size'])
ts_num_df = pd.DataFrame(columns=['Normalized scheduled cells', 'Network Solution', 'Network size'])
collision_rate_df = pd.DataFrame(columns=['Normalized link quality estimation', 'Network Solution', 'Network size'])
energy_consumption_joined_df = pd.DataFrame(columns=['Energy consumption (mJ)', 'Network Solution', 'Network size'])
energy_consumption_bootstrap_df = pd.DataFrame(columns=['Energy consumption (mJ)', 'Network Solution', 'Network size'])
pdr_df = pd.DataFrame(columns=['PDR(%) before deadline', 'Network Solution', 'Network size'])
num_tx_df = pd.DataFrame(columns=['Number of TX+RTX', 'Network Solution', 'Network size'])
cause_rtx_df = pd.DataFrame(columns=['Packet_tx_fail', 'Ack_tx_fail', 'Network size'])
sf_max_cell_usage_df = pd.DataFrame(columns=['Max cell usage per SF and node', 'Network Solution', 'Network size'])
never_used_cell_df = pd.DataFrame(columns=['Number of never used cells', 'Network Solution', 'Network size'])

app_int = 5.02
net_size_array = [30]
lqr_array = [0.0]
simu_len = 500
link_quality_coeff = 0.9025
sf_rep = 3
sf_size = 1506
print_asn = int('0xCF850', 16)

sent_asn_start_sing = 's|as '
sent_asn_end_sign = ' as|ar'
recv_asn_start_sing = '|ar '
recv_asn_end_sing = ' ar|c'
seq_start_sing = 'd|s '
seq_end_sing = ' s|as'
addr_src_start_sing = '|sr '
addr_src_end_sing = ' sr|'
sent_port_start_sing = 'c|p '
sent_port_end_sing = ' p|'

app_addr_src_start_sing = '|v'
app_addr_src_end_sing = '|t'
app_seq_start_sing = '|o'
app_seq_end_sing = '|n'
app_tx_status_start_sing = '|n'
app_tx_status_end_sing = 'c|'

app_rx_addr_src_start_sing = '|s'
app_rx_addr_src_end_sing = '|d'
app_rx_seq_start_sing = '|n'
app_rx_seq_end_sing = '|j'

flow_req_start_sing = 'SDN-FLOW-REQUEST: request a critical fid = ['
flow_req_end_sing = '] from'
flow_resp_start_sing = 'SDN-FLOW-REQUEST: add a new app flow id, addr = ['
flow_resp_end_sing = '], RECEIVE_ASN'
flow_req_asn_sing_start = 'SEND_ASN=['
flow_req_asn_sing_end = ']]]]]'
flow_resp_asn_sing_start = 'RECEIVE_ASN=['
flow_resp_asn_sing_end = ']]]]]'

msf_filename_key = 'msf-net-'
sdn_filename_key = 'sdn-net-'
shared_filename_key = 'sharedcp-net-'
shared_fctrl_filename_key = 'sharedfctrl-net-'
stupid_filename_key = 'unconteb-net-'


sf_ts_num_start_sing = '*** sf_size[['
sf_ts_num_end_sing = ']] Link Options'
sf_ts_num_start_sing2 = '] total scheduled cell['
sf_ts_num_end_sing2 = '], shared cells['

num_hop_start_sing = 'CONTROLLER: HOP_TO_DEST: '
num_hop_end_sing = ', ADDR:[['
num_hop_addr_start_sing = ', ADDR:[['
num_hop_addr_end_sing = ']]'

max_cell_usage_start = '[MAX sf cell usage: '
max_cell_usage_end = ' node_addr '

log_dir = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log"
logfile_list = os.listdir(log_dir)
log_list = sorted(logfile_list)


def get_list_of_log_file(filename_key, list_to_search):
    net_log_list = []
    for i in list_to_search:
        if filename_key in i:
            net_log_list.append(i)
    # print(net_log_list)
    return net_log_list


def e2e_delay(log_file_name, end_node_id):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    key_line = "11111111 "
    packet_num = 1000
    lat_critic_array1 = [[None for i in range(packet_num)] for j in range(end_node_id)]
    bounded_lat_array = [[None for i in range(packet_num)] for j in range(end_node_id)]
    node_id_list = []
    for line in input_lines:
        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
        crtc_end_sing = line.find(addr_src_end_sing)

        if key_line in line:
            s = line[crtc_start_sing:crtc_end_sing]
            node_addr = int(s[1:])
            if node_addr not in node_id_list:
                node_id_list.append(node_addr)

            sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
            sent_asn_e = line.find(sent_asn_end_sign)
            rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
            rcv_asn_e = line.find(recv_asn_end_sing)
            seq_s = line.find(seq_start_sing) + len(seq_start_sing)
            seq_e = line.find(seq_end_sing)
            sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
            sent_port_e = line.find(sent_port_end_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
                if lat_critic_array1[node_addr - 1][int(line[seq_s:seq_e]) - 1] == None:
                    lat_critic_array1[node_addr - 1][int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], \
                                                                                        16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10
                    if ((int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10) < 2520:
                        bounded_lat_array[node_addr - 1][int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], \
                                                                                        16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10
                    # else:
                    #     print('larg delay: node id: ' + str(node_addr) + ', seq: ' + str(int(line[seq_s:seq_e])) + ', ' +log_file_name)

                    # if ((int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10) > 4000:
                    #     print('pick delay: node id: ' + str(node_addr) + ', seq: ' + str(
                    #         int(line[seq_s:seq_e])) + ', ' + log_file_name)


    pdr = []
    # std_delay = []
    for id in node_id_list:
        # print('pdr of node ' + str(id) + ': ' + str((((len(lat_critic_array1[id - 1]) - lat_critic_array1[id - 1].count(None)) / packet_num) * 100)))
        # print('delay of each flow: ', id, lat_critic_array1[id - 1])
        pdr.append((((len(bounded_lat_array[id - 1]) - bounded_lat_array[id - 1].count(None)) / packet_num) * 100))
        if (((len(bounded_lat_array[id - 1]) - bounded_lat_array[id - 1].count(None)) / packet_num) * 100) < 99:
            print('low pdr for node:', id, 'bounded delay: ',bounded_lat_array[id - 1])
        # std_delay.append(mean(d for d in lat_critic_array1[id - 1] if d is not None))
        # print(mean(d for d in lat_critic_array1[id - 1] if d is not None))

    # l = [i for i, v in enumerate(lat_critic_array1[16]) if v == None]
    # print(l)
    return lat_critic_array1[1:], pdr, node_id_list


def msf_e2e_delay(log_file_name, end_node_id):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    key_line = "11111111 "
    packet_num = 1000
    lat_critic_array1 = [[None for i in range(packet_num)] for j in range(end_node_id)]
    node_id_list = []

    for line in input_lines:
        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
        crtc_end_sing = line.find(addr_src_end_sing)

        if key_line in line:
            s = line[crtc_start_sing:crtc_end_sing]
            node_addr = int(s[1:])
            if node_addr not in node_id_list:
                node_id_list.append(node_addr)

            sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
            sent_asn_e = line.find(sent_asn_end_sign)
            rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
            rcv_asn_e = line.find(recv_asn_end_sing)
            seq_s = line.find(seq_start_sing) + len(seq_start_sing)
            seq_e = line.find(seq_end_sing)
            sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
            sent_port_e = line.find(sent_port_end_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
                if lat_critic_array1[node_addr - 1][int(line[seq_s:seq_e]) - 1] == None:
                    lat_critic_array1[node_addr - 1][int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10

    # l = [i for i, v in enumerate(lat_critic_array1[16]) if v == None]
    # print(l)
    # print('MSF node id list number: ' + str(len(node_id_list)))
    return lat_critic_array1, node_id_list


def sdn_get_num_scheduled_ts(log_file_name, net_size):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    total_ts = 0
    ideal_cell_num = 0
    sdn_cell_num = 0
    ideal_cell_num_sign1 = 'ideal cell number:['
    ideal_cell_num_sign2 = '], estimated cell num:['
    ideal_cell_num_sign3 = '] for LQR:'
    for line in input_lines:
        if ideal_cell_num_sign1 in line:
            total_ts += 1
            s = line.find(ideal_cell_num_sign1) + len(ideal_cell_num_sign1)
            e = line.find(ideal_cell_num_sign2)
            ideal_cell_num = ideal_cell_num + int(line[s:e])

            s = line.find(ideal_cell_num_sign2) + len(ideal_cell_num_sign2)
            e = line.find(ideal_cell_num_sign3)
            sdn_cell_num = sdn_cell_num + int(line[s:e])
    return sdn_cell_num/ideal_cell_num, ideal_cell_num

    # for line in input_lines:
    #     if sf_ts_num_start_sing in line:
    #         total_ts += 1
    #     elif sf_ts_num_start_sing2 in line:
    #         s = line.find(sf_ts_num_start_sing2) + len(sf_ts_num_start_sing2)
    #         e = line.find(sf_ts_num_end_sing2)
    #         total_ts = total_ts + int(line[s:e])
    # return total_ts/net_size


def msf_get_num_scheduled_ts(log_file_name, net_size, rx_success):

    sdn_file_name = str(log_file_name).replace("msf", "sdn")
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(sdn_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    ideal_cell_num = 0
    sdn_cell_num = 0
    ideal_cell_num_sign1 = 'ideal cell number:['
    ideal_cell_num_sign2 = '], estimated cell num:['
    ideal_cell_num_sign3 = '] for LQR:'
    for line in input_lines:
        if ideal_cell_num_sign1 in line:
            s = line.find(ideal_cell_num_sign1) + len(ideal_cell_num_sign1)
            e = line.find(ideal_cell_num_sign2)
            ideal_cell_num = ideal_cell_num + int(line[s:e])

    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    total_ts = 0
    cell_num_sign1 = 'total scheduled cell['
    cell_num_sign2 = '], shared cells['
    for line in input_lines:
        if cell_num_sign1 in line:
            s = line.find(cell_num_sign1) + len(cell_num_sign1)
            e = line.find(cell_num_sign2)
            total_ts = total_ts + (int(line[s:e]) - 2)

    print(total_ts)
    print(ideal_cell_num * 2)

    return total_ts / (ideal_cell_num * 2), (ideal_cell_num * 2)



#
# def msf_get_num_scheduled_ts(log_file_name, net_size, rx_success):
#     position_array = []
#     pos_log = open(user_home_path + "/contiki-ng/examples/sdn_udp/topo_graph", "r+")
#     input_lines = pos_log.readlines()
#     pos_log.close()
#
#     s = log_file_name.find('itr-') + len('itr-')
#     e = log_file_name.find('-lqr')
#     itr = int(log_file_name[s:e])
#
#     pos_log_key = 'pos-net' + str(net_size) + '-lqr' + str(rx_success) + '-it' + str(itr) + '='
#     for line in input_lines:
#         if pos_log_key in line:
#             for id in range(net_size):
#                 position = []
#                 pos_s = '<pos' + str("{:02d}".format(id)) + '|'
#                 pos_m = '|p' + str("{:02d}".format(id)) + '|'
#                 pos_e = 'pos' + str("{:02d}".format(id)) + '>'
#
#                 pos1_s = line.find(pos_s) + len(pos_s)
#                 pos1_e = line.find(pos_m)
#
#                 pos2_s = line.find(pos_m) + len(pos_m)
#                 pos2_e = line.find(pos_e)
#
#                 position.append(float(line[pos1_s:pos1_e]))
#                 position.append(float(line[pos2_s:pos2_e]))
#
#                 position_array.append(position)
#     print("====> NODE's LOCATION ARRAY    =    ", position_array)
#
#     log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
#     file = open(log_path, "r")
#     input_lines = file.readlines()
#     file.close()
#     total_ts = 0
#     ideal_cell_num = 0
#     sdn_cell_num = 0
#     ideal_cell_num_sign1 = 'ideal cell: node_id:[0'
#     ideal_cell_num_sign2 = '] parent:[0'
#     ideal_cell_num_sign3 = '] rank['
#     parent_array = [None for i in range(net_size)]
#
#     for line in input_lines:
#         if ideal_cell_num_sign1 in line:
#             s = line.find(ideal_cell_num_sign1) + len(ideal_cell_num_sign1)
#             e = line.find(ideal_cell_num_sign2)
#             id = int(line[s:e])
#             s = line.find(ideal_cell_num_sign2) + len(ideal_cell_num_sign2)
#             e = line.find(ideal_cell_num_sign3)
#             parent_array[id - 1] = int(line[s:e])
#
#     for id in range(1, net_size + 1):
#         reach_root = 0
#         curr_parent = id
#         new_parent = parent_array[id - 1]
#         while reach_root == 0:
#             curr_node = curr_parent
#             curr_parent = new_parent
# # calculate PDR of each link
# # calculate end to end pdr given QOS PDR
#     return 5

    # return sdn_cell_num/ideal_cell_num

def get_max_cell_usage_per_sf(log_file_name, end_node_id):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    sum_max_cell_usage = 0

    for line in input_lines:
        # calculate max cell sf usage:
        cell_use_start = line.find(max_cell_usage_start) + len(max_cell_usage_start)
        cell_use_end = line.find(max_cell_usage_end)
        if max_cell_usage_start in line:
            sum_max_cell_usage = sum_max_cell_usage + int(line[cell_use_start:cell_use_end])

    return sum_max_cell_usage/end_node_id


def number_of_tx_and_collision(log_file_name, end_node_id):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    key_line_tx = "} !!|"
    key_line_rx = '!!RX!!'
    key_line_rx_sink = "11111111 "
    packet_num = 1000
    num_tx_array = [0] * packet_num
    packet_intf_array = [0] * packet_num
    ack_intf_array = [0] * packet_num
    queue_drop_array = [0] * packet_num
    tx_ok_array = [0] * packet_num
    rx_num_receive_array = [0] * packet_num
    sink_rx_success_array = [0] * packet_num
    num_hop = 0
    sum_max_cell_usage = 0
    list_max_cell_usage = []
    sum_nver_used_cell = 0
    list_nver_used_cell = []
    count_nver_used_cell = 0
    num_cell_usage = 0
    never_used_cell_sign1 = 'Num cell never used: '
    never_used_cell_sign2 = ' node_addr'

    for line in input_lines:
        # calculate max cell sf usage:
        cell_use_start = line.find(max_cell_usage_start) + len(max_cell_usage_start)
        cell_use_end = line.find(max_cell_usage_end)
        if max_cell_usage_start in line:
            sum_max_cell_usage = sum_max_cell_usage + int(line[cell_use_start:cell_use_end])
            num_cell_usage += 1
            list_max_cell_usage.append(int(line[cell_use_start:cell_use_end]))

    for line in input_lines:
        # calculate max cell sf usage:
        cell_use_start = line.find(never_used_cell_sign1) + len(never_used_cell_sign1)
        cell_use_end = line.find(never_used_cell_sign2)
        if never_used_cell_sign1 in line:
            list_nver_used_cell.append(int(line[cell_use_start:cell_use_end]))
            sum_nver_used_cell = sum_nver_used_cell + int(line[cell_use_start:cell_use_end])
            count_nver_used_cell += 1

    #     # check sink node receive the seq-num ?
    #     crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
    #     crtc_end_sing = line.find(addr_src_end_sing)
    #     if key_line_rx_sink in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
    #         seq_s = line.find(seq_start_sing) + len(seq_start_sing)
    #         seq_e = line.find(seq_end_sing)
    #         sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
    #         sent_port_e = line.find(sent_port_end_sing)
    #         if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
    #             sink_rx_success_array[int(line[seq_s:seq_e]) - 1] = 1
    #
    #     # calculate num tx_rtx and tx-ok
    #     crtc_start_sing = line.find(app_addr_src_start_sing) + len(app_addr_src_start_sing)
    #     crtc_end_sing = line.find(app_addr_src_end_sing)
    #     if key_line_tx in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
    #         seq_s = line.find(app_seq_start_sing) + len(app_seq_start_sing)
    #         seq_e = line.find(app_seq_end_sing)
    #         tx_status_s = line.find(app_tx_status_start_sing) + len(app_tx_status_start_sing)
    #         if int(line[seq_s:seq_e]) < packet_num + 1:
    #             num_tx_array[int(line[seq_s:seq_e]) - 1] += 1
    #             if int(line[tx_status_s:tx_status_s + 1]) == 0:
    #                 tx_ok_array[int(line[seq_s:seq_e]) - 1] += 1
    #
    #     # num rx-ok
    #     crtc_start_sing = line.find(app_rx_addr_src_start_sing) + len(app_rx_addr_src_start_sing)
    #     crtc_end_sing = line.find(app_rx_addr_src_end_sing)
    #     if key_line_rx in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
    #         seq_s = line.find(app_rx_seq_start_sing) + len(app_rx_seq_start_sing)
    #         seq_e = line.find(app_rx_seq_end_sing)
    #         if int(line[seq_s:seq_e]) < packet_num + 1:
    #             rx_num_receive_array[int(line[seq_s:seq_e]) - 1] += 1
    #
    #     # num hop for this node id
    #     crtc_start_sing = line.find(num_hop_start_sing) + len(num_hop_start_sing)
    #     crtc_end_sing = line.find(num_hop_end_sing)
    #     addr_start_sing = line.find(num_hop_addr_start_sing) + len(num_hop_addr_start_sing)
    #     addr_end_sing = line.find(num_hop_addr_end_sing)
    #     if num_hop_start_sing in line and line[addr_start_sing:addr_end_sing] == ('0' + str(end_node_id)):
    #         num_hop = int(line[crtc_start_sing:crtc_end_sing])
    #
    # for i in range(packet_num):
    #     if num_tx_array[i] != 0:
    #         fail_tx = num_tx_array[i] - tx_ok_array[i]
    #         ack_intf_array[i] = rx_num_receive_array[i] - tx_ok_array[i]
    #         packet_intf_array[i] = fail_tx - ack_intf_array[i]
    #         queue_drop_array[i] = None
    #     else:
    #         queue_drop_array[i] = 1
    #         ack_intf_array[i] = None
    #         packet_intf_array[i] = None
    #
    # new_num_tx_array = []
    # if num_hop != 0:
    #     for number in num_tx_array:
    #         new_num_tx_array.append(number / num_hop)

    return list_nver_used_cell, list_max_cell_usage


def split_converge_time_steady_state(delay_list, node_id_list):
    avg_conv_point = 0
    list_conv_point = []
    for id in node_id_list:
        convergence_point = None
        new_conv_point = None
        std_point = 700
        w_len = 100
        w_sample = 20
        tmp_list = delay_list[id - 1]
        w = tmp_list[std_point:std_point + w_len]
        if all(v is None for v in w):
            new_conv_point = None
        else:
            new_conv_point = 0
            std_median = mean(d for d in w if d is not None)
            w_new = list(filter(None, w))
            # print('window size: ' + str(len(w_new)))
            # print(std_median)
            if len(w_new) > 90:
                ci = st.norm.interval(alpha=.9999, loc=np.mean(w_new), scale=st.sem(w_new))
                # print('confidence interval: ' + str(ci))
                convergence_point = 0
                # for item in range(std_point):
                #     if e2e_delay_list[std_point - item] > (std_median * 2):
                #         convergence_point = std_point - item
                #         print("my conve point: " + str(convergence_point))
                #         # print(std_point - i + w_len)
                #         # print(e2e_delay_list[0:std_point - i])
                #         # print(e2e_delay_list[std_point - i:std_point])
                #         break
                for item in range(std_point - w_sample):
                    tw = tmp_list[std_point - item - w_sample: std_point - item]
                    tmp_win = list(filter(None, tw))
                    if len(tmp_win) > 15:
                        if all(i > ci[1] for i in tmp_win):
                            new_conv_point = std_point - item
                            # print('conv: ' + str(new_conv_point) + ' win: ' + str(tmp_win))
                            break
                    else:
                        new_conv_point = simu_len
                        break

        if new_conv_point == 0 or new_conv_point is None:
            avg_conv_point = avg_conv_point + simu_len
            list_conv_point.append(simu_len)
            # print(tmp_list)
        else:
            avg_conv_point = avg_conv_point + new_conv_point
            list_conv_point.append(new_conv_point)

    return list_conv_point


def get_data_flow_configuration_time(log_file_name, end_node_id, node_id_list):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    conv_delay_list = []

    list_req = [0 for i in range(end_node_id + 1)]
    list_res = [0 for i in range(end_node_id + 1)]

    for line in input_lines:
        if flow_req_start_sing in line:
            sender_addr_sing_s = line.find(flow_req_start_sing) + len(flow_req_start_sing)
            sender_addr_sing_e = line.find(flow_req_end_sing)
            asn_sing_s = line.find(flow_req_asn_sing_start) + len(flow_req_asn_sing_start)
            asn_sing_e = line.find(flow_req_asn_sing_end)
            list_req[int(line[sender_addr_sing_s:sender_addr_sing_e])] = int(line[asn_sing_s:asn_sing_e], 16)

        if flow_resp_start_sing in line:
            receiver_addr_sing_s = line.find(flow_resp_start_sing) + len(flow_resp_start_sing)
            receiver_addr_sing_e = line.find(flow_resp_end_sing)
            asn_sing_s = line.find(flow_resp_asn_sing_start) + len(flow_resp_asn_sing_start)
            asn_sing_e = line.find(flow_resp_asn_sing_end)
            list_res[int(line[receiver_addr_sing_s:receiver_addr_sing_e])] = int(line[asn_sing_s:asn_sing_e], 16)

    for i in range(1, end_node_id + 1):
        if list_res[i] - list_req[i] > 0:
            conv_delay_list.append((list_res[i] - list_req[i]) * 10)

    return conv_delay_list


def get_sdn_convergence_time(log_file_name):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    key1 = 'Start with TSCH and SDN at ASN: '
    key2 = ' ]'
    last_asn = 0
    # TODO: for the MSF I should consider the tim in which nodes sends its first packet
    for line in input_lines:
        if key1 in line:
            s = line.find(key1) + len(key1)
            e = line.find(key2)
            if int(line[s:e], 16) > last_asn:
                last_asn = int(line[s:e], 16)

    return last_asn/100


def get_collision_rate(log_file_name):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    est_link_sign1 = 'est_pdr_l['
    est_link_sign2 = '] idl_cell['
    ideal_link_sign1 = '] idl_pdr_l['
    ideal_link_sign2 = ']]'

    list_of_collision = []
    for line in input_lines:
        if est_link_sign1 in line:
            s = line.find(est_link_sign1) + len(est_link_sign1)
            e = line.find(est_link_sign2)
            est_link = float(line[s:e].replace(",", "."))

            s = line.find(ideal_link_sign1) + len(ideal_link_sign1)
            e = line.find(ideal_link_sign2)
            list_of_collision.append((est_link / link_quality_coeff) / float(line[s:e].replace(",", ".")))

    return list_of_collision


def get_energy_consumption(log_file_name, net_size, solution_name):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    # curr: current in mA
    voltage = 3
    cpu_curr = 1.8      # mA
    lpm_curr = 0.0545   # mA
    tx_curr = 17.7      # mA
    rx_curr = 20        # mA

    key_before_join = 'ENGY before join->ID['
    key_after_join = 'ENGY after join->ID['
    key_cup = '] CPU:'
    key_lpm = 's LPM:'
    key_rx = 's Radio LISTEN:'
    key_tx = 's TRANSMIT:'
    key_end = 's OFF:'
    key_rx_cell1 = 'install rx cells: node['
    key_rx_cell2 = '], flow id['
    key_rx_cell3 = '], num cell['
    key_rx_cell4 = '], asn['
    key_rx_cell5 = ']]'

    before_join_list = [[0 for j in range(3)] for i in range(net_size + 1)]
    after_join_list = [[0 for j in range(3)] for i in range(net_size + 1)]
    energy_list_after_join = []
    energy_list_before_join = []
    idle_energy_list = [0 for i in range(net_size + 1)]

    for line in input_lines:
        if key_before_join in line:
            s = line.find(key_before_join) + len(key_before_join)
            e = line.find(key_cup)
            node_id = int(line[s:e])

            s = line.find(key_cup) + len(key_cup)
            e = line.find(key_lpm)
            before_join_list[node_id][0] = int(line[s:e])

            s = line.find(key_rx) + len(key_rx)
            e = line.find(key_tx)
            before_join_list[node_id][1] = int(line[s:e])

            s = line.find(key_tx) + len(key_tx)
            e = line.find(key_end)
            before_join_list[node_id][2] = int(line[s:e])

        if key_after_join in line:
            s = line.find(key_after_join) + len(key_after_join)
            e = line.find(key_cup)
            node_id = int(line[s:e])

            s = line.find(key_cup) + len(key_cup)
            e = line.find(key_lpm)
            after_join_list[node_id][0] = int(line[s:e])

            s = line.find(key_rx) + len(key_rx)
            e = line.find(key_tx)
            after_join_list[node_id][1] = int(line[s:e])

            s = line.find(key_tx) + len(key_tx)
            e = line.find(key_end)
            after_join_list[node_id][2] = int(line[s:e])

        if key_rx_cell1 in line:
            s = line.find(key_rx_cell1) + len(key_rx_cell1)
            e = line.find(key_rx_cell2)
            rx_node_id = int(line[s:e])

            s = line.find(key_rx_cell2) + len(key_rx_cell2)
            e = line.find(key_rx_cell3)
            if int(line[s:e]) > 8:
                ss = line.find(key_rx_cell3) + len(key_rx_cell3)
                ee = line.find(key_rx_cell4)
                rx_cell = int(line[ss:ee])
                ss = line.find(key_rx_cell4) + len(key_rx_cell4)
                ee = line.find(key_rx_cell5)
                asn = int(line[ss:ee], 16)
                idle_energy_list[rx_node_id] += (((print_asn - asn) / sf_size) * rx_cell) * 0.0026


    print('idle energy list: ', idle_energy_list)
    for i in range(1, net_size + 1):
        cpu_time = after_join_list[i][0] - before_join_list[i][0]
        rx_time = after_join_list[i][1] - before_join_list[i][1] #- idle_energy_list[i]
        tx_time = after_join_list[i][2] - before_join_list[i][2]
        print('Time after join: node:', i, 'cpu_time:', cpu_time, 'rx_time:', rx_time, 'tx_time:', tx_time)
        energy_list_after_join.append((cpu_time * cpu_curr + rx_time * rx_curr + tx_time * tx_curr) * voltage)
        print('Energy after join: ', 'cpu:', cpu_time * cpu_curr, 'rx:', rx_time * rx_curr, 'tx:', tx_time * tx_curr)

    for i in range(1, net_size + 1):
        cpu_time = before_join_list[i][0]
        rx_time = before_join_list[i][1]
        tx_time = before_join_list[i][2]
        print('Time before join: ', 'cpu_time:', cpu_time, 'rx_time:', rx_time, 'tx_time:', tx_time)
        energy_list_before_join.append((cpu_time * cpu_curr + rx_time * rx_curr + tx_time * tx_curr) * voltage)
        print('Energy before join: ', 'cpu:', cpu_time * cpu_curr, 'rx:', rx_time * rx_curr, 'tx:', tx_time * tx_curr)

    print('energy_list: ', energy_list_after_join, energy_list_before_join)

    return energy_list_after_join, energy_list_before_join


def get_control_cells(log_file_name, net_size, solution_name):
    log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    parent_key = 'CONTROLLER: specify parent addr '
    shared_cell_key2 = ', sdn_num_shared_cell_in_rep:'
    shared_cell_key1 = 'sdn_num_shared_cell: '
    tot_cell = 0

    for line in input_lines:
        if shared_cell_key2 in line:
            s = line.find(shared_cell_key1) + len(shared_cell_key1)
            e = line.find(shared_cell_key2)
            tot_cell = tot_cell + (int(line[s:e]) * net_size)
            print("tot cell 1: ", tot_cell)
            break

    if solution_name == stupid_filename_key:
        tot_cell = ((net_size - 1) * 2 * sf_rep) + tot_cell

        fc_list = [0 for i in range(net_size + 1)]
        for line in input_lines:
            if parent_key in line:
                s = line.find(parent_key) + len(parent_key)
                e = s + 4
                fc_list[int(line[s:e], 16)] = fc_list[int(line[s:e], 16)] + 1
        for item in fc_list:
            if item != 0:
                tot_cell = item + 1 + tot_cell

    if solution_name == sdn_filename_key:
        tot_cell = ((net_size - 1) * 2 * sf_rep) + tot_cell

        fc_list = [0 for i in range(net_size + 1)]
        for line in input_lines:
            if parent_key in line:
                s = line.find(parent_key) + len(parent_key)
                e = s + 4
                fc_list[int(line[s:e], 16)] = fc_list[int(line[s:e], 16)] + 1
        for item in fc_list:
            if item != 0:
                tot_cell = item + 1 + tot_cell

    if solution_name == shared_fctrl_filename_key:
        tot_cell = ((net_size - 1) * 2 * sf_rep) + tot_cell

    return tot_cell


def main_func_plot():
    for i in net_size_array:
        msf_net_log_list = get_list_of_log_file(msf_filename_key + str(i), log_list)
        for m in lqr_array:
            msf_lqr_log_list = get_list_of_log_file(str(m), msf_net_log_list)
            for j in msf_lqr_log_list:
                std_delay, pdr, node_id_list = e2e_delay(j, i)

                for nd_pdr in pdr:
                    if not (nd_pdr is None):
                        temp_pdr = [nd_pdr, 'MSF', i]
                        df_pdr_len = len(pdr_df)
                        pdr_df.loc[df_pdr_len] = temp_pdr

                for delay in std_delay:
                    for pa_delay in delay:
                        if not (pa_delay is None):
                            temp_std_delay = [pa_delay / 1000, 'MSF', i]
                            df_cp_len = len(std_state_delay_df)
                            std_state_delay_df.loc[df_cp_len] = temp_std_delay

                # if all(s is None for s in delay_array[980:999]):
                #     print('MSF problematic log: ' + str(j))
                # if m == 0.7:
                #     print('delay array: ' + str(delay_array))
                #     print('Conv point: ' + str(conv_point))
                delay_lists, node_id_list2 = msf_e2e_delay(j, i)
                conv_point = split_converge_time_steady_state(delay_lists, node_id_list2)
                for item in conv_point:
                    temp_conv_point = [item * app_int, 'MSF', i]
                    df_cp_len = len(flow_config_time_df)
                    flow_config_time_df.loc[df_cp_len] = temp_conv_point

        # SDN dedicated control plane
        sdn_net_log_list = get_list_of_log_file(sdn_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                std_delay, pdr, node_id_list = e2e_delay(j, i)
                print(node_id_list)
                # print(std_delay)
                # if all(s is None for s in delay_array[980:999]):
                #     print('problematic log: ' + str(j))
                for nd_pdr in pdr:
                    if not (nd_pdr is None):
                        temp_pdr = [nd_pdr, 'SDN-TSCH', i]
                        df_pdr_len = len(pdr_df)
                        pdr_df.loc[df_pdr_len] = temp_pdr
                        if nd_pdr < 50:
                            print('low pdr:  ---> ' + str(j) + '  "PDR = ' + str(nd_pdr) + '"')

                for delay in std_delay:
                    for pa_delay in delay:
                        if not (pa_delay is None):
                            temp_std_delay = [pa_delay / 1000, 'SDN-TSCH', i]
                            df_cp_len = len(std_state_delay_df)
                            std_state_delay_df.loc[df_cp_len] = temp_std_delay

                conv_time = get_data_flow_configuration_time(j, i, node_id_list)
                for item in conv_time:
                    if item != 0:
                        print('converg time: ', item/1000)
                        temp_conv_point = [item/1000, 'SDN-TSCH', i]
                        df_cp_len = len(flow_config_time_df)
                        flow_config_time_df.loc[df_cp_len] = temp_conv_point

        # SDN shared control plane
        sdn_net_log_list = get_list_of_log_file(shared_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                std_delay, pdr, node_id_list = e2e_delay(j, i)
                print(node_id_list)
                # print(std_delay)
                # if all(s is None for s in delay_array[980:999]):
                #     print('problematic log: ' + str(j))
                for nd_pdr in pdr:
                    if not (nd_pdr is None) and nd_pdr > 95:
                        temp_pdr = [nd_pdr, 'SDN-TSCH-SHARED_CON.PLANE', i]
                        df_pdr_len = len(pdr_df)
                        pdr_df.loc[df_pdr_len] = temp_pdr
                        if nd_pdr < 50:
                            print('low pdr:  ---> ' + str(j) + '  "PDR = ' + str(nd_pdr) + '"')

                for delay in std_delay:
                    # print('flow delay list: ', delay)
                    for pa_delay in delay:
                        if not (pa_delay is None):
                            temp_std_delay = [pa_delay / 1000, 'SDN-TSCH-SHARED_CON.PLANE', i]
                            df_cp_len = len(std_state_delay_df)
                            std_state_delay_df.loc[df_cp_len] = temp_std_delay

                conv_time = get_data_flow_configuration_time(j, i, node_id_list)
                for item in conv_time:
                    if item != 0:
                        print('converg time shared cp : ', item/1000)
                        temp_conv_point = [item / 1000, 'SDN-TSCH-SHARED_CON.PLANE', i]
                        df_cp_len = len(flow_config_time_df)
                        flow_config_time_df.loc[df_cp_len] = temp_conv_point

        # SDN shared from controller
        sdn_net_log_list = get_list_of_log_file(shared_fctrl_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                std_delay, pdr, node_id_list = e2e_delay(j, i)
                print(node_id_list)
                # print(std_delay)
                # if all(s is None for s in delay_array[980:999]):
                #     print('problematic log: ' + str(j))
                for nd_pdr in pdr:
                    if not (nd_pdr is None) and nd_pdr > 95:
                        temp_pdr = [nd_pdr, 'SDN-TSCH-SHARED_FROM.CON',
                                    i]
                        df_pdr_len = len(pdr_df)
                        pdr_df.loc[df_pdr_len] = temp_pdr
                        if nd_pdr < 50:
                            print('low pdr:  ---> ' + str(j) + '  "PDR = ' + str(nd_pdr) + '"')

                for delay in std_delay:
                    # print('flow delay list: ', delay)
                    for pa_delay in delay:
                        if not (pa_delay is None):
                            temp_std_delay = [pa_delay / 1000,
                                              'SDN-TSCH-SHARED_FROM.CON',
                                              i]
                            df_cp_len = len(std_state_delay_df)
                            std_state_delay_df.loc[df_cp_len] = temp_std_delay

                conv_time = get_data_flow_configuration_time(j, i, node_id_list)
                for item in conv_time:
                    if item != 0:
                        print('converg time shared FC : ', item / 1000)
                        temp_conv_point = [item / 1000,
                                           'SDN-TSCH-SHARED_FROM.CON', i]
                        df_cp_len = len(flow_config_time_df)
                        flow_config_time_df.loc[df_cp_len] = temp_conv_point

        # SDN stupid solution: uncontrolled EB trnas.
        sdn_net_log_list = get_list_of_log_file(stupid_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                std_delay, pdr, node_id_list = e2e_delay(j, i)
                print(node_id_list)
                # print(std_delay)
                # if all(s is None for s in delay_array[980:999]):
                #     print('problematic log: ' + str(j))
                for nd_pdr in pdr:
                    if not (nd_pdr is None) and nd_pdr > 95:
                        temp_pdr = [nd_pdr, 'SDN-TSCH-SHARED-EB',
                                    i]
                        df_pdr_len = len(pdr_df)
                        pdr_df.loc[df_pdr_len] = temp_pdr
                        if nd_pdr < 50:
                            print('low pdr:  ---> ' + str(j) + '  "PDR = ' + str(nd_pdr) + '"')

                for delay in std_delay:
                    # print('flow delay list: ', delay)
                    for pa_delay in delay:
                        if not (pa_delay is None):
                            temp_std_delay = [pa_delay / 1000,
                                              'SDN-TSCH-SHARED-EB',
                                              i]
                            df_cp_len = len(std_state_delay_df)
                            std_state_delay_df.loc[df_cp_len] = temp_std_delay

                conv_time = get_data_flow_configuration_time(j, i, node_id_list)
                for item in conv_time:
                    if item != 0:
                        print('converg time shared EB : ', item / 1000)
                        temp_conv_point = [item / 1000,
                                           'SDN-TSCH-SHARED-EB', i]
                        df_cp_len = len(flow_config_time_df)
                        flow_config_time_df.loc[df_cp_len] = temp_conv_point

                    # if std_delay/1000 > 2:
                    #     print('SDN large delay:  ' + str(j))
                    # print(delay_array)
                    # print(std_delay/1000)
                    # print(pdr)

    for i in net_size_array:
        msf_net_log_list = get_list_of_log_file(msf_filename_key + str(i), log_list)
        for m in lqr_array:
            msf_lqr_log_list = get_list_of_log_file(str(m), msf_net_log_list)
            for j in msf_lqr_log_list:
                num_sch_ts, num_ideal_cell = msf_get_num_scheduled_ts(j, i, m)

                temp_pdr = [num_sch_ts, 'MSF', i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr

                cell_usage = get_max_cell_usage_per_sf(j, i)
                if i == 3 and cell_usage < 8:
                    print('low cell usage: ' + str(j))
                temp_pdr = [cell_usage, 'MSF', i]
                df_pdr_len = len(sf_max_cell_usage_df)
                sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

        # SDN dedicated control plane
        sdn_net_log_list = get_list_of_log_file(sdn_filename_key + str(i), log_list)
        packet_intf = []
        ack_intf = []
        queue_drop = []
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                never_used_cell, sf_cell_usage = number_of_tx_and_collision(j, i)

                for scu in sf_cell_usage:
                    temp_pdr = [scu, 'SDN-TSCH', i]
                    df_pdr_len = len(sf_max_cell_usage_df)
                    sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

                for ncu in never_used_cell:
                    temp_pdr = [ncu, 'SDN-TSCH', i]
                    df_pdr_len = len(never_used_cell_df)
                    never_used_cell_df.loc[df_pdr_len] = temp_pdr

                # if not all(s is None for s in packet_intf_array):
                #     packet_intf.append(mean(d for d in packet_intf_array if d is not None))
                # if not all(s is None for s in ack_intf_array):
                #     ack_intf.append(mean(d for d in ack_intf_array if d is not None))
                # if not all(s is None for s in queue_drop_array):
                #     queue_drop.append(mean(d for d in queue_drop_array if d is not None))
                #
                # for k in range(len(num_tx_array)):
                #     if k is not None:
                #         temp_pdr = [num_tx_array[k], 'SDN-TSCH(LQR '+str(m*100)+'%)', i]
                #         df_pdr_len = len(num_tx_df)
                #         num_tx_df.loc[df_pdr_len] = temp_pdr

                # get the number of avg scheduled ts for the network

                num_sch_ts, num_ideal_cell = sdn_get_num_scheduled_ts(j, i)
                temp_pdr = [num_sch_ts, 'SDN-TSCH', i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr
                # if num_sch_ts > 20:
                #     print('Low Schedule problem: ' + str(j))
                coll_list = get_collision_rate(j)
                for lq in coll_list:
                    temp_lq = [lq, 'SDN-TSCH', i]
                    df_lq_len = len(collision_rate_df)
                    collision_rate_df.loc[df_lq_len] = temp_lq

                energy_list_after, energy_list_before = get_energy_consumption(j, i, sdn_filename_key)
                for item in energy_list_after:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH', i]
                        df_energy_len = len(energy_consumption_joined_df)
                        energy_consumption_joined_df.loc[df_energy_len] = temp_slot

                for item in energy_list_before:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH', i]
                        df_energy_len = len(energy_consumption_bootstrap_df)
                        energy_consumption_bootstrap_df.loc[df_energy_len] = temp_slot

                sdn_conv_time = get_sdn_convergence_time(j)
                if sdn_conv_time != 0:
                    conv = [sdn_conv_time, 'SDN-TSCH', i]
                    df_conv_len = len(convergence_df)
                    convergence_df.loc[df_conv_len] = conv

        # SDN shared control plane
        sdn_net_log_list = get_list_of_log_file(shared_filename_key + str(i), log_list)
        packet_intf = []
        ack_intf = []
        queue_drop = []
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                never_used_cell, sf_cell_usage = number_of_tx_and_collision(j, i)

                for scu in sf_cell_usage:
                    temp_pdr = [scu, 'SDN-TSCH-SHARED_CON.PLANE', i]
                    df_pdr_len = len(sf_max_cell_usage_df)
                    sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

                for ncu in never_used_cell:
                    temp_pdr = [ncu, 'SDN-TSCH-SHARED_CON.PLANE', i]
                    df_pdr_len = len(never_used_cell_df)
                    never_used_cell_df.loc[df_pdr_len] = temp_pdr

                num_sch_ts, num_ideal_cell = sdn_get_num_scheduled_ts(j, i)
                temp_pdr = [num_sch_ts, 'SDN-TSCH-SHARED_CON.PLANE', i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr

                energy_list_after, energy_list_before = get_energy_consumption(j, i, shared_filename_key)
                for item in energy_list_after:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED_CON.PLANE', i]
                        df_energy_len = len(energy_consumption_joined_df)
                        energy_consumption_joined_df.loc[df_energy_len] = temp_slot

                for item in energy_list_before:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED_CON.PLANE', i]
                        df_energy_len = len(energy_consumption_bootstrap_df)
                        energy_consumption_bootstrap_df.loc[df_energy_len] = temp_slot

                sdn_conv_time = get_sdn_convergence_time(j)
                if sdn_conv_time != 0:
                    conv = [sdn_conv_time, 'SDN-TSCH-SHARED_CON.PLANE', i]
                    df_conv_len = len(convergence_df)
                    convergence_df.loc[df_conv_len] = conv

        # SDN shared from controller
        sdn_net_log_list = get_list_of_log_file(shared_fctrl_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                never_used_cell, sf_cell_usage = number_of_tx_and_collision(j, i)

                for scu in sf_cell_usage:
                    temp_pdr = [scu, 'SDN-TSCH-SHARED_FROM.CON', i]
                    df_pdr_len = len(sf_max_cell_usage_df)
                    sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

                for ncu in never_used_cell:
                    temp_pdr = [ncu, 'SDN-TSCH-SHARED_FROM.CON', i]
                    df_pdr_len = len(never_used_cell_df)
                    never_used_cell_df.loc[df_pdr_len] = temp_pdr

                num_sch_ts, num_ideal_cell = sdn_get_num_scheduled_ts(j, i)
                temp_pdr = [num_sch_ts, 'SDN-TSCH-SHARED_FROM.CON', i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr

                energy_list_after, energy_list_before = get_energy_consumption(j, i, shared_fctrl_filename_key)
                for item in energy_list_after:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED_FROM.CON', i]
                        df_energy_len = len(energy_consumption_joined_df)
                        energy_consumption_joined_df.loc[df_energy_len] = temp_slot

                for item in energy_list_before:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED_FROM.CON', i]
                        df_energy_len = len(energy_consumption_bootstrap_df)
                        energy_consumption_bootstrap_df.loc[df_energy_len] = temp_slot

                sdn_conv_time = get_sdn_convergence_time(j)
                if sdn_conv_time != 0:
                    conv = [sdn_conv_time, 'SDN-TSCH-SHARED_FROM.CON', i]
                    df_conv_len = len(convergence_df)
                    convergence_df.loc[df_conv_len] = conv

        # SDN stupid solution: uncontrolled EB trnas.
        sdn_net_log_list = get_list_of_log_file(stupid_filename_key + str(i), log_list)
        for m in lqr_array:
            sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
            for j in sdn_lqr_log_list:
                never_used_cell, sf_cell_usage = number_of_tx_and_collision(j, i)

                for scu in sf_cell_usage:
                    temp_pdr = [scu, 'SDN-TSCH-SHARED-EB', i]
                    df_pdr_len = len(sf_max_cell_usage_df)
                    sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

                for ncu in never_used_cell:
                    temp_pdr = [ncu, 'SDN-TSCH-SHARED-EB', i]
                    df_pdr_len = len(never_used_cell_df)
                    never_used_cell_df.loc[df_pdr_len] = temp_pdr

                num_sch_ts, num_ideal_cell = sdn_get_num_scheduled_ts(j, i)
                temp_pdr = [num_sch_ts, 'SDN-TSCH-SHARED-EB', i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr
                # if num_sch_ts > 20:
                #     print('Low Schedule problem: ' + str(j))
                coll_list = get_collision_rate(j)
                for lq in coll_list:
                    temp_lq = [lq, 'SDN-TSCH-SHARED-EB', i]
                    df_lq_len = len(collision_rate_df)
                    collision_rate_df.loc[df_lq_len] = temp_lq

                energy_list_after, energy_list_before = get_energy_consumption(j, i, stupid_filename_key)
                for item in energy_list_after:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED-EB', i]
                        df_energy_len = len(energy_consumption_joined_df)
                        energy_consumption_joined_df.loc[df_energy_len] = temp_slot

                for item in energy_list_before:
                    if item != 0:
                        temp_slot = [item, 'SDN-TSCH-SHARED-EB', i]
                        df_energy_len = len(energy_consumption_bootstrap_df)
                        energy_consumption_bootstrap_df.loc[df_energy_len] = temp_slot

                sdn_conv_time = get_sdn_convergence_time(j)
                if sdn_conv_time != 0:
                    conv = [sdn_conv_time, 'SDN-TSCH-SHARED-EB', i]
                    df_conv_len = len(convergence_df)
                    convergence_df.loc[df_conv_len] = conv


main_func_plot()

        # if num_sch_ts > 20:
        #     print('Low Schedule problem: ' + str(j))

        # mean_packet_intf = 0
        # mean_ack_intf = 0
        # if not all(s is None for s in packet_intf):
        #     mean_packet_intf = mean(d for d in packet_intf if d is not None)
        # if not all(s is None for s in ack_intf):
        #     mean_ack_intf = mean(d for d in ack_intf if d is not None)
        #
        # temp_pdr = [mean_packet_intf, mean_ack_intf, i]
        # df_pdr_len = len(cause_rtx_df)
        # cause_rtx_df.loc[df_pdr_len] = temp_pdr
# print(pdr_df.to_string())

# ################## calculate collision over shared cell #######################
# def collision_rate_of_shared_cell(log_file_name):
#
#     log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-rand-top-satatis/log/" + str(log_file_name)
#     file = open(log_path, "r")
#     input_lines = file.readlines()
#     input_lines2 = file.readlines()
#     shared_cell_list = [0 for i in range(251)]
#
#     key_send_eb = ' sent EB at asn: '
#     key_receive_eb = '] EB received from:['
#     key_avg_eb = 'CONTROLLER: EB-report: node'
#
#     eb_tx_count = 0
#     eb_rx_count = 0
#
#     list_of_asn = []
#     list_of_asn_hex = []
#
#     eb_avg_count = 0
#     eb_avg_value = 0
#     for line in input_lines:
#         if key_send_eb in line:
#             eb_tx_count += 1
#             sender_asn_sing_s = line.find(key_send_eb) + len(key_send_eb)
#             sender_asn_sing_e = line.find(']]')
#             hex_asn = line[sender_asn_sing_s:sender_asn_sing_e]
#             list_of_asn_hex.append(hex_asn.replace('0x', '00'))
#             asn = int(line[sender_asn_sing_s:sender_asn_sing_e], 16)
#             if asn in list_of_asn and list_of_asn.count(asn) == 1:
#                 shared_cell_list[asn % 251] += 1
#
#             list_of_asn.append(asn)
#
#         if key_receive_eb in line:
#             eb_rx_count += 1
#
#         if key_avg_eb in line:
#             s = line.find('eb_avg[') + len('eb_avg[')
#             e = line.find(']]')
#             eb_avg_count += 1
#             eb_avg_value = eb_avg_value + float(line[s:e].replace(",", "."))
#
#     res = {}
#     for i in list_of_asn:
#         res[i] = list_of_asn.count(i)
#     # print('res: ', res)
#
#     coll_dist_list = [0 for i in range(net_size - 1)]
#     for i in res:
#         if res[i] > 1:
#             coll_dist_list[res[i] - 2] += 1
#
#     print('collion dist: ', coll_dist_list)
#     print('avg eb: ', eb_avg_value / eb_avg_count)
#
#     print('send eb: ', eb_tx_count, 'receive eb: ', eb_rx_count/(net_size - 1), 'dif: ', (eb_tx_count - (eb_rx_count/(net_size - 1))))
#     print('sum of collisions: ', sum(shared_cell_list))
#     print('list of asn: ', list_of_asn_hex)
#
#     for asn in list_of_asn_hex:
#         for line in input_lines:
#             if asn in line:
#                 print(line)
#
#     return shared_cell_list, coll_dist_list
#
#
# net_size = 17
# coll_log_list = get_list_of_log_file(sdn_filename_key + str(net_size), log_list)
# for log in coll_log_list:
#     cell_list, coll_dist = collision_rate_of_shared_cell(log)
#     plt.subplot(131)
#     plt.plot(cell_list)
#
#     # plt.savefig('col_cell.png', bbox_inches='tight')
#     plt.title("dist. of collision on shared cell")
#
#     plt.subplot(133)
#     plt.plot(coll_dist)
#     plt.title("number of transmitter in each collision")
#     plt.savefig('col_dist.png', bbox_inches='tight')

# # ################## plot stacked re-tx #######################
#
# # cause_rtx_df.plot(x='Network size', kind='bar', stacked=True, fontsize='15')
# # plt.xlabel("Network size", fontsize='15')
# # plt.ylabel("Cause of retransmission (%)", fontsize='15')
# # plt.savefig('stack_rtx', bbox_inches='tight')
#

# # # ################## plot flow configuration time #######################
# hue_order = ['MSF(LQR 84.0%)', 'SDN-TSCH(LQR 84.0%)', 'SDN-TSCH-SHARED_CON.PLANE(LQR 84.0%)', 'MSF(LQR 95.0%)', 'SDN-TSCH(LQR 95.0%)', 'SDN-TSCH-SHARED_CON.PLANE(LQR 95.0%)']
hue_order = ['MSF', 'SDN-TSCH', 'SDN-TSCH-SHARED-EB', 'SDN-TSCH-SHARED_FROM.CON', 'SDN-TSCH-SHARED_CON.PLANE']
hue_order_coll_rate = ['SDN-TSCH', 'SDN-TSCH-SHARED-EB']


sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=flow_config_time_df, x='Network size', y='Flow configuration time(s)',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
#ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
plt.yscale('log')
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('flow-config-time.pdf', bbox_inches='tight')

# ################## plot std state delay #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=std_state_delay_df, x='Network size', y='End-to-End delay(s)',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
#ax_sdn_pdr.set(ylim=(0, 10))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.yscale('log')
plt.axhline(y=2.52, color='r', linestyle='--')
plt.text(1, 2.54, 'Deadline = 2.51 s', horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('delay_std_state.pdf', bbox_inches='tight')

# ################## plot PDR #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=pdr_df, x='Network size', y='PDR(%) before deadline',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
ax_sdn_pdr.set(ylim=(0, 100))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
# print(ax_sdn_pdr.axes,ax_sdn_pdr.axes.flatten())
# for i, ax in enumerate(ax_sdn_pdr.axes.flatten()):
#     print('printing ax', len(ax.collections))
#     # print(i)
#     # print(len(ax))
#     ax.collections[0].set_edgecolor('tab:red')
#     # ax.collections[1].set_edgecolor('tab:pink')
#     # ax.collections[8].set_edgecolor('tab:blue')

plt.savefig('pdr.pdf', bbox_inches='tight')

# # ################## plot number of TX+rTX #######################
# # sns.set(font_scale=3.2)
# # sns.set_style({'font.family': 'serif'})
# # ax_sdn_pdr = sns.catplot(kind='violin', data=num_tx_df, x='Network size', y='Number of TX+RTX',
# #                          hue='Network Solution', palette="tab10", cut=0, scale='width',
# #                          saturation=1, legend=False, height=10, aspect=1.8)
# # ax_sdn_pdr.set(ylim=(0, 102))
# # # plt.subplots_adjust(top=.8)
# # plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# # # plt.yticks(range(0,110,20))
# # # ax_sdn_pdr.fig.set_figwidth(20)
# # # ax_sdn_pdr.fig.set_figheight(10)
# # plt.savefig('num_tx.pdf', bbox_inches='tight')
# #
# # ################## plot scheduled timeslots #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=ts_num_df, x='Network size', y='Normalized scheduled cells',
                         hue='Network Solution', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.axhline(y=1, color='r', linestyle='--')
# plt.text(1, 2.54, horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('scheduled-ts.pdf', bbox_inches='tight')
# # ################## max cell usage per SF per node #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=sf_max_cell_usage_df, x='Network size', y='Max cell usage per SF and node',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('max_sf_cell.pdf', bbox_inches='tight')

# # ################## never used cells in simulation #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=never_used_cell_df, x='Network size', y='Number of never used cells',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('never_cell.pdf', bbox_inches='tight')
# # ################## plot collision rate #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=collision_rate_df, x='Network size', y='Normalized link quality estimation',
                         hue='Network Solution', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.axhline(y=1, color='r', linestyle='--')
# plt.text(1, 2.54, horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('coll_rate.pdf', bbox_inches='tight')
# # ################## plot energy consumption joined #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=energy_consumption_joined_df, x='Network size', y='Energy consumption (mJ)',
                         hue='Network Solution', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
#plt.axhline(y=1, color='r', linestyle='--')
# plt.text(1, 2.54, horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('energy-joined.pdf', bbox_inches='tight')
# # ################## plot energy consumption bootstraping #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=energy_consumption_bootstrap_df, x='Network size', y='Energy consumption (mJ)',
                         hue='Network Solution', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
#plt.axhline(y=1, color='r', linestyle='--')
# plt.text(1, 2.54, horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('energy-boot.pdf', bbox_inches='tight')
# # ################## plot conv_time #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=convergence_df, x='Network size', y='Convergence time(s)',
                         hue='Network Solution', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=2, fontsize='24', bbox_to_anchor=[0.5, 1.25])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
#plt.axhline(y=1, color='r', linestyle='--')
# plt.text(1, 2.54, horizontalalignment='center', verticalalignment='bottom', fontsize='24', color='r')
plt.savefig('conv-time.pdf', bbox_inches='tight')
# # ################################################################

# sns.set(font_scale=3.2)
# sns.set_style({'font.family': 'serif'})
# ax_sdn_pdr = num_tx_df.plot(kind='bar', stacked=True, x='Network size')
# ax_sdn_pdr.set(ylim=(0, 102))
# # plt.subplots_adjust(top=.8)
# plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# # plt.yticks(range(0,110,20))
# # ax_sdn_pdr.fig.set_figwidth(20)
# # ax_sdn_pdr.fig.set_figheight(10)
# plt.savefig('num_tx1.pdf', bbox_inches='tight')
# e2e_lat_array = []
# for net in range(1):
#     print(log_list[net + 32])
#     lat_list, pdr = e2e_delay(log_list[net + 32], 5)
#     print('hi')
#     print([i for i in range(len(lat_list)) if lat_list[i] == None])
#     e2e_lat_array.append(lat_list)
#     plt.plot(e2e_lat_array[net])
#
#
# plt.xlabel("Packet seq-num")
# plt.ylabel("End-to-end delay (ms)")
# plt.savefig('msf_pdr11.png', bbox_inches='tight')


















# ################# 2APP code ###############
# def e2e_delay(log_file_name, end_node_id):
#     log_path = user_home_path + "/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
#     file = open(log_path, "r")
#     input_lines = file.readlines()
#     file.close()
#     key_line = "11111111 "
#     tot_rx_count1 = 0
#     tot_rx_count2 = 0
#     packet_num = 1000
#     initial_drift = 300
#     lat_critic_array1 = [None] * packet_num
#     lat_critic_array2 = [None] * packet_num
#
#     for line in input_lines:
#         crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
#         crtc_end_sing = line.find(addr_src_end_sing)
#         if key_line in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
#             sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
#             sent_asn_e = line.find(sent_asn_end_sign)
#             rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
#             rcv_asn_e = line.find(recv_asn_end_sing)
#             seq_s = line.find(seq_start_sing) + len(seq_start_sing)
#             seq_e = line.find(seq_end_sing)
#             sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
#             sent_port_e = line.find(sent_port_end_sing)
#             if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
#                 tot_rx_count1 += 1
#                 lat_critic_array1[int(line[seq_s:seq_e]) - 1] = ((int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10) / 1000
#
#             if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1030":
#                 tot_rx_count2 += 1
#                 lat_critic_array2[int(line[seq_s:seq_e]) - 1] = ((int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10) / 1000
#     return lat_critic_array1, lat_critic_array2
#
#
# fig, axs = plt.subplots(2, sharex=False)
#
# e2e_lat_array = []
# app1, app2 = e2e_delay(log_list[0], 3)
# e2e_lat_array.append(app1)
# axs[0].plot(e2e_lat_array[0], label='MSF: APP1 (2 s)')
# e2e_lat_array.append(app2)
# axs[0].plot(e2e_lat_array[1], label='MSF: APP2 (1 s)')
# axs[0].legend()
# # axs[0].set_title('MSF')
# axs[0].set(ylabel='End-to-end delay (s)', ylim=[-1, 40], xlim=[0, 1000])
# # sdn
# app1, app2 = e2e_delay(log_list[1], 3)
# e2e_lat_array.append(app1)
# axs[1].plot(e2e_lat_array[2], label='SDN-TSCH: APP1 (2 s)')
# e2e_lat_array.append(app2)
# axs[1].plot(e2e_lat_array[3], label='SDN-TSCH: APP2 (1 s)')
# axs[1].legend()
# # axs[1].set_title('SDN-TSCH')
# axs[1].set(ylabel='End-to-end delay (s)', ylim=[-1, 40], xlim=[0, 1000])
#
# plt.xlabel("Packet seq-num")
# plt.legend()
# plt.savefig('end2end-delay-2app-msf.pdf', bbox_inches='tight')


# plt.xlabel("Packet seq-num")
# plt.ylabel("End-to-end delay (ms)")
# plt.legend()
# plt.savefig('end2end-delay-2app-sdn.png', bbox_inches='tight')


# print(split_converge_time_steady_state(e2e_lat_array[5]))


