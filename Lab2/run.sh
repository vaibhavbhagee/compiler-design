make clean
make
./cc examples/link-try.c
cat generated_code.txt
lli generated_code.txt
