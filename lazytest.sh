make clean
make test -j16
make test_asm -j16
time make test_exe -j16
make test_output -j16