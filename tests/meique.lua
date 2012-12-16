
function addTest(testName)
    local testRunPath = meique:buildDir().."../tests/"
    local testPath = meique:sourceDir().."../tests/"..testName
    local testRunner = meique:sourceDir().."../tests/runtest.sh"
    local meiquePath = meique:buildDir()..meique:name()
    meique:addTest(string.format("%s %s %s %s", testRunner, testPath, testRunPath, meiquePath))
end

local tests = [[
    twotargets_samename
    two_targets_sharing_a_file
]]

string.gsub(tests, '([^%s]+)', addTest)
