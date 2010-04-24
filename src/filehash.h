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

#ifndef FILEHASH_H
#define FILEHASH_H

#include <string>

/**
*   A file hash, it does not need to be unique, but capable of detect changes
*   between different files.
*/
class FileHash
{
public:
    FileHash(const std::string& fileName);
    static FileHash fromString(const std::string& hash);
    bool operator==(const FileHash& other) const;
    bool operator!=(const FileHash& other) const;
    std::string toString() const;
private:
    std::string m_hash;

    FileHash() {}
};

#endif // FILEHASH_H
