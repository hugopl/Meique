for i = 1, 100 do
    local exe = CustomTarget:new("target_"..i, function()
        -- eat some CPU
        local k = "."
        for j = 1,10 do
            k = k..k
        end
    end)
end