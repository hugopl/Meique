
function addMeiqueTest(testName)
    local testRunPath = meique:buildDir().."../tests/"
    local testPath = meique:sourceDir().."../tests/"..testName
    local testRunner = meique:sourceDir().."../tests/runtest.sh"
    local meiquePath = meique:buildDir()..meique:name()
    addTest(string.format("%s %s %s %s", testRunner, testPath, testRunPath, meiquePath))
end

local tests = [[
    twotargets_samename
]]

string.gsub(tests, '([^%s]+)', addMeiqueTest)
