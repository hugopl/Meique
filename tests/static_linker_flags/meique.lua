shared = Library:new("shared")
shared:addFile("sharedlib.cpp")

static = Library:new("static", STATIC)
static:useTarget(shared)
static:addFile("staticlib.cpp")

main = Executable:new("exec")
main:useTarget(static)
main:addFile("main.cpp")
