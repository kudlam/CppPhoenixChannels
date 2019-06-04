#!/bin/bash
cd ..
mkdir -p build
cd build 
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../CppPhoenixChannels
make -j8
objcopy --only-keep-debug CppPhoenixChannels CppPhoenixChannels.dbg
objcopy --strip-debug CppPhoenixChannels
cd ../CppPhoenixChannels
