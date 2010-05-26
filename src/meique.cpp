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

Meique::Meique(int argc, char** argv) : m_config(argc, argv), m_compiler(0), m_jobManager(new JobManager)
{
}

Meique::~Meique()
{
    delete m_compiler;
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
        Notice() << "Configuring project...";
        checkOptionsAgainstArguments(script.options());
        m_compiler = CompilerFactory::findCompiler();
        m_config.setCompiler(m_compiler->name());
    } else {
        m_compiler = CompilerFactory::createCompiler(m_config.compiler());
        std::string target = m_config.mainArgument();
        if (target == "clean") {
            TargetList list = script.targets();
            TargetList::iterator it = list.begin();
            for (; it != list.end(); ++it)
                (*it)->clean();
        } else {
            if (target.empty())
                target = "all";
            getTargetJobQueues(script.getTarget(target));
            m_jobManager->processJobs();
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

void Meique::getTargetJobQueues(Target* target)
{
    TargetList deps = target->dependencies();
    TargetList::iterator it = deps.begin();
    for (; it != deps.end(); ++it)
        getTargetJobQueues(*it);

    m_jobManager->addJobQueue(target->run(m_compiler));
}
