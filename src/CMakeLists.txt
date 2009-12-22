
find_package(Lua51 REQUIRED)

set(meique_SRC
main.cpp
meique.cpp
logger.cpp
meiquescript.cpp
config.cpp
luastate.cpp
cppstringssux.cpp
compilerfactory.cpp
gcc.cpp
)

include_directories(${LUA_INCLUDE_DIR})
add_executable(meique ${meique_SRC})
target_link_libraries(meique ${LUA_LIBRARIES})
