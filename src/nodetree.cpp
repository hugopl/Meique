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

#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <list>

#include "nodevisitor.h"
#include "luacpputil.h"
#include "logger.h"

Node::Node(const std::string& name)
    : name(strdup(name.c_str()))
    , status(Pristine)
    , targetType(0)
    , isTarget(false)
    , hasCachedCompilerFlags(false)
    , shouldBuild(false)
    , isFake(false)
{
}

Node::~Node()
{
    free(name);
}

NodeTree::NodeTree(lua_State* L)
    : m_L(L)
{
    buildNotExpandedTree();
}

NodeTree::~NodeTree()
{
    NodeVisitor(*this, std::default_delete<Node>());
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
    luaPushTarget(target);
    lua_getfield(m_L, -1, "_deps");
    StringList deps;
    readLuaList(m_L, lua_gettop(m_L), deps);
    lua_pop(m_L, 2);
    for (std::string& dep : deps)
        expandTargetNode(dep);

    if (target->isCustomTarget())
        return;

    // populate the node
    luaPushTarget(target);
    lua_getfield(m_L, -1, "_files");
    StringList files;
    readLuaList(m_L, lua_gettop(m_L), files);
    for (std::string& file : files) {
        Node* fileNode = new Node(file);
        fileNode->parents.push_back(target);
        target->children.push_back(fileNode);
    }
    lua_pop(m_L, 2);
}

void NodeTree::expandTargetNode(const std::string& target)
{
    Node* targetNode = m_targetNodes[target];
    if (!targetNode)
        throw Error("Unknow target \"" + target + "\".");

    expandTargetNode(targetNode);
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
        lua_getfield(m_L, -1, "_name");
        std::string targetName = lua_tocpp<std::string>(m_L, -1);
        Node* node = new Node(targetName);
        node->isTarget = true;

        // Get target type
        lua_getfield(m_L, -2, "_type");
        node->targetType = lua_tocpp<int>(m_L, -1);

        m_targetNodes[targetName] = node;
        lua_pop(m_L, 3);
    }
    lua_pop(m_L, 1);

    if (m_targetNodes.empty())
        throw Error("No targets found in this meique.lua file.");

    // Connect the targets regarding their dependencies
    for (auto pair : m_targetNodes) {
        const std::string& target = pair.first;
        luaPushTarget(target);
        // Get dependencies
        lua_getfield(m_L, -1, "_deps");
        StringList deps;
        readLuaList(m_L, lua_gettop(m_L), deps);
        lua_pop(m_L, 2);

        for (const std::string& dep : deps) {
            Node*& targetNode = m_targetNodes[target];
            Node*& depNode = m_targetNodes[dep];
            targetNode->children.push_front(depNode);
            depNode->parents.push_front(targetNode);
        }
    }

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

void NodeTree::luaPushTarget(const Node* node)
{
    luaPushTarget(node->name);
}

void NodeTree::luaPushTarget(const std::string& target)
{
    lua_getglobal(m_L, "_meiqueAllTargets");
    lua_getfield(m_L, -1, target.c_str());
    lua_remove(m_L, -2);
}

NodeGuard* NodeTree::createNodeGuard(Node* node)
{
    return new NodeGuard(this, node, m_mutex);
}

NodeGuard::NodeGuard(NodeTree* tree, Node* node, std::mutex& mutex)
    : m_tree(tree)
    , m_node(node)
    , m_mutex(mutex)
{
}

NodeGuard::~NodeGuard()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_node->status = Node::Built;
    }
    m_tree->onTreeChange();
}
