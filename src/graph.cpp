/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "graph.h"
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <fstream>
#include "logger.h"

struct Graph::GraphPrivate
{
    enum Color { WHITE, GRAY, BLACK };
    typedef std::vector<std::set<int> > Edges;
    typedef std::set<int>::const_iterator EdgeIterator;

    Edges edges;

    GraphPrivate(int numNodes) : edges(numNodes)
    {
    }

    void dfsVisit(int node, std::list<int>& result, std::vector<Color>& colors) const
    {
        colors[node] = GRAY;
        EdgeIterator it = edges[node].begin();
        for (; it != edges[node].end(); ++it) {
            if (colors[*it] == WHITE)
                dfsVisit(*it, result, colors);
            else if (colors[*it] == GRAY) // This is not a DAG!
                return;
        }
        colors[node] = BLACK;
        result.push_back(node);
    }
};

Graph::Graph(int numNodes) : m_d(new GraphPrivate(numNodes))
{
}

Graph::~Graph()
{
    delete m_d;
}

int Graph::nodeCount() const
{
    return m_d->edges.size();
}

std::list<int> Graph::topologicalSort() const
{
    size_t nodeCount = Graph::nodeCount();
    std::list<int> result;
    std::vector<GraphPrivate::Color> colors(nodeCount, GraphPrivate::WHITE);

    for (size_t i = 0; i < nodeCount; ++i) {
        if (colors[i] == GraphPrivate::WHITE)
            m_d->dfsVisit(i, result, colors);
    }

    // Not a DAG!
    if (result.size() != nodeCount)
        return std::list<int>();
    return result;
}

std::list<int> Graph::topologicalSortDependencies(int node) const
{
    size_t nodeCount = Graph::nodeCount();
    std::list<int> result;
    std::vector<GraphPrivate::Color> colors(nodeCount, GraphPrivate::WHITE);
    m_d->dfsVisit(node, result, colors);
    return result;
}

bool Graph::containsEdge(int from, int to)
{
    std::set<int>& set = m_d->edges[from];
    return set.find(to) != set.end();
}

void Graph::addEdge(int from, int to)
{
    assert(to < (int)m_d->edges.size());
    m_d->edges[from].insert(to);
}

void Graph::removeEdge(int from, int to)
{
    m_d->edges[from].erase(to);
}

void Graph::dump() const
{
    for (size_t i = 0; i < m_d->edges.size(); ++i) {
        std::cout << i << " -> ";
        std::copy(m_d->edges[i].begin(), m_d->edges[i].end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
    }
}

void Graph::dumpDot(const std::map< int, std::string >& nodeNames, const std::string& fileName) const
{
    std::ofstream output(fileName.c_str(), std::ios_base::out);
    if (!output)
        Error() << "Error writing graph to " << fileName;
    output << "digraph D {\n";
    for (size_t i = 0; i < m_d->edges.size(); ++i) {
        GraphPrivate::EdgeIterator it = m_d->edges[i].begin();
        if (m_d->edges[i].empty()) {
            output << '"' << nodeNames.at(i) << "\"\n";
        } else {
            for (;it != m_d->edges[i].end(); ++it)
                output << '"' << nodeNames.at(i) << "\" -> \"" << nodeNames.at(*it) << "\"\n";
        }
    }
    output << "}\n";
}
