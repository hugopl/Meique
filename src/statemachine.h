/*
    This file is part of the Meique project
    Copyright (C) 2011 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <map>
#include <cstring>
#include "logger.h"

/**
 * A state machine, you can define the machine with the syntax:
 *
 * machine[state1][event1] = state2;
 *
 * which means, go to state2 when receive event1 on state1.
 *
 * events are integer constants, states are method pointers, the instance obejct
 * used to call the methods is given from StateMachine constructor.
 */

#define STATE(X) std::make_pair(#X, &X)
template<typename T>
class StateMachine
{
public:
    typedef std::pair<std::string, int (T::*)()> State;

    StateMachine(T* subject) : m_subject(subject) {}
    std::map<int, State>& operator[](State state)
    {
        return m_graph[state];
    }

    void execute(State initialState)
    {
        State& currentState = initialState;

        while (!currentState.first.empty()) {
            int event = (m_subject->*(currentState.second))();
            #ifndef NDEBUG
            Warn() << "Got " << event << ", going to state: " << m_graph[currentState][event].first;
            #endif
            currentState = m_graph[currentState][event];
        }
    }

private:
    struct Comparator
    {
        bool operator()(const State& s1, const State& s2)
        {
            return s1.first < s2.first;
        }
    };
    typedef std::map<State, std::map<int, State >, Comparator > Graph;
    Graph m_graph;
    T* m_subject;
};

#endif // STATEMACHINE_H
