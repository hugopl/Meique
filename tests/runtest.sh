#!/usr/bin/env sh
function die()
{
    echo $1
    exit 1
}

function fail()
{
    echo -e "\n*** $1"
    exit 1
}

if test $# -ne 3; then die "Use runtest.sh TESTE_DIR  WORKING_DIR  MEIQUE_EXECUTABLE"; fi
if [ ! -d $1 ]; then die "Test not found"; fi
if [ ! -x $3 ]; then die "Meique executable not found."; fi

TESTNAME=`basename $1`
TESTDIR=$2/$TESTNAME
MEIQUE="$3 -d"

# clone test
rm -rf $TESTDIR
mkdir -p $TESTDIR/build
cp -rv $1 $2

#run test
cd $TESTDIR/build
source ../script.sh
exit 0
