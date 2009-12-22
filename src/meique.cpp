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

Meique::Meique(int argc, char** argv) : m_config(argc, argv), m_compiler(0)
{
}

Meique::~Meique()
{
    delete m_compiler;
}

void Meique::exec()
{
    MeiqueScript script(m_config);
    script.exec();
    if (m_config.isInConfigureMode()) {
        Notice() << "Configuring project...";
        checkOptionsAgainstArguments(script.options());
        m_compiler = CompilerFactory::findCompiler();
        m_config.setCompiler(m_compiler->name());
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


