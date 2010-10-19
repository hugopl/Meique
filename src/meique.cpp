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
#include "jobmanager.h"
#include "jobqueue.h"
#include "graph.h"
#include "meiqueversion.h"
#include <vector>

Meique::Meique(int argc, char** argv) : m_config(argc, argv), m_jobManager(new JobManager)
{
    m_jobManager->setJobCountLimit(m_config.jobsAtOnce());
}

Meique::~Meique()
{
    delete m_jobManager;
}

void Meique::exec()
{
    if (m_config.action() == Config::ShowVersion) {
        showVersion();
        return;
    } else if (m_config.sourceRoot().empty()
               && m_config.action() == Config::ShowHelp) {
        showHelp();
        return;
    }

    if (m_config.isInConfigureMode() && m_config.action() != Config::ShowHelp)
        Notice() << magenta() << "Configuring project...";

    MeiqueScript script(m_config);
    script.exec();
    if (m_config.action() == Config::ShowHelp) {
        showHelp(script.options());
    } else if (m_config.isInBuildMode()) {
        // Get the list of targets
        StringList targetNames = m_config.targets();
        TargetList targetList;
        if (targetNames.empty()) {
            targetList = script.targets();
        } else {
            StringList::const_iterator it = targetNames.begin();
            for (; it != targetNames.end(); ++it)
                targetList.push_back(script.getTarget(*it));
        }

        switch(m_config.action()) {
            case Config::Clean:
                clean(targetList);
                break;
            case Config::Build:
                build(script, targetList);
                break;
            case Config::Test:
                test(targetList);
                break;
            case Config::Install:
            case Config::Uninstall:
            default:
                Error() << "Action not supported yet";
        }
    }

    if (m_config.action() != Config::ShowHelp)
        m_config.saveCache();
}

void Meique::showVersion()
{
    std::cout << "Meique version " MEIQUE_VERSION << std::endl;
    std::cout << "Copyright 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>\n";
}

void Meique::showHelp(const OptionsMap& options)
{
    std::cout << "Use meique OPTIONS TARGET\n\n";
    std::cout << "When in configure mode, TARGET is the directory of meique.lua file.\n";
    std::cout << "When in build mode, TARGET is the target name.\n\n";
    std::cout << "General options:\n";
    std::cout << " --help                             Print this message and exit.\n";
    std::cout << " --version                          Print the version number of meique and exit.\n";
    if (options.size()) {
        std::cout << "Config mode options for this project:\n";
        std::cout << " --debug                            Create a debug build.\n";
        std::cout << " --release                          Create a release build.\n";
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
    std::cout << "Build mode options:\n";
    std::cout << " -jN                                Allow N jobs at once.\n";
    std::cout << " -c [target [, target2 [, ...]]]    Clean a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -i [target [, target2 [, ...]]]    Install a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -u [target [, target2 [, ...]]]    Uninstall a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -t [target [, target2 [, ...]]]    Run tests for a specific target or all targets\n";
    std::cout << "                                    if none was specified.\n";
}

void Meique::executeJobQueues(const MeiqueScript& script, Target* mainTarget)
{
    if (mainTarget->wasRan())
        return;
    TargetList aux = script.targets();
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
        TargetList deps = target->dependencies();
        TargetList::const_iterator it = deps.begin();
        for (; it != deps.end(); ++it)
            graph.addEdge(i, nodeMap[*it]);
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
    for(std::list<int>::const_iterator it = myDeps.begin(); it != myDeps.end(); ++it) {
        Target* target = targets[*it];
        if (target->wasRan())
            continue;
        JobQueue* queue = target->run(m_config.compiler());
        queues[*it] = queue;
        m_jobManager->addJobQueue(queue);
    }

    // add dependency info to job queues
    for(std::list<int>::const_iterator it = myDeps.begin(); it != myDeps.end(); ++it) {
        Target* target = targets[*it];
        if (target->wasRan())
            continue;
        TargetList deps = target->dependencies();
        TargetList::const_iterator it2 = deps.begin();
        for (; it2 != deps.end(); ++it2)
            queues[*it]->addDependency(queues[nodeMap[*it2]]);
    }

    m_jobManager->processJobs();
}

void Meique::clean(const TargetList& targets)
{
    TargetList::const_iterator it = targets.begin();
    for (; it != targets.end(); ++it)
        (*it)->clean();
}

void Meique::test(const TargetList& targets)
{
    TargetList::const_iterator it = targets.begin();
    for (; it != targets.end(); ++it)
        (*it)->test();
}

void Meique::build(const MeiqueScript& script, const TargetList& targets)
{
    TargetList::const_iterator it = targets.begin();
    for (; it != targets.end(); ++it)
        executeJobQueues(script, *it);
}
