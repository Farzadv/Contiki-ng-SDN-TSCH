import os
# import numpy as np
# import pandas as pd
# import seaborn as sns
import matplotlib.pyplot as plt
# import glob

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

file = open("/home/fvg/contiki-ng/examples/sdn_udp/msf-scenario/COOJA.testlog", "r")
input_lines = file.readlines()
file.close()


key_line = "11111111 "
tot_rx_count1 = 0
tot_rx_count2 = 0
packet_num = 1000
initial_drift = 300
lat_critic_array1 = [None] * packet_num
lat_critic_array2 = [None] * packet_num

for line in input_lines:
    crtc_start_sing = line.find(addr_src_start_sing) + len(addr_src_start_sing)
    crtc_end_sing = line.find(addr_src_end_sing)
    if key_line in line and line[crtc_start_sing:crtc_end_sing] == "05":
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

        if int(line[seq_s:seq_e]) < packet_num + 1 and line[sent_port_s:sent_port_e] == "1030":
            tot_rx_count2 += 1
            lat_critic_array2[int(line[seq_s:seq_e]) - 1] = (int(line[rcv_asn_s:rcv_asn_e], 16) - int(line[sent_asn_s:sent_asn_e], 16)) * 10




print(tot_rx_count1/packet_num)
print(lat_critic_array1)


print(tot_rx_count2/(packet_num - initial_drift))
print(lat_critic_array2)

plt.plot(lat_critic_array1)
plt.savefig('msf_pdr11.png', bbox_inches='tight')

plt.plot(lat_critic_array2)
plt.savefig('msf_pdr22.png', bbox_inches='tight')
