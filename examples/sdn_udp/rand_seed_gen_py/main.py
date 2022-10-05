import random
import sys


# read input parameter-> check is tsch-sdn or tsch-orch ? => sys.argv[1]

# generate a random seed
rand_seed = random.randint(1, 9)
for i in range(5):
    rand_seed = (rand_seed*10) + random.randint(1, 9)


# update random seed of config file
key_word = "<randomseed>"
replace_word = "    <randomseed>"+str(rand_seed)+"</randomseed>\n"

orch_sf_key1 = ''
orch_sf_key2 = ''
orch_sf_replace1 = ''
orch_sf_replace2 = ''


if sys.argv[1] == "tsch-orch":
    orch_sf_key1 = "MAKE_COMMAND_SRV"
    orch_sf_key2 = "MAKE_COMMAND_CLIENT"
    orch_sf_key3 = "MAKE_COMMAND_CRITIC_CLIENT"
    orch_sf_replace1 = "make udp-server.cooja TARGET=cooja DEFINES=ORCHESTRA_CONF_UNICAST_PERIOD="+str(sys.argv[2])+"</commands>\n"
    orch_sf_replace2 = "make udp-client.cooja TARGET=cooja DEFINES=APP_INT="+str(sys.argv[3])+",ORCHESTRA_CONF_UNICAST_PERIOD="+str(sys.argv[2])+"</commands>\n"
    orch_sf_replace3 = "make udp-client-critic.cooja TARGET=cooja DEFINES=APP_INT=" + str(
        sys.argv[4]) + ",ORCHESTRA_CONF_UNICAST_PERIOD=" + str(sys.argv[2]) + "</commands>\n"

################# Update TSCH-SDN seed ##############
if sys.argv[1] == "tsch-sdn":
    file = open("/home/fvg/contiki-ng/examples/sdn_udp/config.csc", "r")
    input_lines = file.readlines()
    file.close()

    for line in input_lines:
        if key_word in line:
            input_lines[input_lines.index(line)] = replace_word
            break

    file = open("/home/fvg/contiki-ng/examples/sdn_udp/config.csc", "w+")
    file.writelines(input_lines)
    file.close()
    print("====> TSCH-SDN: UPDATE RANDOM_SEED")

################# Update TSCH-ORCH seed ##############
if sys.argv[1] == "tsch-orch":
    file = open("/home/fvg/contiki-ng/examples/sdn_udp/config_orch.csc", "r")
    input_lines = file.readlines()
    file.close()

    for line in input_lines:
        if key_word in line:
            input_lines[input_lines.index(line)] = replace_word
            break

    for line in input_lines:
        if orch_sf_key1 in line:
            input_lines[input_lines.index(line)] = orch_sf_replace1
            break

    for line in input_lines:
        if orch_sf_key2 in line:
            input_lines[input_lines.index(line)] = orch_sf_replace2
            break

    for line in input_lines:
        if orch_sf_key3 in line:
            input_lines[input_lines.index(line)] = orch_sf_replace3
            break

    file = open("/home/fvg/contiki-ng/examples/sdn_udp/config_orch.csc", "w+")
    file.writelines(input_lines)
    file.close()
    print("====> TSCH-ORCH: UPDATE RANDOM_SEED")