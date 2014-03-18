-- Build meique as a static lib, so we can use this lib on unit tests.
meiqueLib = Library:new("meiquelib", STATIC)
GCC:addCustomFlags("-std=c++0x")
meiqueLib:use(lua)
meiqueLib:addLinkLibraries("pthread")
meiqueLib:addFiles([[
meiqueregex.cpp
meique.cpp
nodetree.cpp
nodevisitor.cpp
cmdline.cpp
logger.cpp
meiquescript.cpp
meiquecache.cpp
stdstringsux.cpp
compilerfactory.cpp
compiler.cpp
gcc.cpp
compileroptions.cpp
linkeroptions.cpp
os_unix.cpp
job.cpp
jobfactory.cpp
jobmanager.cpp
oscommandjob.cpp
luajob.cpp
luacpputil.cpp
]])

meiqueLib:addFiles(meiqueLib:buildDir().."meiqueapi.cpp")

meiqueApi = CustomTarget:new("meiqueapi", function(modifiedFiles)
    local out = meiqueLib:buildDir().."meiqueapi.cpp"
    local file2cBin = file2c:buildDir().."file2c"
    local res = os.execute(file2cBin.." meiqueApi "..modifiedFiles[0].." > "..out)
    abortIf(res ~= 0, "Error creating "..out..".")
end)
meiqueApi:addFile("meiqueapi.lua")
meiqueApi:addOutput("meiqueapi.cpp")

meiqueLib:addDependency(meiqueApi)
meiqueApi:addDependency(file2c)

meique = Executable:new("meique")
meique:addFile("main.cpp")
meique:use(meiqueLib)
UNIX:meique:install("bin")
