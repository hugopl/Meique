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

#include "nodevisitor.h"
#include "luacpputil.h"
#include "logger.h"

Node::Node(const std::string& name)
    : name(strdup(name.c_str()))
{
}

Node::~Node()
{
    free(name);
}

NodeTree::NodeTree(lua_State* L, const std::string& root)
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
        out << parent->name << " -> " << child->name << "\n";
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

void NodeTree::expandTargetNode(const std::string& /*target*/)
{
    LuaLeakCheck(m_L);

    // TODO: Use target to expand selected nodes instead of all nodes
    for (Node* targetNode : *this) {
        pushTarget(targetNode->name);
        lua_getfield(m_L, -1, "_files");
        StringList files;
        readLuaList(m_L, lua_gettop(m_L), files);
        for (std::string& file : files) {
            Node* fileNode = new Node(file);
            fileNode->parents.push_front(targetNode);
            targetNode->children.push_front(fileNode);
        }
        lua_pop(m_L, 3);
    }
    dump();
}

void NodeTree::buildNotExpandedTree()
{
    LuaLeakCheck(m_L);

    // Get all targets
    lua_getglobal(m_L, "_meiqueAllTargets");
    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    while (lua_next(m_L, tableIndex) != 0) {
        // Get target type
        lua_getfield(m_L, -1, "_name");
        std::string targetName = lua_tocpp<std::string>(m_L, -1);
        m_targetNodes[targetName] = new Node(targetName);
        lua_pop(m_L, 2);
    }
    lua_pop(m_L, 1);

    // Connect the targets regarding their dependencies
    for (auto pair : m_targetNodes) {
        const std::string& target = pair.first;
        pushTarget(target);
        // Get dependencies
        lua_getfield(m_L, -1, "_deps");
        StringList deps;
        readLuaList(m_L, lua_gettop(m_L), deps);
        lua_pop(m_L, 3);

        for (const std::string& dep : deps) {
            Node*& targetNode = m_targetNodes[target];
            Node*& depNode = m_targetNodes[dep];
            targetNode->children.push_front(depNode);
            depNode->parents.push_front(targetNode);
        }
    }
}

void NodeTree::pushTarget(const std::string& target)
{
    lua_getglobal(m_L, "_meiqueAllTargets");
    lua_getfield(m_L, -1, target.c_str());
}
