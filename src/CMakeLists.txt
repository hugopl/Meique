
set(meiqueVersion "0.9")
configure_file(meiqueversion.h.in meiqueversion.h @ONLY)

add_custom_command(
    OUTPUT meiqueapi.cpp
    COMMAND
    ${file2c_BINARY_DIR}/file2c meiqueApi ${CMAKE_CURRENT_SOURCE_DIR}/meiqueapi.lua > meiqueapi.cpp
)

set(meique_SRC
${CMAKE_CURRENT_BINARY_DIR}/meiqueapi.cpp
main.cpp
meiqueregex.cpp
meique.cpp
cmdline.cpp
logger.cpp
hashgroups.cpp
meiquescript.cpp
meiquecache.cpp
luastate.cpp
stdstringsux.cpp
compilerfactory.cpp
compiler.cpp
gcc.cpp
compileroptions.cpp
linkeroptions.cpp
target.cpp
compilabletarget.cpp
executabletarget.cpp
librarytarget.cpp
customtarget.cpp
os_unix.cpp
filehash.cpp
job.cpp
oscommandjob.cpp
luajob.cpp
jobqueue.cpp
jobmanager.cpp
graph.cpp
luacpputil.cpp
lualocker.cpp
)

add_definitions("-Wall")

include_directories(${lua_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_executable(meique ${meique_SRC})
target_link_libraries(meique lua -lpthread)
install(TARGETS meique DESTINATION bin)
