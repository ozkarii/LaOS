rm -rf build

cmake -B build

cmake --build build

cd build

ctest

cd ../