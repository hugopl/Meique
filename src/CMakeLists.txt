
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
target.cpp
compilabletarget.cpp
librarytarget.cpp
customtarget.cpp
os_unix.cpp
filehash.cpp
)

add_definitions("-Wall")

include_directories(${LUA_INCLUDE_DIR})
add_executable(meique ${meique_SRC})
target_link_libraries(meique ${LUA_LIBRARIES})
install(TARGETS meique DESTINATION bin)