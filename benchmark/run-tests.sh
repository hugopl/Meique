#! /bin/sh
for i in 1 10 100 1000; do
echo -n "Benchmarking for n=$i..."
./run-test.sh $i > out-$i.txt 2>&1;
echo "done."
done
