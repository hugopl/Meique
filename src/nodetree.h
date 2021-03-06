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

#ifndef NODETREE_H
#define NODETREE_H

#include <list>
#include <unordered_map>
#include <lua.h>
#include <mutex>
#include "basictypes.h"

class MeiqueScript;
class Node;
class NodeTree;
typedef std::list<Node*> NodeList;

class Node
{
public:

    enum Status {
        Pristine,
        Expanded,
        Building,
        Built,
    };

    // This need to keep in sync with the constants in meiqueapi.lua
    enum Type {
        ExecutableTarget = 1,
        LibraryTarget,
        CustomTarget
    };

    explicit Node(const std::string& name);

    bool isCustomTarget() const { return targetType == Node::CustomTarget; }
    bool isLibraryTarget() const { return targetType == Node::LibraryTarget; }

    std::string name;
    NodeList parents;
    NodeList children;
    unsigned status:2;
    unsigned targetType:2;
    unsigned isTarget:1;
    unsigned hasCachedCompilerFlags:1;
    unsigned shouldBuild:1;
    unsigned isFake:1;
    unsigned isHook:1;

private:
    Node(const Node&) = delete;
};

class NodeGuard {
public:
    NodeGuard(NodeTree& tree, Node* node);
    ~NodeGuard();

    void failed() { m_failed = true; }

    NodeGuard(const NodeGuard&) = delete;
    NodeGuard& operator=(const NodeGuard&) = delete;
private:
    NodeTree& m_tree;
    Node* m_node;
    bool m_failed;
};

class NodeTree
{
    typedef std::unordered_map<std::string, Node*> TargetNodeMap;
public:
    explicit NodeTree(MeiqueScript& script, const StringList& targets = StringList());
    ~NodeTree();

    class Iterator {
    public:
        Iterator(TargetNodeMap::const_iterator it) : m_it(it) { }
        bool operator !=(const Iterator& other) { return m_it != other.m_it; }
        Iterator& operator++() { ++m_it; return *this; }
        Node* operator*() { return m_it->second; }
    private:
        TargetNodeMap::const_iterator m_it;
    };

    unsigned size() const { return m_size; }

    // Iterate over target nodes
    NodeTree::Iterator begin() const;
    NodeTree::Iterator end() const;

    lua_State* luaState() const { return m_L; }
    Node* getTargetNode(const std::string& target) const { return m_targetNodes.at(target); }

    void expandTargetNode(Node* target);
    void expandTargetNode(const std::string& target);

    void dump(const char* fileName = 0) const;
    Node* root() const { return m_root; }

    std::function<void ()> onTreeChange;

    void failed() { m_hasFail = true; }
    bool hasFail() const { return m_hasFail; }

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }
private:
    void buildNotExpandedTree();
    void removeUnusedTargets(const StringList& targets);
    void connectForest(const StringList& selectedTargets);
    void addTargetHookNodes();

    MeiqueScript& m_script;
    lua_State* m_L;
    TargetNodeMap m_targetNodes;
    Node* m_root;
    bool m_hasFail;

    unsigned m_size;

    std::mutex m_mutex;

    NodeTree(const NodeTree&) = delete;
};

#endif
