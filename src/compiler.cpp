/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "compiler.h"
#include <algorithm>
#include "logger.h"

Language identifyLanguage(const std::string& fileName)
{
    size_t idx = fileName.find_last_of('.');
    if (idx == std::string::npos || idx == fileName.length())
        return UnsupportedLanguage;
    std::string ext = fileName.substr(idx+1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "c")
        return CLanguage;

    const char* cppExtensions[] = { "cpp", "cxx", "c++", "cc", "cp", 0 };
    for (int i = 0; cppExtensions[i]; ++i) {
        if (ext == cppExtensions[i])
            return CPlusPlusLanguage;
    }
    return UnsupportedLanguage;
}

