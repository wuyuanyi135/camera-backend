#!/usr/bin/env bash

mkdir -p build
cd build
#cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake ..
# this will generate some code that can not be captured by the file glob.
cmake --build . --target proto
cmake ..
make -j`nproc`
make install
cd build/camera_backend
for fn in `ls -1 `; do
  ldd $fn | grep '=> /' | awk '{print $3}' | xargs -I '{}' cp -v '{}'  ./ || :
done