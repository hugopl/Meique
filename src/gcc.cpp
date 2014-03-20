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

#include "gcc.h"
#include "compileroptions.h"
#include "linkeroptions.h"
#include "logger.h"
#include "stdstringsux.h"
#include <algorithm>
#include <fstream>

static bool isAvailable(std::string& fullName)
{
    int res = OS::exec("g++ --version", &fullName);
    if (!res) {
        size_t it = fullName.find('\n');
        fullName.erase(it);
        return true;
    }
    return false;
}

static Compiler* create()
{
    return new Gcc;
}

CompilerFactory Gcc::factory()
{
    CompilerFactory f;
    f.id = "Gcc";
    f.probe = isAvailable;
    f.create = create;
    return std::move(f);
}

bool Gcc::shouldCompile(const std::string& source, const std::string& output) const
{
    if (OS::timestampCompare(source, output) < 0)
        return true;

    std::ifstream f((output + ".d").c_str());
    if (!f)
        return true;

    std::string line;
    std::getline(f, line);
    stringReplace(line, output + ": ", "");

    do {
        if (line.length() > 2) {
            if (*line.rbegin() == '\\')
            line.erase(line.length()-2);

            StringList deps = split(line, ' ');
            for (const std::string& dep : deps) {
                if (OS::timestampCompare(dep, output) < 0)
                    return true;
            }
        }
        std::getline(f, line);
        trim(line);
    } while (f);
    return false;
}

std::string Gcc::compile(const std::string& fileName, const std::string& output, const CompilerOptions* options)
{
    CompilerCommandCache::const_iterator it = m_compileCommandCache.find(options);
    if (it != m_compileCommandCache.end()) {
        std::string cachedCmd = it->second;
        cachedCmd += " -MMD -MF ";
        cachedCmd += output + ".d";
        cachedCmd += " -c ";
        cachedCmd += fileName;
        cachedCmd += " -o ";
        cachedCmd += output;
        return std::move(cachedCmd);
    }

    std::string command;
    Language lang = identifyLanguage(fileName);
    if (lang == CLanguage)
        command += "gcc";
    else if (lang == CPlusPlusLanguage)
        command += "g++";
    else
        throw Error("Unknown programming language used for " + fileName);

    // custom flags
    command += " " + join(options->customFlags(), " ");

    // include paths
    for (const std::string& path : options->includePaths())
        command += " -I\"" + path + '"';

    // defines
    for (const std::string& path : options->defines())
        command += " -D" + path;

    // Extra arguments
    if (options->compileForLibrary()) {
        if (!contains(command, " -fPIC") && !contains(command, " -fpic"))
            command += " -fPIC";
        command += " -fvisibility=hidden";
    }
    if (options->debugInfoEnabled()) {
        if (!contains(command, " -g") && !contains(command, " -ggdb"))
            command += " -ggdb";
    }

    if (!contains(command, " -W"))
        command += " -Wall";

    m_compileCommandCache[options] = command;
    return compile(fileName, output, options);
}

std::string Gcc::link(const std::string& output, const StringList& objects, const LinkerOptions* options) const
{
    StringList args = objects;
    std::string linker;

    if (options->linkType() == LinkerOptions::StaticLibrary) {
        linker = "ar";
        args.push_front(output);
        args.push_front("-rcs");
    } else {
        if (options->language() == CPlusPlusLanguage)
            linker = "g++";
        else if (options->language() == CLanguage)
            linker = "gcc";
        else
            throw Error("Unsupported programming language sent to the linker!");

        if (options->linkType() == LinkerOptions::SharedLibrary) {
            if (!contains(args, "-fPIC") && !contains(args, "-fpic"))
                args.push_front("-fPIC");
            args.push_front("-shared");
            args.push_front("-Wl,-soname=" + output);
        }
        args.push_back("-o");
        args.push_back(output);

        // custom flags
        StringList flags = options->customFlags();
        std::copy(flags.begin(), flags.end(), std::back_inserter(args));

        // library paths
        StringList paths = options->libraryPaths();
        StringList::iterator it = paths.begin();
        for (; it != paths.end(); ++it)
            args.push_back("-L\"" + *it + '"');

        // libraries
        StringList libraries = options->libraries();
        it = libraries.begin();
        for (; it != libraries.end(); ++it)
            args.push_back("-l" + *it);

        // static libraries
        StringList staticLibs = options->staticLibraries();
        std::copy(staticLibs.begin(), staticLibs.end(), std::back_inserter(args));

        // Add rpath
        if (paths.size())
            args.push_back("-Wl,-rpath=" + join(paths, ":"));
    }

    return linker + ' ' + join(args, " ");
}

std::string Gcc::nameForExecutable(const std::string& name) const
{
    return name;
}

std::string Gcc::nameForStaticLibrary(const std::string& name) const
{
    return "lib" + name + ".a";
}

std::string Gcc::nameForSharedLibrary(const std::string& name) const
{
    return "lib" + name + ".so";
}
