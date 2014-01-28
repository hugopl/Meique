$MEIQUE .. lib
$MEIQUE exe

EXE=`./exe` || fail "Target not compiled!?"
if [ $EXE != "EXECUTABLE" ]
then
    fail "main.cpp should use foo.cpp compiled with TARGET_NAME=EXECUTABLE!"
fi

$MEIQUE libUser

LIBUSER=`./libUser` || fail "Target not compiled!?"
if [ $LIBUSER != "LIBRARY" ]
then
    fail "main.cpp on libUser should use foo.cpp compiled with TARGET_NAME=LIBRARY!"
fi
