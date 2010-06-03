
find_package(Lua51 REQUIRED)

set(meique_SRC
main.cpp
meique.cpp
logger.cpp
meiquescript.cpp
config.cpp
luastate.cpp
stdstringsux.cpp
compilerfactory.cpp
gcc.cpp
compileroptions.cpp
linkeroptions.cpp
target.cpp
luatarget.cpp
compilabletarget.cpp
executabletarget.cpp
librarytarget.cpp
customtarget.cpp
maintarget.cpp
os_unix.cpp
filehash.cpp
job.cpp
jobqueue.cpp
jobmanager.cpp
)

add_definitions("-Wall")

include_directories(${LUA_INCLUDE_DIR})
add_executable(meique ${meique_SRC})
target_link_libraries(meique ${LUA_LIBRARIES} -lpthread)
install(TARGETS meique DESTINATION bin)