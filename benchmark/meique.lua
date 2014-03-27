benchmark = CustomTarget:new("benchmark", function()
    for i = 1, 4 do
        local n = 10^i
        os.execute("./run-test.sh "..n.." \""..benchmark:buildDir().."\"")
    end
    os.execute("ruby plot.rb \""..benchmark:buildDir().."\" > "..benchmark:buildDir().."result.html")
    print("All done, check "..benchmark:buildDir().."result.html")
end)

benchmark:excludeFromAll()
