#!/bin/bash

if [ "$1" == "clean" ]; then
  rm -rf build
  mkdir build
  cd build
  cmake ..
  make
else
  cd build || mkdir build && cd build
  cmake --build .
fi
