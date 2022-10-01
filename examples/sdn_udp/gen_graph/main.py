import random
import math


def random_gen(minimum, maximum):
    return minimum + (maximum - minimum) * random.random()


def set_nodes_position(nodes_num, x_radius, y_radius, tx_range):
    find_loc = 1
    max_nbr = 8
    pos_array = nodes_num * [2*[0]]
    while find_loc < nodes_num:
        p1 = [random_gen(-x_radius, x_radius), random_gen(-y_radius, y_radius)]
        for i in range(nodes_num):
            if i < find_loc and math.sqrt(((p1[0] - pos_array[i][0]) ** 2) + ((p1[1] - pos_array[i][1]) ** 2)) < (tx_range - 3):
                if pos_array[0][0] == 0 and pos_array[0][1] == 0:
                    pos_array[find_loc-1] = p1
                    break
                else:
                    pos_array[find_loc] = p1
                    find_loc += 1
                    break

    return pos_array


def create_nodes_graph(nodes_num, x_radius, y_radius, tx_range):
    max_nbr = 8
    found_node = 0
    pos_array = []
    p1 = [random_gen(-x_radius, x_radius), random_gen(-y_radius, y_radius)]
    pos_array.append(p1)
    found_node += 1
    while found_node < nodes_num:
        p1 = [random_gen(-x_radius, x_radius), random_gen(-y_radius, y_radius)]
        for i in range(len(pos_array)):
            if math.sqrt(((p1[0] - pos_array[i][0]) ** 2) + ((p1[1] - pos_array[i][1]) ** 2)) < (tx_range - 3):
                pos_array.append(p1)
                exceed_max_nbr = 0
                for j in range(len(pos_array) - 1):
                    nbr_num = 0
                    for k in range(len(pos_array)):
                        if j != k and math.sqrt(((pos_array[j][0] - pos_array[k][0]) ** 2) + ((pos_array[j][1] - pos_array[k][1]) ** 2)) < (tx_range - 3):
                            nbr_num += 1
                    if nbr_num > max_nbr:
                        exceed_max_nbr = 1
                        pos_array.pop()
                        break
                if exceed_max_nbr == 0:
                    found_node += 1
                break
    return pos_array


print(create_nodes_graph(50, 2000, 2000, 50))

#
# l = []
# x = [1, 2]
# y = [33, 66]
# l.append(x)
#
# l.append(y)
# s = [4 , 5]
# l.append(s)
# print(l)
#
# l.pop()
# print(l)


