
function addMeiqueTest(testName)
    local testRunPath = meique:buildDir().."../tests/"
    local testPath = meique:sourceDir().."../tests/"..testName
    local testRunner = meique:sourceDir().."../tests/runtest.sh"
    local meiquePath = meique:buildDir()..meique:name()
    addTest(string.format("%s %s %s %s", testRunner, testPath, testRunPath, meiquePath), testName)
end

local tests = [[
    two_targets_samename
    two_targets_sharing_a_file
    static_linker_flags
    cyclic_dependency
    global_flags
    requires_meique
    basic_relink
    basic_header_dependence
    change_compiler_flags
    lua_lock
]]

string.gsub(tests, '([^%s]+)', addMeiqueTest)

function addMeiqueUnitTest(testName, deps)
    local tgt = Executable:new(testName)
    tgt:addFile(testName..".cpp")
    tgt:addIncludePath(meique:sourceDir())
    tgt:addFiles(deps)
    -- add basic files used by all tests
    tgt:addFiles("../src/logger.cpp ../src/stdstringsux.cpp")
    addTest(tgt)
end

local unitTests = {
    testnormalizepath = [[
        ../src/os_unix.cpp
    ]]
}

for test, deps in pairs(unitTests) do
    addMeiqueUnitTest(test, deps)
end
