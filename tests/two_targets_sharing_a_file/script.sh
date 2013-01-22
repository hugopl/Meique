$MEIQUE .. lib
$MEIQUE exe

if [ `./exe` != "EXECUTABLE" ]
then
    fail "main.cpp should use foo.cpp compiled with TARGET_NAME=EXECUTABLE!"
fi

$MEIQUE libUser

if [ `./libUser` != "LIBRARY" ]
then
    fail "main.cpp on libUser should use foo.cpp compiled with TARGET_NAME=LIBRARY!"
fi
