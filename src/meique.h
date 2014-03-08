/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef MEIQUE_H
#define MEIQUE_H
#include "cmdline.h"

class MeiqueScript;

class Meique
{
public:
    Meique(int argc, const char** argv);
    ~Meique();
    void exec();
private:
    CmdLine m_args;
    MeiqueScript* m_script;
    bool m_firstRun;

    StringList getChosenTargetNames();
    void printOptionsSummary();

    // program states
    int checkArgs();
    int lookForMeiqueCache();
    int showVersion();
    int showHelp();
    int dumpProject();
    int lookForMeiqueLua();
    int configureProject();
    int getBuildAction();
    int testTargets();
    int installTargets();
    int uninstallTargets();
    int buildTargets();
    int cleanTargets();

    Meique(const Meique&) = delete;
};

#endif
