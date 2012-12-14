/*
    This file is part of the Meique project
    Copyright (C) 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "meique.h"
#include "logger.h"
#include "meiquescript.h"
#include "compiler.h"
#include "compilerfactory.h"
#include "target.h"
#include "compilabletarget.h"
#include "jobmanager.h"
#include "jobqueue.h"
#include "graph.h"
#include "meiqueversion.h"
#include <vector>
#include <sstream>
#include "statemachine.h"
#include "meiquecache.h"

#define MEIQUECACHE "meiquecache.lua"

enum {
    HasVersionArg = 1,
    HasHelpArg,
    NormalArgs,
    DumpProject,
    Found,
    NotFound,
    Yes,
    No,
    TestAction,
    InstallAction,
    UninstallAction,
    BuildAction,
    CleanAction
};

Meique::Meique(int argc, const char** argv) : m_args(argc, argv), m_jobManager(new JobManager), m_script(0), m_firstRun(false)
{
    m_jobManager->setJobCountLimit(m_args.intArg("j", 1));
}

Meique::~Meique()
{
    delete m_script;
    delete m_jobManager;
}

int Meique::checkArgs()
{
    std::string verboseValue = OS::getEnv("VERBOSE");
    std::istringstream s(verboseValue);
    s >> ::verbosityLevel;

    if (m_args.boolArg("d"))
        ::coloredOutputEnabled = false;
    if (m_args.boolArg("version"))
        return HasVersionArg;
    if (m_args.boolArg("help"))
        return HasHelpArg;
    if (m_args.boolArg("meique-dump-project"))
        return DumpProject;
    return NormalArgs;
}

int Meique::lookForMeiqueCache()
{
    std::ifstream file(MEIQUECACHE);
    return file ? Found : NotFound;
}

int Meique::lookForMeiqueLua()
{
    if (m_args.numberOfFreeArgs() != 1)
        return NotFound;

    std::string path = m_args.freeArg(0);
    return OS::fileExists(path + "/meique.lua") ? Found : NotFound;
}

int Meique::configureProject()
{
    std::string meiqueLuaPath = OS::normalizeDirPath(m_args.freeArg(0));
    m_script = new MeiqueScript(meiqueLuaPath + "/meique.lua", &m_args);
    m_script->setSourceDir(meiqueLuaPath);
    m_script->setBuildDir(OS::pwd());
    m_firstRun = true;

    try {
        m_script->exec();
    } catch (MeiqueError& e) {
        m_script->cache()->setAutoSave(false);
        throw;
    }
    return 0;
}

int Meique::dumpProject()
{
    std::ifstream file(MEIQUECACHE);
    if (!file)
        Error() << MEIQUECACHE << " not found.";
    file.close();

    m_script = new MeiqueScript;
    m_script->setBuildDir("./");
    m_script->exec();

    std::cout << "Project: " << OS::baseName(m_script->sourceDir()) << std::endl;

    for (Target* target : m_script->targets()) {
        if (!target->isCompilableTarget())
            continue;
        CompilableTarget* ctarget = static_cast<CompilableTarget*>(target);

        std::cout << "Target: " << ctarget->name() << std::endl;
        for (const std::string& fileName : target->files()) {
            if (fileName.empty())
                continue;
            std::string absPath = fileName[0] == '/' ? fileName : m_script->sourceDir() + target->directory() + fileName;
            std::cout << "File: " << OS::normalizeFilePath(absPath) << std::endl;
            // TODO: Defines
        }

        std::cout << "Include: " << OS::normalizeDirPath(m_script->sourceDir() + target->directory()) << std::endl;
        for (const std::string& inc : ctarget->includeDirectories())
            std::cout << "Include: " << OS::normalizeDirPath(inc) << std::endl;
    }
    return 0;
}

int Meique::getBuildAction()
{
    if (!m_script) {
        m_script = new MeiqueScript;
        m_script->setBuildDir(OS::pwd());
        m_script->exec();
    }

    if (m_args.boolArg("c"))
        return CleanAction;
    else if (m_args.boolArg("i"))
        return InstallAction;
    else if (m_args.boolArg("t"))
        return TestAction;
    else if (m_args.boolArg("u"))
        return UninstallAction;
    else
        return BuildAction;
}

TargetList Meique::getChosenTargets()
{
    int ntargets = m_args.numberOfFreeArgs();
    StringList targetNames;
    for (int i = m_firstRun ? 1 : 0; i < ntargets; ++i)
        targetNames.push_back(m_args.freeArg(i));

    TargetList targets;
    if (targetNames.empty()) {
        targets = m_script->targets();
    } else {
        for (const std::string& targetName : targetNames)
            targets.push_back(m_script->getTarget(targetName));
    }

    if (targets.empty())
        Error() << "There's no targets!";

    return targets;
}

int Meique::buildTargets()
{
    for (Target* target : getChosenTargets())
        createJobQueues(m_script, target);

    m_jobManager->processJobs();
    return 0;
}

int Meique::cleanTargets()
{
    for (Target* target : getChosenTargets())
        target->clean();
    return 0;
}

int Meique::installTargets()
{
    for (Target* target : getChosenTargets())
        target->install();
    return 0;
}

int Meique::testTargets()
{
    bool thereIsATest = false;
    for (Target* target : getChosenTargets())
        thereIsATest |= target->test();

    if (!thereIsATest)
        Notice() << "No tests to run :-(";
    return 0;
}

int Meique::uninstallTargets()
{
    return 0;
}

void Meique::exec()
{
    StateMachine<Meique> machine(this);

    machine[STATE(Meique::checkArgs)][HasHelpArg] = STATE(Meique::showHelp);
    machine[STATE(Meique::checkArgs)][HasVersionArg] = STATE(Meique::showVersion);
    machine[STATE(Meique::checkArgs)][NormalArgs] = STATE(Meique::lookForMeiqueCache);
    machine[STATE(Meique::checkArgs)][DumpProject] = STATE(Meique::dumpProject);

    machine[STATE(Meique::lookForMeiqueCache)][Found] = STATE(Meique::getBuildAction);
    machine[STATE(Meique::lookForMeiqueCache)][NotFound] = STATE(Meique::lookForMeiqueLua);

    machine[STATE(Meique::lookForMeiqueLua)][Found] = STATE(Meique::configureProject);
    machine[STATE(Meique::lookForMeiqueLua)][NotFound] = STATE(Meique::showHelp);

    machine[STATE(Meique::configureProject)][0] = STATE(Meique::getBuildAction);

    machine[STATE(Meique::getBuildAction)][TestAction] = STATE(Meique::testTargets);
    machine[STATE(Meique::getBuildAction)][InstallAction] = STATE(Meique::installTargets);
    machine[STATE(Meique::getBuildAction)][UninstallAction] = STATE(Meique::uninstallTargets);
    machine[STATE(Meique::getBuildAction)][BuildAction] = STATE(Meique::buildTargets);
    machine[STATE(Meique::getBuildAction)][CleanAction] = STATE(Meique::cleanTargets);

    machine.execute(STATE(Meique::checkArgs));
}

int Meique::showVersion()
{
    std::cout << "Meique version " MEIQUE_VERSION << std::endl;
    std::cout << "Copyright 2009-2011 Hugo Parente Lima <hugo.pl@gmail.com>\n";
    return 0;
}

int Meique::showHelp()
{
    std::cout << "Usage: meique OPTIONS TARGET\n\n";
    std::cout << "When in configure mode, TARGET is the directory of meique.lua file.\n";
    std::cout << "When in build mode, TARGET is the target name.\n\n";
    std::cout << "General options:\n";
    std::cout << " --help                             Print this message and exit.\n";
    std::cout << " --version                          Print the version number of meique and exit.\n";
    std::cout << "Config mode options for this project:\n";
    std::cout << " --debug                            Create a debug build.\n";
    std::cout << " --release                          Create a release build.\n";
    std::cout << " --install-prefix                   Install directory used by install, this directory\n";
    std::cout << "                                    is prepended onto all install directories.\n";

/*
    if (options.size()) {
        OptionsMap::const_iterator it = options.begin();
        for (; it != options.end(); ++it) {
            std::cout << " --" << std::left;
            std::cout.width(33);
            std::cout << it->first;
            std::cout << it->second.description;
            if (!it->second.defaultValue.empty())
                std::cout << " [default value: " << it->second.defaultValue << ']';
            std::cout << std::endl;
        }
    }
*/
    std::cout << "Build mode options:\n";
    std::cout << " -jN                                Allow N jobs at once.\n";
    std::cout << " -d                                 Disable colored output\n";
    std::cout << " -c [target [, target2 [, ...]]]    Clean a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -i [target [, target2 [, ...]]]    Install a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -u [target [, target2 [, ...]]]    Uninstall a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -t [target [, target2 [, ...]]]    Run tests for a specific target or all targets\n";
    std::cout << "                                    if none was specified.\n";
    return 0;
}

