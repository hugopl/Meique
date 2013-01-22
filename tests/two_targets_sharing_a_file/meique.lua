
exe = Executable:new("exe")
exe:addFiles("main.cpp foo.cpp")
exe:addCustomFlags("-DTARGET_NAME='\"EXECUTABLE\"'")

lib = Library:new("lib")
lib:addFiles("lib.cpp foo.cpp")
lib:addCustomFlags("-DTARGET_NAME='\"LIBRARY\"'")

libUser = Executable:new("libUser")
libUser:useTarget(lib)
libUser:addFile("main.cpp")
