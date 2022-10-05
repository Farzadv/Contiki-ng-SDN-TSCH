import numpy as np
import pandas as pd
import pickle
import json

lq_list = [0.7, 0.9]
hop_list = [1, 2, 3]

qos_pdr = 0.99

sum_cell = []

for lq in lq_list:
    lq_list = []
    for hop in hop_list:
        e2e_pdr = lq
        pdr_hop = [lq] * hop
        hop_num_cell = [1] * hop
        last_hop = -1
        while e2e_pdr < qos_pdr:
            e2e_pdr = np.prod(pdr_hop)
            if e2e_pdr < qos_pdr:
                if last_hop == len(pdr_hop) - 1:
                    last_hop = 0
                else:
                    last_hop = last_hop + 1

                hop_num_cell[last_hop] = hop_num_cell[last_hop] + 1
                pdr_hop[last_hop] = 1 - ((1 - lq) ** hop_num_cell[last_hop])
        lq_list.append(sum(hop_num_cell))

    sum_cell.append(lq_list)

# print(sum_cell)
df = pd.DataFrame(sum_cell, columns=['Net_2', 'Net_3', 'Net_4'])
df.index = ['LQR_70%', 'LQR_90%']
print(df)

l = [[1, 2], [4, 5]]
with open("test", "w") as fp:
    json.dump(l, fp)

with open("test", "r") as fp:
    b = json.load(fp)

a = open('test1', "w+")
a.write('l = ' + str(l))
a.close()

a = open('test1', "r")
input_lines = a.readlines()
print(input_lines)
a.close()
# print(b)