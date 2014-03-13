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

#ifndef JOBFACTORY_H
#define JOBFACTORY_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

#include "compileroptions.h"
#include "dependencechecker.h"
#include "linkeroptions.h"
#include "nodetree.h"

class MeiqueScript;
class Job;

class JobFactory
{
public:
    JobFactory(MeiqueScript& script, const StringList& targets);
    ~JobFactory();

    Job* createJob();
    unsigned processedNodes() const { return m_processedNodes; }
    unsigned nodeCount() const;
private:
    JobFactory(const JobFactory&) = delete;

    struct Options {
        std::string targetDirectory;
        CompilerOptions compilerOptions;
        LinkerOptions linkerOptions;
    };

    Node* findAGoodNode(Node** target, Node* node);
    Job* createCompilationJob(Node* target, Node* node);
    Job* createTargetJob(Node* target);
    Job* createCustomTargetJob(Node* target);
    Job* createHookJob(Node* target, Node* node);
    void fillTargetOptions(Node* node, Options* options);
    void mergeCompilerAndLinkerOptions(Node* node);
    void cacheTargetCompilerOptions(Node* node);

    MeiqueScript& m_script;
    NodeTree m_nodeTree;
    Node* m_root;

    bool m_needToWait;
    std::mutex m_treeChangedMutex;
    std::condition_variable m_treeChanged;
    bool m_treeChangedMeanWhile;

    unsigned m_processedNodes;

    DependenceChecker m_depChecker;
    typedef std::unordered_map<Node*, Options*> CompilerOptionsMap;
    CompilerOptionsMap m_targetCompilerOptions;
};

#endif
