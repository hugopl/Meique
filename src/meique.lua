
meique = Executable:new("meique")
GCC:addCustomFlags("-std=c++0x")
meique:use(lua)
meique:addLinkLibraries("pthread")
meique:addFiles([[
main.cpp
meiqueregex.cpp
meique.cpp
nodetree.cpp
nodevisitor.cpp
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
jobfactory.cpp
jobmanager.cpp
oscommandjob.cpp
luajob.cpp
graph.cpp
luacpputil.cpp
lualocker.cpp
]])

meique:addFiles(meique:buildDir().."meiqueapi.cpp")

meiqueApi = CustomTarget:new("meiqueapi", function(modifiedFiles)
    local out = meique:buildDir().."meiqueapi.cpp"
    local file2cBin = file2c:buildDir().."file2c"
    local res = os.execute(file2cBin.." meiqueApi "..modifiedFiles[0].." > "..out)
    abortIf(res ~= 0, "Error creating "..out..".")
end)
meiqueApi:addFile("meiqueapi.lua")
meiqueApi:addOutput("meiqueapi.cpp")

meique:addDependency(meiqueApi)
meiqueApi:addDependency(file2c)
UNIX:meique:install("bin")
