#!/bin/bash

set -e

if [[ "$1" == "--me" ]]; then
    SHARE=false
    echo "NOT BUILDING TO SHARE"
else
    SHARE=true
    echo "BUILDING TO SHARE"
fi

current_branch=$(git rev-parse --abbrev-ref HEAD)

# Check for detached HEAD
if [ "$current_branch" = "HEAD" ]; then
    return_to_branch=""
else
    return_to_branch=$current_branch
fi

rm -rf release

# Function to build and move firmware
build_and_move() {
    local board="$1"
    local display="$2"
    local branch="$3"

    make clean
    if $SHARE; then
        make BOARD="$board" DISPLAY="$display" SHARE=true
    else
        make BOARD="$board" DISPLAY="$display"
    fi
    mkdir -p "release/$branch"
    mv build/firmware.uf2 "release/$branch/firmware-${board}-${display}.uf2"
}

build_all_for_branch() {
    local branch="$1"
    git checkout "$branch"
    build_and_move sensorwatch_pro classic "$branch"
    build_and_move sensorwatch_pro custom "$branch"
    build_and_move sensorwatch_red custom "$branch"
    build_and_move sensorwatch_red classic "$branch"
}

build_all_for_branch devolov

if [ -n "$return_to_branch" ]; then
    git checkout "$return_to_branch"
fi

make clean