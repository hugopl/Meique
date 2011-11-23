
meiqueVersion = "0.9"
configureFile("meiqueversion.h.in", "meiqueversion.h")

meique = Executable:new("meique")
meique:useTarget(lua)
meique:addLinkLibraries("pthread")
meique:addFiles([[
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
]])

meique:addFiles(meique:buildDir().."meiqueapi.cpp")

meiqueApi = CustomTarget:new("meiqueapi", function()
    local luaApi = meique:sourceDir().."meiqueapi.lua"
    local out = meique:buildDir().."meiqueapi.cpp"
    local res = os.execute("echo \"extern const char meiqueApi[] = {\" > "..out.." &&"
                        .."hexdump -v -e '16/1 \" 0x0%02x,\" \"\\n\"' "..luaApi.." >> "..out.." &&"
                        .."echo \"};\" >> "..out)
    abortIf(res ~= 0, "Error creating "..out..".")
end)
meiqueApi:addFiles("meiqueapi.lua")
meiqueApi:addFiles(meique:buildDir().."meiqueapi.cpp")
meique:addDependency(meiqueApi)
UNIX:meique:install("bin")
