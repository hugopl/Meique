$MEIQUE .. || fail "Early fail."

echo "exe:addCustomFlags('-DTHINGS_CHANGED')" >> ../meique.lua

$MEIQUE || fail "Early fail."

EXE=`./exe` || fail "Target not compiled."
if [ "$EXE" != "Changed" ]
then
    fail "main.cpp compilation not triggered after compiler command line modification."
fi
