#!/bin/bash
TARGET=z1
COOJA_BUILD=1

mkdir cooja_firmwares

make TARGET=$TARGET DEFINES=COOJA=$COOJA_BUILD,SINK=1
mv sdn-project.$TARGET cooja_firmwares/sink.$TARGET 
make TARGET=$TARGET clean

make TARGET=$TARGET DEFINES=COOJA=$COOJA_BUILD,SINK=0
mv sdn-project.$TARGET cooja_firmwares/node.$TARGET 
make TARGET=$TARGET clean

