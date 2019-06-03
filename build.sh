#!/bin/bash
cd ..
mkdir -p build
cd build 
cmake -DCMAKE_BUILD_TYPE=ReleaseWithDebInfo ../CppPhoenixChannels
make -j8
cd ../CppPhoenixChannels
