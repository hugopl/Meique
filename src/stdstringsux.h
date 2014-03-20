/*
    This file is part of the Meique project
    Copyright (C) 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef STDSTRINGSUX_H
#define STDSTRINGSUX_H

#include <string>
#include <algorithm>
#include "basictypes.h"

std::string& stringReplace(std::string& str, const std::string& substr, const std::string& replace);

std::string escape(std::string str);

/**
*   Remove the left and right blank caracters of the string \p str
*   \param  str The string that will be "trim'ed".
*/
void trim(std::string& str);

StringList split(const std::string& str, char sep = ' ');
std::string join(const StringList& list, const std::string& sep);

inline bool contains(const std::string& str, const char* value)
{
    return str.find(value) != std::string::npos;
}

template<typename T>
bool contains(T container, typename T::value_type value) {
    return std::find(container.begin(), container.end(), value) != container.end();
}

template<typename T, typename Predicate>
void eraseIf(T& container, Predicate p) {
    for(auto it = container.begin(), endIt = container.end(); it != endIt;){
        if(p(*it))
            it = container.erase(it);
        else
            ++it;
    }
}

#endif
