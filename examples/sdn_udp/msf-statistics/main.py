import os
import numpy as np
import scipy.stats as st
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from statistics import mean
# import glob
import numpy as np

std_state_delay_df = pd.DataFrame(columns=['End-to-End delay(s)', 'Network Solution', 'Network size'])
convergence_delay_df = pd.DataFrame(columns=['End-to-End delay(s)', 'Network Solution', 'Network size'])
convergence_df = pd.DataFrame(columns=['Convergence time (s)', 'Network Solution', 'Network size'])
ts_num_df = pd.DataFrame(columns=['Avg. scheduled timeslots per node', 'Network Solution', 'Network size'])
pdr_df = pd.DataFrame(columns=['PDR(%)', 'Network Solution', 'Network size'])
num_tx_df = pd.DataFrame(columns=['Number of TX+RTX', 'Network Solution', 'Network size'])
cause_rtx_df = pd.DataFrame(columns=['Packet_tx_fail', 'Ack_tx_fail', 'Network size'])
sf_max_cell_usage_df = pd.DataFrame(columns=['Max cell usage per SF and node', 'Network Solution', 'Network size'])

app_int = 1
net_size_array = [2, 3, 4]
lqr_array = [0.95, 0.84, 0.71]
simu_len = 1000

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

sf_ts_num_start_sing = '*** sf_size[['
sf_ts_num_end_sing = ']] Link Options'

num_hop_start_sing = 'CONTROLLER: HOP_TO_DEST: '
num_hop_end_sing = ', ADDR:[['
num_hop_addr_start_sing = ', ADDR:[['
num_hop_addr_end_sing = ']]'

max_cell_usage_start = '[MAX sf cell usage: '
max_cell_usage_end = ' node_addr '

