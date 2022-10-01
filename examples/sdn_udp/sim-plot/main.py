import os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import glob

pdr_df = pd.DataFrame(columns=['PDR(%)', 'PDR', 'Network size'])
delay_df = pd.DataFrame(columns=['End-to-End delay(s)', 'Delay', 'Network size'])
ts_num_df = pd.DataFrame(columns=['Scheduled timeslots(%)', 'Timeslots', 'Network size'])

net_size_array = [5, 10, 15]
orch_sf_size_array = [17, 53, 151]
sdn_bkg_ts_array = [5, 1]
orch_asn_reset = "0x2BF20"
orch_filename_node_num_key = '--orc-n'
orch_filename_sf_key = '-sf'
orch_pdr_array = []
orch_pdr_avg_array = []
orch_pdr_std_array = []
orch_latency_array = []

sdn_filename_key = '--sdn-n'
sdn_filename_ts_key = '-ts'
sdn_asn_reset = '0x3A980'
sdn_critic_node_list_key_start = 'request a critical fid = ['
sdn_critic_node_list_key_end = '] from controller'
sdn_pdr_array = []

sent_asn_start_sing = 's|as '
sent_asn_end_sign = ' as|ar'
recv_asn_start_sing = '|ar '
recv_asn_end_sing = ' ar|c'
seq_start_sing = 'd|s '
seq_end_sing = ' s|as'
addr_src_start_sing = '|sr '
addr_src_end_sing = ' sr|'
sf_size_start_sing = 'sf_key_size [['
sf_size_end_sing = ']]'
sf_ts_num_start_sing = '*** sf_size[['
sf_ts_num_end_sing = ']] Link Options'

latency_array = []

log_dir = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log"
logfile_list = os.listdir(log_dir)
log_list = sorted(logfile_list)
# print(log_list)


def get_list_of_log_file(filename_key, list_to_search):
    net_log_list = []
    for i in list_to_search:
        if filename_key in i:
            net_log_list.append(i)
    # print(net_log_list)
    return net_log_list


def format_orch_log_file(list_to_search, node_num):
    key_line = "11111111 "
    pdr_array = []
    bkg_lat_array = []
    ts_num_array = []
    for i in list_to_search:
        rx_count = 0
        i_path = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/"+str(i)
        file = open(i_path, "r")
        input_lines = file.readlines()
        file.close()

        sf_size_array = []
        for line in input_lines:
            if sf_size_start_sing in line:
                start_sing = line.find(sf_size_start_sing) + len(sf_size_start_sing)
                end_sing = line.find(sf_size_end_sing)
                if line[start_sing:end_sing] not in sf_size_array:
                    sf_size_array.append(line[start_sing:end_sing])

        sum_ts = 0
        for sf in sf_size_array:
            ts_num = 0
            for line in input_lines:
                if sf_ts_num_start_sing in line:
                    start_sing = line.find(sf_ts_num_start_sing) + len(sf_ts_num_start_sing)
                    end_sing = line.find(sf_ts_num_end_sing)
                    if line[start_sing:end_sing] == sf:
                        ts_num += 1
            sum_ts += ((ts_num * 100) / int(sf))

        if sum_ts != 0:
            ts_num_array.append(sum_ts / node_num)
        # print(ts_num_array)

        new_format = ''
        for node_id in range(2, node_num + 1):
            node_pdr = 0
            node_lat = 0
            str_node_id = '0' + str(node_id)
            for line in input_lines:
                if key_line in line:
                    sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
                    sent_asn_e = line.find(sent_asn_end_sign)
                    rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
                    rcv_asn_e = line.find(recv_asn_end_sing)
                    seq_s = line.find(seq_start_sing) + len(seq_start_sing)
                    seq_e = line.find(seq_end_sing)
                    if int(line[sent_asn_s:sent_asn_e], 16) > int(orch_asn_reset, 16) and int(line[seq_s:seq_e]) < 101:
                        start = line.find(key_line) + len(key_line)
                        s = line[start:]
                        new_format = new_format + s
                        rx_count += 1

                        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
                        crtc_end_sing = line.find(addr_src_end_sing)
                        if line[crtc_start_sing:crtc_end_sing] == str_node_id:
                            node_pdr += 1
                            node_lat += (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16))
            pdr_array.append(node_pdr)
            if node_pdr != 0:
                bkg_lat_array.append((node_lat/node_pdr) / 100)


        index_name = i.find("orc")
        new_path = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/p-"+i[index_name:]
        file_w = open(new_path, "w+")
        file_w.write(new_format)
        file_w.close()
    return pdr_array, bkg_lat_array, ts_num_array


