meique = Executable:new("meique")
meique:usePackage(lua)
meique:addFiles([[
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
librarytarget.cpp
customtarget.cpp
maintarget.cpp
os_unix.cpp
filehash.cpp
job.cpp
jobqueue.cpp
jobmanager.cpp
]])
