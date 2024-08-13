make clean
make test -j15
make test_asm -j15
time make test_exe -j15
make test_output -j15