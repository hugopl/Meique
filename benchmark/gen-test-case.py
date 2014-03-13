import os
import sys
import shutil

NUM_FILES = 10
NUM_DEPS = 7
NUM_MODULES = 5

def genInitialModule():
    fp = open("main.c", "w")
    for i in range(NUM_MODULES):
        fp.write('#include "module%d/0.h"\n' % i)
    fp.write('\nint main() {\n')
    for i in range(NUM_MODULES):
        fp.write('    initModule%d(%d);\n' % (i, i))
    fp.write('    return 0;\n')
    fp.write('}\n')
    fp.close()

def genModule(moduleId, numModules):
    for i in range(NUM_FILES):
        h = open("%d.h" % i, "w")
        h.write("#ifndef MODULE%d_HEADER%d_H\n" % (moduleId,i))
        h.write("#define MODULE%d_HEADER%d_H\n" % (moduleId,i))
        if i == 0:
            h.write('__attribute__ ((visibility("default"))) int initModule%d(int);\n' % moduleId);
        h.write("int func%d(int a, int b);\n" % i)
        h.write("#endif\n")
        h.close()

        c = open("%d.c" % i, "w")
        for j in range(NUM_DEPS):
            dep = (j + i) % NUM_FILES
            c.write('#include "%d.h"\n' % dep)

        c.write("int func%d(int a, int b) {\n" % i)
        c.write("    return a + b")
        for j in range(NUM_DEPS):
            dep = (j + i) % NUM_FILES
            c.write(" + func%d(a, b)" % dep)
        c.write(";\n}\n")

        if i == 0:
            c.write("int initModule%d(int a) {\n" % moduleId)
            c.write("    return func0(a, a + 1);\n")
            c.write("}\n")
        c.close()

def createCMake(moduleId, numModules, filesPerModule):
    script = open("CMakeLists.txt", "w")
    if moduleId == -1:
        script.write("cmake_minimum_required(VERSION 2.8)\n")
        for i in range(numModules):
            script.write("add_subdirectory(module%i)\n" % i)

        script.write("include_directories(")
        for i in range(numModules):
            script.write(" module%d" % i)
        script.write(")\n");

        script.write("add_executable(exe main.c)\n")
        script.write("target_link_libraries(exe")
        for i in range(numModules):
            script.write(" module%d" % i)
        script.write(")\n");
    else:
        script.write("add_library(module%d SHARED" % moduleId)
        for i in range(filesPerModule):
            script.write(" %d.c\n" % i)
        script.write(")\n");
    script.close()

def createMeique(moduleId, numModules, filesPerModule):
    script = open("meique.lua", "w")
    if moduleId == -1:
        script.write('exe = Executable:new("exe")\n')
        script.write('exe:addFile("main.c")\n')
        for i in range(numModules):
            script.write('addSubdirectory("module%i")\n' % i)
            script.write("exe:use(module%d)\n" % i)
    else:
        script.write('module%d = Library:new("module%d")\n' % (moduleId, moduleId))
        script.write("module%d:addFiles([[\n" % moduleId)
        for i in range(filesPerModule):
            script.write("%d.c\n" % i)
        script.write("]])\n")
    script.close()

def main():
    builders = [createCMake, createMeique]

    genInitialModule()

    for i in range(NUM_MODULES):
        os.mkdir('module%d' % i)
        os.chdir('module%d' % i)
        genModule(i, NUM_MODULES)
        for builder in builders:
            builder(i, NUM_MODULES, NUM_FILES)
        os.chdir('..')

    for builder in builders:
        builder(-1, NUM_MODULES, NUM_FILES)

if __name__ == "__main__":
    nargs = len(sys.argv)
    if nargs >= 2:
        NUM_FILES = int(sys.argv[1])
    if nargs >= 3:
        NUM_DEPS = int(sys.argv[2])
    if nargs >= 4:
        NUM_MODULES = int(sys.argv[3])

    main()
