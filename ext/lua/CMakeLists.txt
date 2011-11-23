project(lua)
set(SRC
lapi.cpp
lauxlib.cpp
lbaselib.cpp
lcode.cpp
ldblib.cpp
ldebug.cpp
ldo.cpp
ldump.cpp
lfunc.cpp
lgc.cpp
linit.cpp
liolib.cpp
llex.cpp
lmathlib.cpp
lmem.cpp
loadlib.cpp
lobject.cpp
lopcodes.cpp
loslib.cpp
lparser.cpp
lstate.cpp
lstring.cpp
lstrlib.cpp
ltable.cpp
ltablib.cpp
ltm.cpp
lundump.cpp
lvm.cpp
lzio.cpp                                                                                                                                                                  )

add_library(lua STATIC ${SRC})

if (UNIX)
    add_definitions("-DLUA_USE_POSIX")
endif()
