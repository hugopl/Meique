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

#include "nodevisitor.h"

#include "logger.h"

NodeVisitor::NodeVisitor(const NodeTree& tree, std::function<void (Node *)> visitor)
    : m_visitor(visitor)
{
    for (Node* parentNode : tree) {
        if (m_status[parentNode] == NotVisited) {
            visitNode(parentNode);
        }
    }
}

NodeVisitor::NodeVisitor(Node *root, std::function<void (Node *)> visitor)
    : m_visitor(visitor)
{
    visitNode(root);
}


void NodeVisitor::visitNode(Node* node)
{
    m_status[node] = Visiting;
    for (Node* child : node->children) {
        VisitStatus& status = m_status[child];
        switch(status) {
        case NotVisited:
            visitNode(child);
            break;
        case Visiting:
            throw Error("Node tree is cyclic!");
        case Visited:
            return;
        }
    }
    m_visitor(node);
    m_status[node] = Visited;
}

EdgeVisitor::EdgeVisitor(const NodeTree& tree, std::function<void (Node*, Node*)> visitor)
{
    NodeVisitor(tree, [&](Node* node){
        for (Node* child : node->children)
            visitor(node, child);
    });
}
