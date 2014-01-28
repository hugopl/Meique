$MEIQUE .. || fail "Failed to build"

EXE=`./exe` || fail "Target not compiled!?"
if [ $EXE != "Pass" ]
then
    fail "Global addCustomFlags wasn't used on this target!"
fi
