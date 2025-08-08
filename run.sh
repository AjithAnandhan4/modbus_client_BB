#!/bin/bash

rm -rf build
mkdir build
cd build || { echo "Failed to enter build directory"; exit 1; }
cmake ..
make

