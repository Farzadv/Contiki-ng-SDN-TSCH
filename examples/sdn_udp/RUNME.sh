#!/bin/bash
TARGET=cooja

mkdir cooja_firmwares

make TARGET=$TARGET udp-server DEFINES=SINK=1
mv udp-server.$TARGET cooja_firmwares/sink.$TARGET 
make TARGET=$TARGET clean

make TARGET=$TARGET udp-client DEFINES=SINK=0
mv udp-client.$TARGET cooja_firmwares/node.$TARGET 
make TARGET=$TARGET clean


#make TARGET=$TARGET udp-server DEFINES=SINK=1
#make TARGET=$TARGET udp-client DEFINES=SINK=0