log_dir = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log"
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
    log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    key_line = "11111111 "
    tot_rx_count1 = 0
    tot_rx_count2 = 0
    packet_num = 1000
    lat_critic_array1 = [None] * packet_num
    bounded_lat_array = [None] * packet_num
    lat_critic_array2 = [None] * packet_num

    for line in input_lines:
        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
        crtc_end_sing = line.find(addr_src_end_sing)
        if key_line in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
            sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
            sent_asn_e = line.find(sent_asn_end_sign)
            rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
            rcv_asn_e = line.find(recv_asn_end_sing)
            seq_s = line.find(seq_start_sing) + len(seq_start_sing)
            seq_e = line.find(seq_end_sing)
            sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
            sent_port_e = line.find(sent_port_end_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
                tot_rx_count1 += 1
                lat_critic_array1[int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10
                if ((int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10) < 2520:
                    bounded_lat_array[int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], 16) - int(
                        line[sent_asn_s:sent_asn_e], 16)) * 10

            if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1030":
                tot_rx_count2 += 1
                lat_critic_array2[int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10
    return bounded_lat_array, ((len(bounded_lat_array) - bounded_lat_array.count(None)) / packet_num) * 100


# def get_num_scheduled_ts(log_file_name, net_size):
#     log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
#     file = open(log_path, "r")
#     input_lines = file.readlines()
#     file.close()
#     total_ts = 0
#     for line in input_lines:
#         if sf_ts_num_start_sing in line:
#             total_ts += 1
#     return total_ts/net_size

def get_num_scheduled_ts(log_file_name, net_size):
    log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    total_ts = 0
    sing1 = 'scheduled cell perhop:nodeid[0'
    sing2 = '] hopup['
    sing3 = '], hopdown['
    sing4 = ']]'
    cells_per_hop = [0 for i in range(net_size - 1)]
    for line in input_lines:
        if sing1 in line:
            s = line.find(sing1) + len(sing1)
            e = line.find(sing2)
            node_id = int(line[s:e])
            s = line.find(sing2) + len(sing2)
            e = line.find(sing3)
            up_hop = int(line[s:e])
            s = line.find(sing3) + len(sing3)
            e = line.find(sing4)
            down_hop = int(line[s:e])
            if node_id != 1:
                cells_per_hop[node_id - 2] = cells_per_hop[node_id - 2] + up_hop
            if node_id != net_size:
                cells_per_hop[node_id - 1] = cells_per_hop[node_id - 1] + down_hop
    # print(cells_per_hop)

    return cells_per_hop


def get_max_cell_usage_per_sf(log_file_name, end_node_id):
    log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
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
    log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
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

    for line in input_lines:
        # calculate max cell sf usage:
        cell_use_start = line.find(max_cell_usage_start) + len(max_cell_usage_start)
        cell_use_end = line.find(max_cell_usage_end)
        if max_cell_usage_start in line:
            sum_max_cell_usage = sum_max_cell_usage + int(line[cell_use_start:cell_use_end])

        # check sink node receive the seq-num ?
        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
        crtc_end_sing = line.find(addr_src_end_sing)
        if key_line_rx_sink in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
            seq_s = line.find(seq_start_sing) + len(seq_start_sing)
            seq_e = line.find(seq_end_sing)
            sent_port_s = line.find(sent_port_start_sing) + len(sent_port_start_sing)
            sent_port_e = line.find(sent_port_end_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1020":
                sink_rx_success_array[int(line[seq_s:seq_e]) - 1] = 1

        # calculate num tx_rtx and tx-ok
        crtc_start_sing = line.find(app_addr_src_start_sing) + len(app_addr_src_start_sing)
        crtc_end_sing = line.find(app_addr_src_end_sing)
        if key_line_tx in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
            seq_s = line.find(app_seq_start_sing) + len(app_seq_start_sing)
            seq_e = line.find(app_seq_end_sing)
            tx_status_s = line.find(app_tx_status_start_sing) + len(app_tx_status_start_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1:
                num_tx_array[int(line[seq_s:seq_e]) - 1] += 1
                if int(line[tx_status_s:tx_status_s + 1]) == 0:
                    tx_ok_array[int(line[seq_s:seq_e]) - 1] += 1

        # num rx-ok
        crtc_start_sing = line.find(app_rx_addr_src_start_sing) + len(app_rx_addr_src_start_sing)
        crtc_end_sing = line.find(app_rx_addr_src_end_sing)
        if key_line_rx in line and line[crtc_start_sing:crtc_end_sing] == ('0' + str(end_node_id)):
            seq_s = line.find(app_rx_seq_start_sing) + len(app_rx_seq_start_sing)
            seq_e = line.find(app_rx_seq_end_sing)
            if int(line[seq_s:seq_e]) < packet_num + 1:
                rx_num_receive_array[int(line[seq_s:seq_e]) - 1] += 1

        # num hop for this node id
        crtc_start_sing = line.find(num_hop_start_sing) + len(num_hop_start_sing)
        crtc_end_sing = line.find(num_hop_end_sing)
        addr_start_sing = line.find(num_hop_addr_start_sing) + len(num_hop_addr_start_sing)
        addr_end_sing = line.find(num_hop_addr_end_sing)
        if num_hop_start_sing in line and line[addr_start_sing:addr_end_sing] == ('0' + str(end_node_id)):
            num_hop = int(line[crtc_start_sing:crtc_end_sing])

    for i in range(packet_num):
        if num_tx_array[i] != 0:
            fail_tx = num_tx_array[i] - tx_ok_array[i]
            ack_intf_array[i] = rx_num_receive_array[i] - tx_ok_array[i]
            packet_intf_array[i] = fail_tx - ack_intf_array[i]
            queue_drop_array[i] = None
        else:
            queue_drop_array[i] = 1
            ack_intf_array[i] = None
            packet_intf_array[i] = None

    new_num_tx_array = []
    if num_hop != 0:
        for number in num_tx_array:
            new_num_tx_array.append(number / num_hop)

    return new_num_tx_array, packet_intf_array, ack_intf_array, queue_drop_array, sum_max_cell_usage/end_node_id


def split_converge_time_steady_state(e2e_delay_list):
    print(e2e_delay_list)
    convergence_point = None
    new_conv_point = None
    std_point = 800
    w_len = 200
    w_sample = 20
    w = e2e_delay_list[std_point:std_point + w_len]
    if not all(v is None for v in w):
        std_median = mean(d for d in w if d is not None)
        w_new = list(filter(None, w))
        # print(std_median)
        ci = st.norm.interval(alpha=.99, loc=np.mean(w_new), scale=st.sem(w_new))
        # print('confidence interval: ' + str(ci))
        convergence_point = 0
        new_conv_point = 0
        # for item in range(std_point):
        #     if e2e_delay_list[std_point - item] > (std_median * 2):
        #         convergence_point = std_point - item
        #         print("my conve point: " + str(convergence_point))
        #         # print(std_point - i + w_len)
        #         # print(e2e_delay_list[0:std_point - i])
        #         # print(e2e_delay_list[std_point - i:std_point])
        #         break
        for item in range(std_point - w_sample):
            tw = e2e_delay_list[std_point - item - w_sample: std_point - item]
            tmp_win = list(filter(None, tw))
            if len(tmp_win) > 15:
                if all(x > ci[1] for x in tmp_win):
                    new_conv_point = std_point - item
                    break
            else:
                new_conv_point = simu_len
                break

    else:
        return simu_len
    return new_conv_point


def get_convergence_time_sdn_tsch(log_file_name, end_node_id):
    log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
    file = open(log_path, "r")
    input_lines = file.readlines()
    file.close()
    send_asn = 0
    receive_asn = 0
    for line in input_lines:
        sender_addr_sing_s = line.find(flow_req_start_sing) + len(flow_req_start_sing)
        sender_addr_sing_e = line.find(flow_req_end_sing)
        if flow_req_start_sing in line and line[sender_addr_sing_s:sender_addr_sing_e] == ('0' + str(end_node_id)):
            asn_sing_s = line.find(flow_req_asn_sing_start) + len(flow_req_asn_sing_start)
            asn_sing_e = line.find(flow_req_asn_sing_end)
            send_asn = int(line[asn_sing_s:asn_sing_e], 16)

        receiver_addr_sing_s = line.find(flow_resp_start_sing) + len(flow_resp_start_sing)
        receiver_addr_sing_e = line.find(flow_resp_end_sing)
        if flow_resp_start_sing in line and line[receiver_addr_sing_s:receiver_addr_sing_e] == ('0' + str(end_node_id)):
            asn_sing_s = line.find(flow_resp_asn_sing_start) + len(flow_resp_asn_sing_start)
            asn_sing_e = line.find(flow_resp_asn_sing_end)
            receive_asn = int(line[asn_sing_s:asn_sing_e], 16)

    return (receive_asn - send_asn) * 10


for i in net_size_array:
    msf_net_log_list = get_list_of_log_file(msf_filename_key + str(i), log_list)
    for m in lqr_array:
        msf_lqr_log_list = get_list_of_log_file(str(m), msf_net_log_list)
        for j in msf_lqr_log_list:
            delay_array, pdr = e2e_delay(j, i)
            # if all(s is None for s in delay_array[980:999]):
            #     print('MSF problematic log: ' + str(j))

            std_delay = None
            if not all(s is None for s in delay_array):
                std_delay = mean(d for d in delay_array if d is not None)

            if not (std_delay is None):
                temp_std_delay = [std_delay / 1000, 'MSF(LQR ' + str(m * 100) + '%)', i]
                df_cp_len = len(std_state_delay_df)
                std_state_delay_df.loc[df_cp_len] = temp_std_delay

            temp_pdr = [pdr, 'MSF(LQR ' + str(m * 100) + '%)', i]
            df_pdr_len = len(pdr_df)
            pdr_df.loc[df_pdr_len] = temp_pdr

            print('delay array of ' + str(j) + ' ' + str(delay_array))
            if not all(v is None for v in delay_array):
                conv_point = split_converge_time_steady_state(delay_array)
                # if m == 0.7:
                #     print('delay array: ' + str(delay_array))
                #     print('Conv point: ' + str(conv_point))
                print('MSF conv point ' + str(conv_point))
                if conv_point is not None:
                    temp_pdr = [pdr, 'MSF(LQR '+str(m*100)+'%)', i]
                    df_pdr_len = len(pdr_df)
                    pdr_df.loc[df_pdr_len] = temp_pdr

                    conv_delay = None
                    if not all(s is None for s in delay_array[0:conv_point]):
                        conv_delay = mean(d for d in delay_array[0:conv_point] if d is not None)

                    temp_conv_point = [conv_point * app_int, 'MSF(LQR '+str(m*100)+'%)', i]
                    df_cp_len = len(convergence_df)
                    convergence_df.loc[df_cp_len] = temp_conv_point

                    if not (conv_delay is None):
                        temp_conv_delay = [conv_delay/1000, 'MSF(LQR '+str(m*100)+'%)', i]
                        df_cd_len = len(convergence_delay_df)
                        convergence_delay_df.loc[df_cd_len] = temp_conv_delay

                else:
                    print('Convergence point is None for log: ' + str(j))
                    temp_conv_point = [1000 * app_int, 'MSF(LQR ' + str(m * 100) + '%)', i]
                    df_cp_len = len(convergence_df)
                    convergence_df.loc[df_cp_len] = temp_conv_point
            else:
                print('all the delay-elements are None in this log' + str(j))
                temp_conv_point = [1000 * app_int, 'MSF(LQR ' + str(m * 100) + '%)', i]
                df_cp_len = len(convergence_df)
                convergence_df.loc[df_cp_len] = temp_conv_point

    sdn_net_log_list = get_list_of_log_file(sdn_filename_key + str(i), log_list)
    for m in lqr_array:
        sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
        for j in sdn_lqr_log_list:
            delay_array, pdr = e2e_delay(j, i)
            if pdr < 100:
                print('low pdr:  ---> ' + str(j) + '  "PDR = '+str(pdr)+'"')
                print(delay_array)
            # if all(s is None for s in delay_array[980:999]):
            #     print('problematic log: ' + str(j))
            if not all(v is None for v in delay_array):
                temp_pdr = [pdr, 'SDN-TSCH(LQR '+str(m*100)+'%)', i]
                df_pdr_len = len(pdr_df)
                pdr_df.loc[df_pdr_len] = temp_pdr

                conv_time = get_convergence_time_sdn_tsch(j, i)
                if conv_time != 0:
                    # print(conv_time/1000)
                    temp_conv_point = [conv_time/1000, 'SDN-TSCH(LQR '+str(m*100)+'%)', i]
                    df_cp_len = len(convergence_df)
                    convergence_df.loc[df_cp_len] = temp_conv_point

                    # if std_delay/1000 > 2:
                    #     print('SDN large delay:  ' + str(j))
                    #     print(delay_array)
                    # print(delay_array)
                    # print(std_delay/1000)
                    # print(pdr)
                for delay in delay_array:
                    if not (delay is None):
                        temp_std_delay = [delay/1000, 'SDN-TSCH(LQR '+str(m*100)+'%)', i]
                        df_cp_len = len(std_state_delay_df)
                        std_state_delay_df.loc[df_cp_len] = temp_std_delay


for i in net_size_array:
    msf_net_log_list = get_list_of_log_file(msf_filename_key + str(i), log_list)
    for m in lqr_array:
        msf_lqr_log_list = get_list_of_log_file(str(m), msf_net_log_list)
        for j in msf_lqr_log_list:
            num_sch_ts = get_num_scheduled_ts(j, i)
            print(num_sch_ts)
            for index, val in enumerate(num_sch_ts):
                temp_pdr = [val / 2, 'MSF: hop' + str(index + 1), i]
                df_pdr_len = len(ts_num_df)
                ts_num_df.loc[df_pdr_len] = temp_pdr

            cell_usage = get_max_cell_usage_per_sf(j, i)
            if i == 3 and cell_usage < 8:
                print('low cell usage: ' + str(j))
            temp_pdr = [cell_usage, 'MSF(LQR ' + str(m * 100) + '%)', i]
            df_pdr_len = len(sf_max_cell_usage_df)
            sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

    sdn_net_log_list = get_list_of_log_file(sdn_filename_key + str(i), log_list)
    packet_intf = []
    ack_intf = []
    queue_drop = []
    for m in lqr_array:
        sdn_lqr_log_list = get_list_of_log_file(str(m), sdn_net_log_list)
        for j in sdn_lqr_log_list:

            num_tx_array, packet_intf_array, ack_intf_array, queue_drop_array, sf_cell_usage = number_of_tx_and_collision(j, i)
            temp_pdr = [sf_cell_usage, 'SDN-TSCH(LQR ' + str(m * 100) + '%)', i]
            df_pdr_len = len(sf_max_cell_usage_df)
            sf_max_cell_usage_df.loc[df_pdr_len] = temp_pdr

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
            if m == 0.84:
                num_sch_ts = get_num_scheduled_ts(j, i)
                print(num_sch_ts)
                for index, val in enumerate(num_sch_ts):
                    temp_pdr = [val/2, 'SDN-TSCH: hop' + str(index + 1), i]
                    df_pdr_len = len(ts_num_df)
                    ts_num_df.loc[df_pdr_len] = temp_pdr
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
# ################## plot stacked re-tx #######################

# cause_rtx_df.plot(x='Network size', kind='bar', stacked=True, fontsize='15')
# plt.xlabel("Network size", fontsize='15')
# plt.ylabel("Cause of retransmission (%)", fontsize='15')
# plt.savefig('stack_rtx', bbox_inches='tight')


# ################## plot convergence time #######################
hue_order = ['MSF(LQR 95.0%)', 'SDN-TSCH(LQR 95.0%)', 'MSF(LQR 84.0%)', 'SDN-TSCH(LQR 84.0%)', 'MSF(LQR 71.0%)', 'SDN-TSCH(LQR 71.0%)']
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=convergence_df, x='Network size', y='Convergence time (s)',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
#ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('Convergence_time.pdf', bbox_inches='tight')

# ################## plot std state delay #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=std_state_delay_df, x='Network size', y='End-to-End delay(s)',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
#ax_sdn_pdr.set(ylim=(0, 10))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('delay_std_state.pdf', bbox_inches='tight')

# ################## plot PDR #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=pdr_df, x='Network size', y='PDR(%)',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
# for ax in ax_sdn_pdr.axes.flatten():
#     ax.collections[0].set_edgecolor('tab:blue')
#     ax.collections[4].set_edgecolor('tab:blue')
#     ax.collections[8].set_edgecolor('tab:blue')

plt.savefig('pdr.pdf', bbox_inches='tight')

# ################## plot number of TX+rTX #######################
# sns.set(font_scale=3.2)
# sns.set_style({'font.family': 'serif'})
# ax_sdn_pdr = sns.catplot(kind='violin', data=num_tx_df, x='Network size', y='Number of TX+RTX',
#                          hue='Network Solution', palette="tab10", cut=0, scale='width',
#                          saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# # plt.subplots_adjust(top=.8)
# plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# # plt.yticks(range(0,110,20))
# # ax_sdn_pdr.fig.set_figwidth(20)
# # ax_sdn_pdr.fig.set_figheight(10)
# plt.savefig('num_tx.pdf', bbox_inches='tight')
#
# ################## max cell usage per SF per node #######################
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=sf_max_cell_usage_df, x='Network size', y='Max cell usage per SF and node',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('max_sf_cell.pdf', bbox_inches='tight')
# ################## plot scheduled timeslots #######################
hue_order = ['MSF: hop1', 'SDN-TSCH: hop1', 'MSF: hop2', 'SDN-TSCH: hop2', 'MSF: hop3', 'SDN-TSCH: hop3']
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=ts_num_df, x='Network size', y='Avg. scheduled timeslots per node',
                         hue='Network Solution', hue_order=hue_order, palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
# ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('scheduled-ts.pdf', bbox_inches='tight')



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
#     log_path = "/home/fvg/contiki-ng/examples/sdn_udp/msf-statistics/log/" + str(log_file_name)
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

