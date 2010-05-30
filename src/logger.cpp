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

#include "logger.h"

bool MeiqueError::errorAlreadyset = false;

MeiqueError::MeiqueError()
{
    errorAlreadyset = true;
}

std::ostream& operator<<(std::ostream& out, const green&)
{
    return out << COLOR_GREEN;
}

std::ostream& operator<<(std::ostream& out, const red&)
{
    return out << COLOR_RED;
}
std::ostream& operator<<(std::ostream& out, const yellow&)
{
    return out << COLOR_YELLOW;
}

std::ostream& operator<<(std::ostream& out, const nocolor&)
{
    return out << COLOR_END;
}

std::ostream& operator<<(std::ostream& out, const magenta&)
{
    return out << COLOR_MAGENTA;
}
