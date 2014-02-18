#!/bin/sh

fatalError()
{
    echo "Something went wrong :-(, try to fix the shellscript and/or file a bug on https://github.com/hugopl/Meique/issues"
    exit 1
}

echo '  __  __      _                  '
echo ' |  \/  |    (_)                 '
echo ' | \  / | ___ _  __ _ _   _  ___ '
echo ' | |\/| |/ _ \ |/ _` | | | |/ _ \'
echo ' | |  | |  __/ | (_| | |_| |  __/'
echo ' |_|  |_|\___|_|\__, |\__,_|\___|'
echo '                   | |           '
echo '                   |_|  Bootstrap'
echo ''

# compile file2c
mkdir -p build
cd build
echo "Compiling file2c..."
g++ -o file2c -O2 ../ext/file2c/file2c.cpp
echo "Generating meiqueapi.cpp..."
./file2c meiqueApi ../src/meiqueapi.lua > meiqueapi.cpp

srcs="`ls -1 ../src/*.cpp ../ext/lua/*.cpp`"

echo "Compiling meique in one go, this can take some minutes..."
g++ -std=c++11 -o meique -O2 -I. -DNDEBUG -DLUA_USE_POSIX -lpthread meiqueapi.cpp $srcs -I../ext/lua -lpthread

if [ ! -r ./meique ]; then
    fatalError
fi

numJobs=1
if [ -r /proc/cpuinfo ]; then
  numJobs=`grep 'physical id' /proc/cpuinfo | wc -l`
fi
numJobs=`expr $numJobs + 1`

echo "Compiling meique using bootstraped meique :-)"
./meique -j$numJobs ..

if [ -r ./src/meique ]; then
    echo "All done! Go into build dir (cd build) and type \"./meique -i\" to install meique on /usr or \"DESTDIR=MYPREFIX ./meique -i\" to install meique on MYPREFIX."
else
    fatalError
fi
