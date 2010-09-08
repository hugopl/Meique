
set(meique_SRC
main.cpp
meiqueregex.cpp
meique.cpp
logger.cpp
meiquescript.cpp
config.cpp
luastate.cpp
stdstringsux.cpp
compilerfactory.cpp
compiler.cpp
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
oscommandjob.cpp
luajob.cpp
jobqueue.cpp
jobmanager.cpp
graph.cpp
luacpputil.cpp
lualocker.cpp
)

add_definitions("-Wall")

include_directories(${lua_SOURCE_DIR})
add_executable(meique ${meique_SRC})
target_link_libraries(meique lua -lpthread)
install(TARGETS meique DESTINATION bin)
