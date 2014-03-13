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

#include "nodetree.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <memory>
#include <list>

#include "meiquescript.h"
#include "nodevisitor.h"
#include "luacpputil.h"
#include "logger.h"

Node::Node(const std::string& name)
    : name(name)
    , status(Pristine)
    , targetType(0)
    , isTarget(false)
    , hasCachedCompilerFlags(false)
    , shouldBuild(false)
    , isFake(false)
    , isHook(false)
{
}

NodeTree::NodeTree(MeiqueScript& script, const StringList& targets)
    : m_script(script)
    , m_L(script.luaState())
{
    buildNotExpandedTree();
    if (!targets.empty())
        removeUnusedTargets(targets);
    connectForest();
    addTargetHookNodes();
    m_size = m_targetNodes.size();
}

NodeTree::~NodeTree()
{
    NodeVisitor<>(*this, std::default_delete<Node>());
}

void NodeTree::dump(const char* fileName) const
{
    if (m_targetNodes.empty()) {
        Warn() << "Can't dump an empty graph.";
        return;
    }

    std::ofstream out(fileName ? fileName : "/tmp/nodes.dot");
    out << "digraph nodes {\n";

    EdgeVisitor(*this, [&](Node* parent, Node* child) {
        out << '"' << parent->name << "\" -> \"" << child->name << "\"\n";
    });

    out << "}\n";
}

NodeTree::Iterator NodeTree::begin() const
{
    return NodeTree::Iterator(m_targetNodes.begin());
}

NodeTree::Iterator NodeTree::end() const
{
    return NodeTree::Iterator(m_targetNodes.end());
}

void NodeTree::expandTargetNode(Node* target)
{
    LuaLeakCheck(m_L);

    if (target->status > Node::Pristine)
        return;

    target->status = Node::Expanded;

    if (target->isFake) {
        for (Node* child : target->children)
            expandTargetNode(child);
        return;
    }

    // Expand the dependencies first
    m_script.luaPushTarget(target->name);
    LuaAutoPop autoPop(m_L);

    for (std::string& dep : luaGetField<StringList>(m_L, "_deps"))
        expandTargetNode(dep);

    if (target->isCustomTarget())
        return;

    // populate the node
    for (std::string& file : luaGetField<StringList>(m_L, "_files")) {
        Node* fileNode = new Node(file);
        fileNode->shouldBuild = target->shouldBuild;
        fileNode->parents.push_back(target);
        target->children.push_back(fileNode);
        m_size++;
    }
}

void NodeTree::expandTargetNode(const std::string& target)
{
    Node* targetNode = m_targetNodes[target];
    if (!targetNode)
        throw Error("Unknow target \"" + target + "\".");

    expandTargetNode(targetNode);
}

void NodeTree::removeUnusedTargets(const StringList &targets)
{
    StringSet usedTargets;
    for (const std::string& target : targets) {
        TargetNodeMap::iterator it = m_targetNodes.find(target);
        if (it != m_targetNodes.end()) {
            NodeVisitor<>(it->second, [&](Node* node) {
                usedTargets.insert(node->name);
            });
        } else {
            std::string msg = target == ".." ? "Call meique without \"..\", you need to tell where meique.lua file is just once ;-)." : "Target not found \"" + target + '\"';
            throw Error(msg);
        }
    }

    StringSet allTargets;
    for (auto pair: m_targetNodes)
        allTargets.insert(pair.first);

    StringSet targetsToBeRemoved;
    std::set_difference(allTargets.begin(), allTargets.end(), usedTargets.begin(), usedTargets.end(),
                        std::insert_iterator<StringSet>(targetsToBeRemoved, targetsToBeRemoved.begin()));

    for (const std::string& target : targetsToBeRemoved) {
        Node* node = m_targetNodes[target];
        m_targetNodes.erase(target);

        for (Node* child : node->children)
            child->parents.remove(node);

        for (Node* parent : node->parents)
            parent->children.remove(node);

        delete node;
    }
}

void NodeTree::buildNotExpandedTree()
{
    LuaLeakCheck(m_L);

    // Get all targets
    lua_getglobal(m_L, "_meiqueAllTargets");
    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    while (lua_next(m_L, tableIndex) != 0) {
        // Get target name
        std::string targetName = luaGetField<std::string>(m_L, "_name");
        Node* node = new Node(targetName);
        node->isTarget = true;
        node->targetType = luaGetField<int>(m_L, "_type");

        m_targetNodes[targetName] = node;
        lua_pop(m_L, 1);
    }
    lua_pop(m_L, 1);

    if (m_targetNodes.empty())
        throw Error("No targets found in this meique.lua file.");

    // Connect the targets regarding their dependencies
    for (auto pair : m_targetNodes) {
        const std::string& target = pair.first;
        m_script.luaPushTarget(target);
        StringList deps = luaGetField<StringList>(m_L, "_deps");
        lua_pop(m_L, 1);

        for (const std::string& dep : deps) {
            Node*& targetNode = m_targetNodes[target];
            Node*& depNode = m_targetNodes[dep];
            targetNode->children.push_front(depNode);
            depNode->parents.push_front(targetNode);
        }
    }
}

void NodeTree::connectForest()
{
    // Connect trees to create a single tree
    std::list<Node*> roots;
    for (auto pair : m_targetNodes) {
        Node*& node = pair.second;
        if (node->parents.empty())
            roots.push_back(node);
    }

    auto numRoots = roots.size();
    // FIXME: Replace this by a proper circular dependency check!
    if (!numRoots) {
        throw Error("Circular dependency found!");
    } else if (numRoots == 1) {
        m_root = roots.front();
    } else {
        m_root = new Node("<fakeroot>");
        m_root->isFake = true;
        for (Node* root : roots) {
            m_root->children.push_front(root);
            root->parents.push_front(m_root);
        }
    }
}

void NodeTree::addTargetHookNodes()
{
    LuaLeakCheck(m_L);

    for (const auto& pair : m_targetNodes) {
        if (m_script.hasHook(pair.first.c_str())) {
            Node* hookNode = new Node("<hook>");
            hookNode->isFake = true;
            hookNode->isHook = true;
            hookNode->parents.push_back(pair.second);
            pair.second->children.push_back(hookNode);
        }
    }
}

NodeGuard::NodeGuard(NodeTree& tree, Node* node)
    : m_tree(tree)
    , m_node(node)
{
}

NodeGuard::~NodeGuard()
{
    {
        std::lock_guard<NodeTree> lock(m_tree);
        m_node->status = Node::Built;
    }
    m_tree.onTreeChange();
}
