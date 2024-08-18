make clean
make test -j28
make test_asm -j28
time taskset -c 0,2,4,6,8,10,12,14 make test_exe -j8
make test_output -j28