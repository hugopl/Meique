shared = Library:new("shared")
shared:addFile("sharedlib.cpp")

static = Library:new("static", STATIC)
static:use(shared)
static:addFile("staticlib.cpp")

main = Executable:new("exec")
main:use(static)
main:addFile("main.cpp")
