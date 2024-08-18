make clean
make test -j8
make test_asm -j8
time make test_exe -j8
make test_output -j8