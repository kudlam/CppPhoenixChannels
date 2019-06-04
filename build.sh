#!/bin/bash
mkdir -p build
cd build 
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../
make -j8
objcopy --only-keep-debug CppPhoenixChannels CppPhoenixChannels.dbg
objcopy --strip-debug CppPhoenixChannels
