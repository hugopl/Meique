#! /bin/bash

TUP=tup

rm -rf run_test_tmp
mkdir run_test_tmp
cd run_test_tmp
niter=3
python ../gen-test-case.py "$@" || exit 1
sync

# Run the full gamut of tests for one of the tools:
benchmark ()
{
    tool="$1"
    firstbuild="$2"
    update="$3"

    rm -rf "build_$tool"
    mkdir "build_$tool"
    cd "build_$tool"
    # try to make the SO cache some files
    find .. -type f | while read i; do cat $i > /dev/null; done

    echo "$tool: initial"
    time -p eval "$firstbuild"
    sync
    cfile="../module0/0.c";
    hfile="../module0/0.h";

    echo "$tool: 0.c touched"
    for i in `seq 1 $niter`; do
        sleep 1; touch $cfile
        time -p eval "$update"
    done

    echo "$tool: 0.h touched"
    for i in `seq 1 $niter`; do
        sleep 1; touch $hfile
        time -p eval "$update"
    done

    echo "$tool: nothing"
    for i in `seq 1 $niter`; do
        time -p eval "$update"
    done

    cd ..
}

benchmark "cmake|make" "cmake .. > /dev/null && make -j9 > /dev/null" "make -j9 > /dev/null"
benchmark "meique" "meique .. > /dev/null" "meique > /dev/null"


# benchmark "tup" "$TUP init --force > /dev/null" "$TUP monitor" "$TUP upd > /dev/null" "$TUP stop"

#diff -r tmake ttup | grep -v Makefile | grep -v build | grep -v '\.d$' | grep -v '\.tup'
cd ..
