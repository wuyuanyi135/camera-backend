#!/usr/bin/env bash

if [[ -z "${CMAKE_DURING_BUILD}" ]]; then
    echo "Skip building phase"
else
    mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
fi