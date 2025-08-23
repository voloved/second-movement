#!/bin/bash

set -e  # Exit on any error

rm -rf release
mkdir -p release

make clean
make BOARD=sensorwatch_pro DISPLAY=classic
mv build/firmware.uf2 release/firmware-pro-classic.uf2

make clean
make BOARD=sensorwatch_pro DISPLAY=custom
mv build/firmware.uf2 release/firmware-pro-custom.uf2

make clean
make BOARD=sensorwatch_red DISPLAY=classic
mv build/firmware.uf2 release/firmware-red-custom.uf2

make clean
make BOARD=sensorwatch_red DISPLAY=classic
mv build/firmware.uf2 release/firmware-red-classic.uf2

make clean