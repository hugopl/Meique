$MEIQUE .. || fail "Basic compilation failed."

sleep 1
echo -e "#include <iostream>\\n"\
"int main() { std::cout << \"MODIFIED\"; }" > ../main.cpp;

$MEIQUE || fail "Basic compilation failed."

EXE=`./exe` || fail "Target not compiled!?"
if [ $EXE != "MODIFIED" ]
then
    fail "The target should be relinked, but wasn't."
fi
