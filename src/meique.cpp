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
// TODO: Integrate the version number on build system
#define MEIQUE_VERSION "0.1"
#include "jobqueue.h"
#include "graph.h"
#include <vector>

Meique::Meique(int argc, char** argv) : m_config(argc, argv), m_jobManager(new JobManager)
{
    m_jobManager->setJobCountLimit(m_config.jobsAtOnce());
}

Meique::~Meique()
{
}

void Meique::exec()
{
    if (m_config.hasArgument("version")) {
        showVersion();
        return;
    } else if (m_config.isInConfigureMode()
               && m_config.mainArgument().empty()
               && m_config.hasArgument("help")) {
        showHelp();
        return;
    }

    MeiqueScript script(m_config);
    script.exec();
    if (m_config.hasArgument("help")) {
        showHelp(script.options());
    } else if (m_config.isInConfigureMode()) {
        Notice() << magenta() << "Configuring project...";
        checkOptionsAgainstArguments(script.options());
    } else {
        std::string target = m_config.mainArgument();
        if (target == "clean") {
            TargetList list = script.targets();
            TargetList::iterator it = list.begin();
            for (; it != list.end(); ++it)
                (*it)->clean();
        } else {
            if (target.empty())
                target = "all";
            executeJobQueues(script, target);
        }
    }
    m_config.saveCache();
}

void Meique::checkOptionsAgainstArguments(const OptionsMap& options)
{
    const StringMap args = m_config.arguments(); // Is std::map implicitly shared?
    // copy options to an strmap.
    StringMap userOptions;
    for (OptionsMap::const_iterator it = options.begin(); it != options.end(); ++it)
        userOptions[it->first] = it->second.defaultValue;

    for (StringMap::const_iterator it = args.begin(); it != args.end(); ++it) {
        if (options.find(it->first) == options.end())
            Error() << "The option \"" << it->first << "\" doesn't exists, use meique --help to see the available options.";
        userOptions[it->first] = it->second;
    }
    m_config.setUserOptions(userOptions);
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
            std::cout << "  --" << std::left;
            std::cout.width(32);
            std::cout << it->first;
            std::cout << it->second.description;
            if (!it->second.defaultValue.empty())
                std::cout << " [default value: " << it->second.defaultValue << ']';
            std::cout << std::endl;
        }
    }
    std::cout << "Build mode options:\n";
    std::cout << " -jN                                Allow N jobs at once.\n";
}

void Meique::executeJobQueues(const MeiqueScript& script, const std::string& targetName)
{
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
    std::list<int> myDeps = graph.topologicalSortDependencies(nodeMap[script.getTarget(targetName)]);
    std::vector<JobQueue*> queues(targets.size());
    for(std::list<int>::const_iterator it = myDeps.begin(); it != myDeps.end(); ++it) {
        JobQueue* queue = targets[*it]->run(m_config.compiler());
        queues[*it] = queue;
        m_jobManager->addJobQueue(queue);
    }

    // add dependency info to job queues
    for(std::list<int>::const_iterator it = myDeps.begin(); it != myDeps.end(); ++it) {
        TargetList deps = targets[*it]->dependencies();
        TargetList::const_iterator it2 = deps.begin();
        for (; it2 != deps.end(); ++it2)
            queues[*it]->addDependency(queues[nodeMap[*it2]]);
    }

    m_jobManager->processJobs();
}
