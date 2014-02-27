
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
]]

string.gsub(tests, '([^%s]+)', addMeiqueTest)