void Meique::createJobQueues(const MeiqueScript* script, Target* mainTarget)
{
    if (mainTarget->wasRan())
        return;

    TargetList aux = script->targets();
    std::vector<Target*> targets;
    std::copy(aux.begin(), aux.end(), std::back_inserter(targets));

    // Create and fill some collections to help with the graph manipulation
    std::map<Target*, int> nodeMap;
    std::map<int, std::string> revMap;
    for (size_t i = 0; i < targets.size(); ++i) {
        Target* target = targets[i];
        nodeMap[target] = i;
        revMap[i] = target->name();
    }

    // Create the dependency graph
    Graph graph(targets.size());
    for (size_t i = 0; i < targets.size(); ++i) {
        Target* target = targets[i];
        for (Target* dep : target->dependencies())
            graph.addEdge(i, nodeMap[dep]);
    }

    // Try to find cyclic dependences
    std::list<int> sortedNodes = graph.topologicalSort();
    if (sortedNodes.empty()) {
        graph.dumpDot(revMap, "cyclicDeps.dot");
        Error() << "Cyclic dependency found in your targets! You can check the dot graph at ./cyclicDeps.dot.";
    }

    // get all job queues
    std::list<int> myDeps = graph.topologicalSortDependencies(nodeMap[mainTarget]);
    std::vector<JobQueue*> queues(targets.size());
    for(int depIdx : myDeps) {
        Target* target = targets[depIdx];
        if (target->wasRan())
            continue;
        JobQueue* queue = target->run(m_script->cache()->compiler());
        queues[depIdx] = queue;
        m_jobManager->addJobQueue(queue);
    }

    // add dependency info to job queues
    for(int depIdx : myDeps) {
        Target* target = targets[depIdx];
        if (target->wasRan())
            continue;
        for (Target* dep : target->dependencies())
            queues[depIdx]->addDependency(queues[nodeMap[dep]]);
    }
}
