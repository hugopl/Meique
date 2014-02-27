$MEIQUE .. || fail "Failed to compile."

sleep 1
echo -e "#define MESSAGE \"MODIFIED\"\\n" > ../header.h;

$MEIQUE || fail "Basic compilation failed."

EXE=`./exe` || fail "Target not compiled!?"
if [ $EXE != "MODIFIED" ]
then
    fail "The target should be recompiled and relinked, but wasn't."
fi

