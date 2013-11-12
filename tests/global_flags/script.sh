$MEIQUE .. || fail "Failed to build"

if [ `./exe` != "Pass" ]
then
    fail "Global addCustomFlags wasn't used on this target!"
fi
