sudo insmod /NPHeap/npheap.ko
sudo chmod 777 /dev/npheap
cd kernel_module
sudo make clean
make
sudo make install
cd ..
cd library
sudo make clean
make
sudo make install
cd ..
cd benchmark
sudo make clean
sudo make benchmark
sudo make validate
cd ..
sudo insmod kernel_module/tnpheap.ko
sudo chmod 777 /dev/tnpheap
./benchmark/benchmark 4 8192 1





# change this to where your npheap is.
sudo insmod ../NPHeap/kernel_module/npheap.ko
sudo chmod 777 /dev/npheap
sudo insmod kernel_module/tnpheap.ko
sudo chmod 777 /dev/tnpheap
./benchmark/benchmark 256 8192 4
cat *.log > trace
sort -n -k 3 trace > sorted_trace
./benchmark/validate 256 8192 < sorted_trace
rm -f *.log
sudo rmmod tnpheap
sudo rmmod npheap
