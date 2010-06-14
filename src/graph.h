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

#ifndef GRAPH_H
#define GRAPH_H

#include "basictypes.h"

/// A graph that can have their nodes topologically sorted.
class Graph
{
public:
    /// Create a new graph with \p numNodes nodes.
    Graph(int numNodes);
    ~Graph();

    /// Returns the numbed of nodes in this graph.
    int nodeCount() const;
    /// Returns true if the graph contains the edge from -> to
    bool containsEdge(int from, int to);
    /// Adds an edge to this graph.
    void addEdge(int from, int to);
    /// Removes an edge out of this graph.
    void removeEdge(int from, int to);
    /// Print this graph to stdout.
    void dump() const;
    /**
    *   Dumps a dot graph to a file named \p filename.
    *   \param nodeNames map used to translate node ids to human readable text.
    *   \param fileName file name where the output should be written.
    */
    void dumpDot(const std::map<int, std::string>& nodeNames, const std::string& fileName) const;

    /**
    *   Topologically sort this graph.
    *   \return A collection with all nodes topologically sorted or an empty collection if a ciclic dependency was found.
    */
    std::list<int> topologicalSort() const;
    std::list<int> topologicalSortDependencies(int node) const;
private:

    struct GraphPrivate;
    GraphPrivate* m_d;
};

#endif
