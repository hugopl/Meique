shared = Library:new("shared")
shared:addFile("sharedlib.cpp")

static1 = Library:new("static1", STATIC)
static1:use(shared)
static1:addFile("staticlib1.cpp")

static2 = Library:new("static2", STATIC)
static2:use(static1)
static2:addFile("staticlib2.cpp")

main = Executable:new("exec")
main:use(static2)
main:addFile("main.cpp")
