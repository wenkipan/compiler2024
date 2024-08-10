make clean
make test -j28
make test_asm -j28
time make test_exe -j28
make test_output -j28