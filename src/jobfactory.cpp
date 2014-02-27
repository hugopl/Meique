/*
    This file is part of the Meique project
    Copyright (C) 2014 Hugo Parente Lima <hugo.pl@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "jobfactory.h"

#include "compileroptions.h"
#include "stdstringsux.h"
#include "linkeroptions.h"
#include "meiquecache.h"
#include "meiquescript.h"
#include "nodetree.h"
#include "nodevisitor.h"
#include "oscommandjob.h"
#include "logger.h"
#include "luacpputil.h"
#include "lualocker.h"
#include "luajob.h"
#include <cassert>

JobFactory::JobFactory(MeiqueScript& script, NodeTree& nodeTree)
    : m_script(script)
    , m_nodeTree(nodeTree)
    , m_root(nullptr)
    , m_needToWait(false)
    , m_processedNodes(0)
{
    m_nodeTree.onTreeChange = [&]() {
        m_treeChanged.notify_all();
    };
}

JobFactory::~JobFactory()
{
    for (auto i : m_targetCompilerOptions)
        delete i.second;
}

void JobFactory::setRoot(Node* root)
{
    m_root = root;
    NodeVisitor<>(m_root, [&](Node* node){
        cacheTargetCompilerOptions(node);
        mergeCompilerAndLinkerOptions(node);
    });

#if 0
    // preTargetCompileHooks must be run before the target node expansion because they may create more nodes.
    // execute pre-compile hook functions
    getLuaField("_preTargetCompileHooks");
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        // push the target to the stack, it'll be the arg.
        lua_pushlightuserdata(L, (void *)this);
        lua_gettable(L, LUA_REGISTRYINDEX);
        luaPCall(L, 1, 0, "[preTargetCompileHook]");
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1); // remove the table
#endif
    m_nodeTree.expandTargetNode(m_root);
    m_nodeTree.dump("/tmp/tree.dot");
}

Job* JobFactory::createJob()
{
    assert(m_root);

    Job* job = nullptr;
    Node* target;
    Node* node;

    do {
        m_needToWait = false;
        {
            std::lock_guard<NodeTree> nodeTreeLock(m_nodeTree);
            node = findAGoodNode(&target, m_root);
        }
        if (!node) {
            if (m_needToWait) {
                std::unique_lock<std::mutex> lock(m_treeChangedMutex);
                m_treeChanged.wait(lock);
                continue;
            }
            break;
        }

        node->status = Node::Building;
        m_processedNodes++;

        LuaLocker luaLock(m_script.luaState());
        std::lock_guard<NodeTree> nodeTreeLock(m_nodeTree);

        if (!node->isTarget)
            job = createCompilationJob(target, node);
        else if (node->isCustomTarget())
            job = createCustomTargetJob(target);
        else
            job = createTargetJob(target);
    } while(!job);

    if (job) {
        std::lock_guard<NodeTree> nodeTreeLock(m_nodeTree);
        NodeVisitor<NodeGetParent>(node, [node](Node* parent) {
            if (node != parent)
                parent->shouldBuild = 1;
        });
    }

    return job;
}

Node* JobFactory::findAGoodNode(Node** target, Node* node)
{
    if (node->status >= Node::Building)
        return nullptr;

    if (node->isTarget)
        *target = node;

    if (node->children.empty()) {
        return node;
    }

    bool hasChildrenBuilding = false;
    bool hasDependenceBuilding = false;
    for (Node* child : node->children) {
        hasChildrenBuilding |= child->status < Node::Built;

        // Don't build files from this target is some dependence still building
        hasDependenceBuilding |= child->isTarget && child->status < Node::Built;
        if (hasDependenceBuilding && !child->isTarget)
            continue;

        if (child->status < Node::Building) {
            Node* nodeFound = findAGoodNode(target, child);
            if (nodeFound)
                return nodeFound;
        }
    }
    if (node->isFake)
        return nullptr;

    if (hasChildrenBuilding) {
        m_needToWait = true;
        return nullptr;
    }
    return node;
}

Job* JobFactory::createCompilationJob(Node* target, Node* node)
{
    node->status = Node::Building;

    Options* options = m_targetCompilerOptions[target];
    const std::string sourceDir = m_script.sourceDir() + options->targetDirectory;
    const std::string buildDir = m_script.buildDir() + options->targetDirectory;
    const std::string& fileName = node->name;

    const std::string absSourcePath = fileName.at(0) == '/' ? fileName : sourceDir + fileName;
    std::string absObjPath = m_script.cache()->compiler()->nameForObject(node->name, target->name);
    if (absObjPath.at(0) != '/')
        absObjPath.insert(0, buildDir);

    std::string source = OS::normalizeFilePath(absSourcePath);
    std::string output = OS::normalizeFilePath(absObjPath);

    m_depChecker.setWorkingDirectory(sourceDir);
    if (!m_depChecker.shouldCompile(source, output)) {
        node->status = Node::Built;
        return nullptr;
    }

    Compiler* compiler = m_script.cache()->compiler();
    OSCommandJob* job = new OSCommandJob(m_nodeTree.createNodeGuard(node), compiler->compile(source, output, &options->compilerOptions));
    job->setWorkingDirectory(buildDir);
    job->setName("Compiling " + OS::baseName(fileName));

    return job;
}

Job* JobFactory::createTargetJob(Node* target)
{
    lua_State* L = m_script.luaState();
    LuaLeakCheck(L);

    target->status = Node::Building;

    Compiler* compiler = m_script.cache()->compiler();
    Options* options = m_targetCompilerOptions[target];
    const std::string buildDir = m_script.buildDir() + options->targetDirectory;

    StringList objects;
    for (Node* child : target->children) {
        if (!child->isTarget)
        objects.push_back(compiler->nameForObject(child->name, target->name));
    }

    m_script.luaPushTarget(target->name);
    lua_getfield(L, -1, "_output");
    std::string outputName = lua_tocpp<std::string>(L, -1);
    lua_pop(L, 2);

    // Check if the target must be build
    if (!target->shouldBuild && OS::fileExists(buildDir + outputName)) {
        target->status = Node::Built;
        return nullptr;
    }

    OSCommandJob* job = new OSCommandJob(m_nodeTree.createNodeGuard(target), compiler->link(outputName, objects, &options->linkerOptions));
    job->setWorkingDirectory(buildDir);
    job->setName("Linking " + outputName);

    return job;
}

Job* JobFactory::createCustomTargetJob(Node* target)
{
    lua_State* L = m_script.luaState();
    LuaLeakCheck(L);

    target->status = Node::Building;

    m_script.luaPushTarget(target->name);
    LuaAutoPop autoPop(L);

    StringList files;
    lua_getfield(L, -1, "_files");
    readLuaList(L, lua_gettop(L), files);
    lua_pop(L, 1);

    Options* options = m_targetCompilerOptions[target];

    if (!target->shouldBuild) {
        StringList outputs;
        lua_getfield(L, -1, "_outputs");
        readLuaList(L, lua_gettop(L), outputs);
        lua_pop(L, 1);

        const std::string buildDir = m_script.buildDir() + options->targetDirectory;
        const std::string sourceDir = m_script.sourceDir() + options->targetDirectory;

        if (!outputs.empty()) {
            bool shouldRun = false;
            for (const std::string& file : files) {
                for (const std::string& output : outputs) {
                    if (OS::timestampCompare(sourceDir + file, buildDir + output) < 0) {
                        shouldRun = true;
                        break;
                    }
                }
                if (shouldRun)
                    break;
            }
            if (!shouldRun) {
                target->status = Node::Built;
                return nullptr;
            }
        }
    }

    lua_getfield(L, -1, "_func");
    createLuaArray(L, files);

    LuaJob* job = new LuaJob(m_nodeTree.createNodeGuard(target), L, 1);
    job->setName("Running custom target " + std::string(target->name));
    job->setWorkingDirectory(m_script.sourceDir() + options->targetDirectory);

    return job;
}

void JobFactory::cacheTargetCompilerOptions(Node* node)
{
    if (!node->isTarget || node->hasCachedCompilerFlags)
        return;

    node->hasCachedCompilerFlags = true;

    Options* options = new Options;
    m_targetCompilerOptions[node] = options;
    fillTargetOptions(node, options);
    OS::mkdir(m_script.buildDir() + options->targetDirectory);
}

static void readMeiquePackageOnStack(lua_State* L, CompilerOptions& compilerOptions, LinkerOptions& linkerOptions)
{
    StringMap map;
    readLuaTable(L, lua_gettop(L), map);

    compilerOptions.addIncludePaths(split(map["includePaths"]));
    compilerOptions.addCustomFlag(map["cflags"]);
    linkerOptions.addCustomFlag(map["linkerFlags"]);
    linkerOptions.addLibraryPaths(split(map["libraryPaths"]));
    linkerOptions.addLibraries(split(map["linkLibraries"]));
}

void JobFactory::fillTargetOptions(Node* node, Options* options)
{
    assert(node->isTarget);

    lua_State* L = m_script.luaState();
    LuaLeakCheck(L);

    m_script.luaPushTarget(node->name);

    lua_getfield(L, -1, "_dir");
    options->targetDirectory = lua_tocpp<std::string>(L, -1);
    const std::string& targetDirectory = options->targetDirectory;
    lua_pop(L, 1);

    if (node->isCustomTarget()) {
        lua_pop(L, 1);
        return;
    }

    CompilerOptions& compilerOptions = options->compilerOptions;
    LinkerOptions& linkerOptions = options->linkerOptions;

    // TODO: Detect language based on Node children.
    linkerOptions.setLanguage(CPlusPlusLanguage);

    // Add source dir in the include path
    const std::string sourcePath = m_script.sourceDir() + targetDirectory;
    compilerOptions.addIncludePath(sourcePath);
    // Add info from global package
    lua_getglobal(L, "_meiqueGlobalPackage");
    readMeiquePackageOnStack(L, compilerOptions, linkerOptions);
    lua_pop(L, 1);

    // Get the package info
    lua_getfield(L, -1, "_packages");
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        if (lua_istable(L, -1))
            readMeiquePackageOnStack(L, compilerOptions, linkerOptions);
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1);


    if (m_script.cache()->buildType() == MeiqueCache::Debug)
        compilerOptions.enableDebugInfo();
    else
        compilerOptions.addDefine("NDEBUG");

    StringList list;
    // explicit include directories
    lua_getfield(L, -1, "_incDirs");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    for (std::string& item : list) {
        if (!item.empty() && item[0] != '/')
            item.insert(0, sourcePath);
    }
    compilerOptions.addIncludePaths(list);
    list.clear();

    // explicit link libraries
    lua_getfield(L, -1, "_linkLibraries");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    linkerOptions.addLibraries(list);
    list.clear();

    // explicit library include dirs
    lua_getfield(L, -1, "_libDirs");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    linkerOptions.addLibraryPaths(list);
    list.clear();

    // Add build dir in the include path
    compilerOptions.addIncludePath(m_script.buildDir() + targetDirectory);
    compilerOptions.normalize();

    if (node->isLibraryTarget()) {
        compilerOptions.setCompileForLibrary(true);
        lua_getfield(L, -1, "_libType");
        int libType = lua_tocpp<int>(L, -1);
        lua_pop(L, 1);
        LinkerOptions::LinkType linkType;
        switch (libType) {
            case 1:
                linkType = LinkerOptions::SharedLibrary;
                break;
            case 2:
                linkType = LinkerOptions::StaticLibrary;
                break;
            default:
                throw Error("Unknown library type! " + libType);
        }
        linkerOptions.setLinkType(linkType);
    } else {
        linkerOptions.setLinkType(LinkerOptions::Executable);
    }

    lua_pop(L, 1);
}

void JobFactory::mergeCompilerAndLinkerOptions(Node* node)
{
    if (!node->isTarget || node->isCustomTarget())
        return;

    assert(node->hasCachedCompilerFlags);

    lua_State* L = m_script.luaState();
    LuaLeakCheck(L);

    m_script.luaPushTarget(node->name);
    // other targets
    StringList targets;
    lua_getfield(L, -1, "_targets");
    readLuaList(L, lua_gettop(L), targets);
    lua_pop(L, 2);
    for (const std::string& usedTarget : targets) {
        Node* dependence = m_nodeTree.getTargetNode(usedTarget);

        Options* sourceOptions = m_targetCompilerOptions[node];
        Options* depOptions = m_targetCompilerOptions[dependence];
        sourceOptions->compilerOptions.merge(depOptions->compilerOptions);
        sourceOptions->linkerOptions.merge(depOptions->linkerOptions);

        if (dependence->isLibraryTarget()) {
            if (depOptions->linkerOptions.linkType() == LinkerOptions::SharedLibrary) {
                sourceOptions->linkerOptions.addLibraryPath(m_script.buildDir() + depOptions->targetDirectory);
                sourceOptions->linkerOptions.addLibrary(dependence->name);
            } else {
                std::string libName = m_script.cache()->compiler()->nameForStaticLibrary(dependence->name);
                sourceOptions->linkerOptions.addStaticLibrary(m_script.buildDir() + depOptions->targetDirectory + libName);
            }
        }
    }
}

unsigned JobFactory::nodeCount() const
{
    return m_nodeTree.size();
}
