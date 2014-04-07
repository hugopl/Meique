$MEIQUE ..

if [ $? -ne "1" ]
then
    fail "Cyclic dependency not detected";
fi
