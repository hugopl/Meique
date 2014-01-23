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

#ifndef NODEVISITOR_H
#define NODEVISITOR_H

#include "nodetree.h"
#include <functional>
#include <unordered_map>

class NodeVisitor
{
public:
    NodeVisitor(const NodeTree& tree, std::function<void(Node*)> visitor);
    NodeVisitor(Node* root, std::function<void(Node*)> visitor);

private:
    void visitNode(Node* node);

    enum VisitStatus {
        NotVisited,
        Visiting,
        Visited
    };

    std::function<void(Node*)> m_visitor;
    std::unordered_map<Node*, VisitStatus> m_status;
};

class EdgeVisitor
{
public:
    EdgeVisitor(const NodeTree& tree, std::function<void(Node*,Node*)> visitor);
};

#endif
