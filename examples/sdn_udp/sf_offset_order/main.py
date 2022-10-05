import sys
import shutil
import fileinput
import os
from func_lib import *
from conf_tmp import *
import random
# import numpy as np


offs_list_6 = [1, 5, 3, 2, 6, 4]
offs_list_12 = [1, 9, 5, 3, 10, 6, 2, 11, 7, 4, 12, 8]
offs_list_18 = [1, 11, 9, 5, 12, 3, 13, 7, 14, 2, 15, 10, 6, 16, 4, 17, 8, 18]
offs_list_24 = [1, 17, 9, 5, 18, 10, 3, 19, 11, 6, 20, 12, 2, 21, 13, 7, 22, 14, 4, 23, 15, 8, 24, 16]
offs_list_30 = [1, 17, 9, 18, 5, 19, 10, 20, 3, 21, 11, 22, 7, 23, 15, 2, 24, 12, 25, 6, 26, 13, 27, 4, 28, 14, 29, 8, 30, 16]
# print('====> INPUT PARAMITER    =   ', str(sys.argv))
# param_input = sys.argv
#
# sf_size = get_val_from_input_array(param_input, "sf_size")
# sf_rep_period = get_val_from_input_array(param_input, "sf_rep_period")
# ctrl_sf_size = get_val_from_input_array(param_input, "ctrl_sf_size")
# node_num = get_val_from_input_array(param_input, "node_num")
# server_num = get_val_from_input_array(param_input, "server_num")
# client_bkg_num = get_val_from_input_array(param_input, "client_bkg_num")
# client_critic_num = get_val_from_input_array(param_input, "client_critic_num")
# bkg_traffic_period = get_val_from_input_array(param_input, "bkg_traffic_period")  # second
# critic_traffic_period = get_val_from_input_array(param_input, "critic_traffic_period")  # second
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
