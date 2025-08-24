#!/bin/bash

set -e  # Exit on error

rm -rf release

# Function to build and move firmware
build_and_move() {
    local board="$1"
    local display="$2"
    local branch="$3"

    make clean
    make BOARD="$board" DISPLAY="$display" SHARE=true
    mkdir -p release/$branch
    mv build/firmware.uf2 "release/$branch/firmware-${board}-${display}.uf2"
}

build_all_for_branch() {
    local branch="$1"
    git checkout $branch
    build_and_move sensorwatch_pro classic $branch
    build_and_move sensorwatch_pro custom $branch
    build_and_move sensorwatch_red custom $branch
    build_and_move sensorwatch_red classic $branch
}

build_all_for_branch devolov
build_all_for_branch devolov-nocounter32
make clean