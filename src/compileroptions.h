/*
    This file is part of the Meique project
    Copyright (C) 2010-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef COMPILEROPTIONS_H
#define COMPILEROPTIONS_H

#include "basictypes.h"

class CompilerOptions
{
public:
    CompilerOptions();
    void addIncludePath(const std::string& includePath);
    void addIncludePaths(const StringList& includePaths);
    StringList includePaths() const { return m_includePaths; }
    void addDefine(const std::string& define);
    StringList defines() const { return m_defines; }
    void addCustomFlag(const std::string& customFlag);
    StringList customFlags() const { return m_customFlags; }
    void setCompileForLibrary(bool value) { m_compileForLibrary = value; }
    bool compileForLibrary() const { return m_compileForLibrary; }
    void enableDebugInfo() { m_debugInfoEnabled = true; }
    bool debugInfoEnabled() const { return m_debugInfoEnabled; }
    void normalize();

    void merge(const CompilerOptions& other);
    std::string hash() const;
private:
    StringList m_includePaths;
    StringList m_defines;
    StringList m_customFlags;
    bool m_compileForLibrary;
    bool m_debugInfoEnabled;

    CompilerOptions(const CompilerOptions&) = delete;
};

#endif // COMPILEROPTIONS_H
