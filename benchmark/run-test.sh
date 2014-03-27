#! /bin/bash

rm -rf "$2run_test_tmp"
mkdir "$2run_test_tmp"
pwd=`pwd`
cd "$2run_test_tmp"
niter=3
python $pwd/gen-test-case.py "$1" || exit 1
sync

n=$1
output="$2/out-$1.txt"
rm -f $output

# Run the full gamut of tests for one of the tools:
benchmark ()
{
    tool="$1"
    firstbuild="$2"
    update="$3"

    mkdir "build_$tool"
    cd "build_$tool"
    # try to make the SO cache some files
    find .. -type f -name *.c -o -name *.h | while read i; do cat $i > /dev/null; done

    echo "first build, tool=$tool, n=$n"
    echo "$tool: initial" >> $output
    { time -p eval "$firstbuild"; } 2>> $output
    sync
    cfile="../module0/0.c";
    hfile="../module0/0.h";

    echo "0.c touched, tool=$tool, n=$n"
    echo "$tool: 0.c touched" >> $output
    for i in `seq 1 $niter`; do
        echo "round $i..."
        sleep 1; touch $cfile
        { time -p eval "$update"; } 2>> $output
    done

    echo "0.h touched, tool=$tool, n=$n"
    echo "$tool: 0.h touched" >> $output
    for i in `seq 1 $niter`; do
        echo "round $i..."
        sleep 1; touch $hfile
        { time -p eval "$update"; } 2>> $output
    done

    echo "doing nothing, tool=$tool, n=$n"
    echo "$tool: nothing" >> $output
    for i in `seq 1 $niter`; do
        echo "round $i..."
        { time -p eval "$update"; } 2>> $output
    done

    cd ..
}

# Default number of jobs for make = number of cores + 1
j=`grep -c ^processor /proc/cpuinfo`
j=`expr $j + 1`

benchmark "cmake_make" "cmake .. > /dev/null && make -j$j > /dev/null" "make -j$j > /dev/null"
benchmark "cmake_ninja" "cmake -G\"Ninja\" .. > /dev/null && ninja > /dev/null" "ninja > /dev/null"
benchmark "meique" "../../../src/meique .. > /dev/null" "../../../src/meique > /dev/null"

echo "done!"