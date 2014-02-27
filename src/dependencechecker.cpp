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

#include "dependencechecker.h"

#include "compiler.h"
#include "os.h"
#include "meiqueregex.h"

#include <algorithm>
#include <fstream>

bool DependenceChecker::shouldCompile(const std::string& file, const std::string& output)
{
    for (const std::string& dep : getDependencies(file)) {
        if (OS::timestampCompare(dep, output) < 0)
            return true;
    }
    return false;
}

StringList DependenceChecker::getDependencies(const std::string& file)
{
    StringList dependencies;
    StringList includePaths;
    includePaths.push_back(m_cwd);
    preprocessFile(file, includePaths, dependencies);
    return std::move(dependencies);
}

void DependenceChecker::preprocessFile(const std::string& source, StringList& includeDirs, StringList& deps)
{
    static Regex regex("^\\s*#\\s*include\\s*[<\"]([^\">]+)[\">]");

    if (source.empty())
        return;

    std::string absSource;
    std::ifstream fp;

    if (source[0] == '/') {
        absSource = source;
        fp.open(source.c_str());
    } else {
        for (const std::string& dir : includeDirs) {
            absSource = dir + source;
            fp.open(absSource.c_str());
            if (fp) {
                absSource = OS::normalizeFilePath(absSource);
                break;
            }
        }
    }

    if (!fp || !fp.is_open())
        return;

    if (std::find(deps.begin(), deps.end(), absSource) != deps.end())
        return;

    deps.push_back(absSource);

    std::string line;

    while (fp) {
        std::getline(fp, line);
        // Avoid regex execution non-preprocessor lines
        if (line.find_first_of('#') == std::string::npos)
            continue;

        if (fp && regex.match(line)) {
            includeDirs.push_front(OS::dirName(absSource));
            preprocessFile(regex.group(1, line), includeDirs, deps);
            includeDirs.pop_front();
        }
    }
}