def format_sdn_log_file(list_to_search, node_num):
    key_line = "11111111 "
    pdr_array = []
    pdr_bkg_array = []
    pdr_critic_array = []
    bkg_lat_array = []
    critic_lat_array = []
    ts_num_array = []
    for i in list_to_search:
        critic_node_list = []
        sf_size_array = []
        tot_rx_count = 0
        critic_rx_count = 0
        i_path = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/"+str(i)
        file = open(i_path, "r")
        input_lines = file.readlines()
        file.close()

        for line in input_lines:
            if sdn_critic_node_list_key_start in line:
                start_sing = line.find(sdn_critic_node_list_key_start) + len(sdn_critic_node_list_key_start)
                end_sing = line.find(sdn_critic_node_list_key_end)
                critic_node_list.append(line[start_sing:end_sing])
            if sf_size_start_sing in line:
                start_sing = line.find(sf_size_start_sing) + len(sf_size_start_sing)
                end_sing = line.find(sf_size_end_sing)
                if line[start_sing:end_sing] not in sf_size_array:
                    sf_size_array.append(line[start_sing:end_sing])

        # print(sf_size_array)
        # print(critic_node_list)
        sum_ts = 0
        for sf in sf_size_array:
            ts_num = 0
            for line in input_lines:
                if sf_ts_num_start_sing in line:
                    start_sing = line.find(sf_ts_num_start_sing) + len(sf_ts_num_start_sing)
                    end_sing = line.find(sf_ts_num_end_sing)
                    if line[start_sing:end_sing] == sf:
                        ts_num += 1
            sum_ts += ((ts_num * 100) / int(sf))

        if sum_ts != 0:
            ts_num_array.append(sum_ts / node_num)
        # print(ts_num_array)


        new_format = ''
        for node_id in range(2, node_num + 1):
            lat_critic = 0
            lat_bkg = 0
            critic_node_pdr = 0
            bf_node_pdr = 0
            str_node_id = '0' + str(node_id)
            for line in input_lines:
                if key_line in line:
                    sent_asn_s = line.find(sent_asn_start_sing) + len(sent_asn_start_sing)
                    sent_asn_e = line.find(sent_asn_end_sign)
                    rcv_asn_s = line.find(recv_asn_start_sing) + len(recv_asn_start_sing)
                    rcv_asn_e = line.find(recv_asn_end_sing)
                    seq_s = line.find(seq_start_sing) + len(seq_start_sing)
                    seq_e = line.find(seq_end_sing)
                    if int(line[sent_asn_s:sent_asn_e], 16) > int(sdn_asn_reset, 16) and int(line[seq_s:seq_e]) < 101:
                        start = line.find(key_line) + len(key_line)
                        s = line[start:]
                        new_format = new_format + s
                        tot_rx_count += 1
                        crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
                        crtc_end_sing = line.find(addr_src_end_sing)
                        if line[crtc_start_sing:crtc_end_sing] == str_node_id:
                            if line[crtc_start_sing:crtc_end_sing] in critic_node_list:
                                critic_rx_count += 1
                                critic_node_pdr += 1
                                lat_critic += (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16))
                            else:
                                bf_node_pdr += 1
                                lat_bkg += (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16))
            if str_node_id in critic_node_list:
                pdr_critic_array.append(critic_node_pdr)
                if critic_node_pdr != 0:
                    critic_lat_array.append((lat_critic/critic_node_pdr) / 100)
            else:
                pdr_bkg_array.append(bf_node_pdr)
                if bf_node_pdr != 0:
                    bkg_lat_array.append((lat_bkg/bf_node_pdr) / 100)


        index_name = i.find("sdn")
        new_path = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/q-"+i[index_name:]
        file_w = open(new_path, "w+")
        file_w.write(new_format)
        file_w.close()
    pdr_array.append(pdr_critic_array)
    pdr_array.append(pdr_bkg_array)
    return pdr_array, critic_lat_array, bkg_lat_array, ts_num_array

