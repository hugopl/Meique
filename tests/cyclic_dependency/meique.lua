function foo()
end

t1 = CustomTarget:new("t1", foo)
t2 = CustomTarget:new("t2", foo)
t3 = CustomTarget:new("t3", foo)
t4 = CustomTarget:new("t4", foo)
t5 = CustomTarget:new("t5", foo)
t6 = CustomTarget:new("t6", foo)

t1:addDependency(t2)
t1:addDependency(t3)

t2:addDependency(t4)

t3:addDependency(t5)

t5:addDependency(t6)

-- cyclic dependency!
t5:addDependency(t1)
