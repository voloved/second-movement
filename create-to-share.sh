#!/bin/bash

set -e

if [[ "$1" == "--me" ]]; then
    SHARE=false
    RELEASE_DIR="release-no-share"
    echo "NOT BUILDING TO SHARE"
else
    SHARE=true
    RELEASE_DIR="release"
    echo "BUILDING TO SHARE"
fi

TAG_CLEAN=""
if [ -n "$GITHUB_REF_NAME" ]; then
    TAG="${GITHUB_REF_NAME#share-}"
    TAG_CLEAN="-${TAG//./}"
fi

GIT_HASH=$(git rev-parse --short=6 HEAD)

current_branch=$(git rev-parse --abbrev-ref HEAD)

# Check for detached HEAD
if [ "$current_branch" = "HEAD" ]; then
    return_to_branch=""
else
    return_to_branch=$current_branch
fi

rm -rf "$RELEASE_DIR"

# Function to build and move firmware
build_and_move() {
    local board="$1"
    local display="$2"

    make clean
    if $SHARE; then
        make BOARD="$board" DISPLAY="$display" SHARE=true
    else
        make BOARD="$board" DISPLAY="$display"
    fi
    mkdir -p "$RELEASE_DIR"
    mv build/firmware.uf2 "$RELEASE_DIR/firmware-${board}-${display}${TAG_CLEAN}-${GIT_HASH}.uf2"
}

build_all_for_branch() {
    local branch="$1"
    git checkout "$branch"
    build_and_move sensorwatch_jolt jolt
    build_and_move sensorwatch_pro classic
    build_and_move sensorwatch_pro custom
    build_and_move sensorwatch_red custom
    build_and_move sensorwatch_red classic
}

build_all_for_branch jolt

if [ -n "$return_to_branch" ]; then
    git checkout "$return_to_branch"
fi

make clean