def plot_sdn_pdr():
    for k in sdn_bkg_ts_array:
        for j in net_size_array:
            a = get_list_of_log_file(sdn_filename_key + str(j), log_list)
            b = get_list_of_log_file(sdn_filename_ts_key + str(k), a)
            node_num_pdr_array, critic_latency, bkg_latency, ts_num = format_sdn_log_file(b, j)
            for pdr in node_num_pdr_array[0]:
                temp = [pdr, 'SDN-TSCH-CR (BF_TS'+str(k)+')', j]
                df_len = len(pdr_df)
                pdr_df.loc[df_len] = temp
            for pdr in node_num_pdr_array[1]:
                temp = [pdr, 'SDN-TSCH-BF (BF_TS'+str(k)+')', j]
                df_len = len(pdr_df)
                pdr_df.loc[df_len] = temp
            for delay in critic_latency:
                temp = [delay, 'SDN-TSCH-CR (BF_TS'+str(k)+')', j]
                df_len = len(delay_df)
                delay_df.loc[df_len] = temp
            for delay in bkg_latency:
                temp = [delay, 'SDN-TSCH-BF (BF_TS'+str(k)+')', j]
                df_len = len(delay_df)
                delay_df.loc[df_len] = temp
            for ts in ts_num:
                temp = [ts, 'SDN-TSCH (BF_TS'+str(k)+')', j]
                df_len = len(ts_num_df)
                ts_num_df.loc[df_len] = temp


        # print(node_num_pdr_array)

        #     sdn_latency_bkg.append(bkg_latency)
        #     sdn_latency_critic.append(critic_latency)
        #     print("sdn pdr :"+str(sdn_pdr_array))
        #
        # latency_array.append(sdn_latency_bkg)
        # latency_array.append(sdn_latency_critic)
        # print("sdn latency: " + str(latency_array))
        #
        #
        # for i in range(len(net_size_array)):
        #     avg = []
        #     sttd = []
        #     for n in range(2):
        #         avg.append(np.mean(sdn_pdr_array[i][n]))
        #         sttd.append(np.std(sdn_pdr_array[i][n]))
        #     sdn_pdr_avg_array.append(avg)
        #     sdn_pdr_std_array.append(sttd)
        # # print("SDN avg PDR: " + str(sdn_pdr_avg_array))
        # leg_label = []
        # # # for i in range(2):
        # leg_label.append("Critical traffic")
        # leg_label.append("Best-effort traffic")
        #
        #
        # # print("SDN PDR legends: " + str(leg_label))
        #
        # bar_width = 0.1
        # x_pos = np.arange(len(net_size_array))
        # y_pos = np.arange(100)
        # print(sdn_pdr_array)
        # fig, ax_sdn_pdr = plt.subplots()
        # for i in range(len(sdn_bkg_ts_array)):
        #     a = []
        #     for j in range(len(net_size_array)):
        #         a.append(sdn_pdr_array[j][i])
        #     print(a)
        # ax_sdn_pdr = sns.violinplot(data=sdn_pdr_array, showmeans=True, showmedians=True, palette="muted")
        # # print(x_pos)
        # ax_sdn_pdr.set_ylabel('PDR (%)')
        # ax_sdn_pdr.set_xlabel('Number of nodes')
        # ax_sdn_pdr.set_xticks(x_pos + bar_width, net_size_array)
        # ax_sdn_pdr.set_title('TSCH-SDN with BKG_TS_NO='+str(k))
        # ax_sdn_pdr.yaxis.grid(True)
        # plt.legend(leg_label, loc=(1.05, 0.5))
        # plt.tight_layout()
        # plt.savefig('TSCH_SDN_pdr_'+str(k)+'_ts.png')
        # #plt.show()
        #


# print(df)

