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

#include <forward_list>
#include <unordered_map>
#include <lua.h>

class Node;
typedef std::forward_list<Node*> NodeList;

class Node
{
public:
    Node(const std::string& name);
    ~Node();
    char* name;
    NodeList parents;
    NodeList children;
private:
    Node(const Node&);
    Node& operator=(const Node&);
};

class NodeTree
{
    typedef std::unordered_map<std::string, Node*> TargetNodeMap;
public:
    NodeTree(lua_State* L, const std::string& root);
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

    // Iterate over target nodes
    NodeTree::Iterator begin() const;
    NodeTree::Iterator end() const;

    void expandTargetNode(const std::string& target = std::string());

    void dump(const char* fileName = 0) const;

private:
    void buildNotExpandedTree();

    void pushTarget(const std::string& target);

    lua_State* m_L;
    TargetNodeMap m_targetNodes;
};

#endif