##########################- PLOT ORCHESTRA -############################
def plot_orch_pdr():
    for j in net_size_array:
        n_array = []
        lat = []
        for k in orch_sf_size_array:
            a = get_list_of_log_file(orch_filename_node_num_key+str(j), log_list)
            b = get_list_of_log_file(orch_filename_sf_key+str(k), a)
            p, l , ts_num = format_orch_log_file(b, j)
            n_array.append(p)
            # lat.append(l)
            for pdr in p:
                temp = [pdr, 'ORCH-SF'+str(k), j]
                df_len = len(pdr_df)
                pdr_df.loc[df_len] = temp
            for delay in l:
                temp = [delay, 'ORCH-SF'+str(k), j]
                df_len = len(delay_df)
                delay_df.loc[df_len] = temp
            for ts in ts_num:
                temp = [ts, 'ORCH-SF'+str(k), j]
                df_len = len(ts_num_df)
                ts_num_df.loc[df_len] = temp

        # orch_latency_array.append(lat)
        # orch_pdr_array.append(n_array)

    # print("orch pdr  " + str(orch_pdr_array))
    # # insert orch latency to global latency array
    # for k in range(len(orch_sf_size_array)):
    #     p = []
    #     for j in range(len(net_size_array)):
    #         p.append(orch_latency_array[j][k])
    #     latency_array.append(p)
    #
    # print("sdn latency: " + str(latency_array))
    # print("orch pdr array: " + str(orch_pdr_array))
    # for i in range(len(net_size_array)):
    #     avg = []
    #     sttd = []
    #     for j in range(len(orch_sf_size_array)):
    #         avg.append(np.mean(orch_pdr_array[i][j]))
    #         sttd.append(np.std(orch_pdr_array[i][j]))
    #     orch_pdr_avg_array.append(avg)
    #     orch_pdr_std_array.append(sttd)
    #
    # # print(orch_pdr_avg_array)
    # leg_label = []
    # x_pos = np.arange(len(net_size_array))
    # for i in range(len(orch_sf_size_array)):
    #     leg_label.append('SF_'+str(orch_sf_size_array[i]))
    #
    # # print("Orch PDR legends: " + str(leg_label))
    #
    # bar_width = 0.1
    # fig, ax_orch_pdr = plt.subplots()
    # for i in range(len(orch_sf_size_array)):
    #     a = []
    #     b = []
    #     for j in range(len(net_size_array)):
    #         a.append(orch_pdr_avg_array[j][i])
    #         b.append(orch_pdr_std_array[j][i])
    #
    #     ax_orch_pdr.bar(x_pos + (i * bar_width), a, yerr=b, align='center', width=.09, alpha=0.6, ecolor='black', capsize=5)
    #
    # # print(x_pos)
    # ax_orch_pdr.set_ylabel('PDR (%)')
    # ax_orch_pdr.set_xlabel('Number of nodes')
    # ax_orch_pdr.set_xticks(x_pos + bar_width, net_size_array)
    #
    # ax_orch_pdr.set_title('TSCH-Orchestra PDR')
    # ax_orch_pdr.yaxis.grid(True)
    # plt.legend(leg_label, loc=(1.05, 0.5))
    # plt.tight_layout()
    # plt.savefig('orchestra_pdr.png')
    # # #plt.show()


plot_orch_pdr()
plot_sdn_pdr()
sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_sdn_pdr = sns.catplot(kind='violin', data=pdr_df, x='Network size', y='PDR(%)',
                         hue='PDR', palette="tab10", cut=0, scale='width',
                         saturation=1, legend=False, height=10, aspect=1.8)
ax_sdn_pdr.set(ylim=(0, 102))
# plt.subplots_adjust(top=.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
# plt.yticks(range(0,110,20))
# ax_sdn_pdr.fig.set_figwidth(20)
# ax_sdn_pdr.fig.set_figheight(10)
plt.savefig('pdr.pdf', bbox_inches='tight')

sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_delay = sns.catplot(kind='violin', data=delay_df, x='Network size', y='End-to-End delay(s)',
                       hue='Delay', palette="tab10", cut=0, scale='width', saturation=1,
                       legend=False, height=10, aspect=1.8)
ax_delay.set(ylim=(0, 280))
# ax_delay.fig.set_figwidth(20)
# ax_delay.fig.set_figheight(10)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.26])
plt.savefig('delay.pdf', bbox_inches='tight')

sns.set(font_scale=3.2)
sns.set_style({'font.family': 'serif'})
ax_ts = sns.catplot(kind='violin', data=ts_num_df, x='Network size', y='Scheduled timeslots(%)',
                    hue='Timeslots', palette="tab10", cut=0, scale='width', saturation=1,
                    legend=False, height=10, aspect=1.8)
ax_ts.set(ylim=(0, 22))
# ax_ts.fig.set_figwidth(17)
# ax_ts.fig.set_figheight(8.8)
plt.legend(loc='upper center', ncol=3, fontsize='24', bbox_to_anchor=[0.5, 1.19])
plt.savefig('ts_hist.pdf', bbox_inches='tight')


# kind="strip"
#
#  kind="swarm")
# ####################### latency plot #########################
# def plot_latency():
#     leg_label = []
#     for i in sdn_bkg_ts_array:
#         leg_label.append('TSCH_SDN_BF (ts_no = ' + str(i) + ')')
#         leg_label.append('TSCH_SDN_CR (ts_no = ' + str(i) + ')')
#
#     for i in orch_sf_size_array:
#         leg_label.append('TSCH_ORCH (sf = ' + str(i) + ')')
#
#     fig, ax_latency = plt.subplots()
#     x_pos = np.arange(len(net_size_array))
#     for i in range(len(orch_sf_size_array) + (len(sdn_bkg_ts_array) * 2)):
#         plt.plot(x_pos, latency_array[i])
#
#     ax_latency.set_ylabel('Mean delay (ms)')
#     ax_latency.set_xlabel('Number of nodes')
#     ax_latency.set_xticks(x_pos, net_size_array)
#
#     ax_latency.set_title('End-to-End delay')
#     ax_latency.yaxis.grid(True)
#     plt.legend(leg_label, loc=(1.05, 0.5))
#     plt.tight_layout()
#     plt.savefig('latency.png')
#
#
# plot_latency()
#
#
#
#
#
#
#





















# for j in net_size_array:
#     sdn_pdr_array.append(format_log_file(get_list_of_log_file(sdn_filename_key+str(j)), j))
# print(sdn_pdr_array)

# file = open("/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/23--orc-n10-i4.testlog", "r")
# input_lines = file.readlines()
# file.close()
# key_word = "11111111 "
# new_from = ''
#
# for line in input_lines:
#     if key_word in line:
#         # input_lines[input_lines.index(line)] = replace_word
#         # print(line)
#         start = line.find("11111111 ") + len("11111111 ")
#         # end = line.find("11111111 ")
#         s = line[start:]
#         new_from = new_from + s
#
# # print(new_from)
#
# file_w = open("/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log/cl-23--orc-n10-i4.testlog", "w+")
# file_w.write(new_from)
# file_w.close()

# rootdir = '/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/simulation-log'
#
#
# def listdirs(rootdir):
#     for it in os.scandir(rootdir):
#         if it.is_dir():
#             print(it.path)
#             listdirs(it)
#
# listdirs(rootdir)

# path = "/home/fvg/contiki-ng/examples/sdn_udp/archive/simulation-log"
#
# directory_contents = os.listdir(path)
# print(directory_contents)

# os.chdir("/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/simulation-log")
# for file in glob.glob("*.testlog"):
#     print(file)
# given_path = "/home/fvg/contiki-ng/examples/sdn_udp/sim-plot/log"
# files = os.listdir(given_path)
# print(files)
# print(files[2])
#
#
#

#
# import pandas as pd
# pd.set_option('display.max_columns', 999)
# pd.set_option('display.max_rows', 999)
# pd.set_option('display.width', 999)
#
# a = [[6, 2, 3], [44, 66,99], [47, 8, 5, 4, 3, 8]]
#
# dfs = list()
# for l in a:
#     new_df = pd.DataFrame(dict(value=l, pdr='A', nb_nodes=5))
#     dfs.append(new_df)
# full_df = pd.concat(dfs)
# print (full_df)
#
# import sys
# sys.exit(0)