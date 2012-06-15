/*
    This file is part of the Meique project
    Copyright (C) 2012 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>

using namespace std;

void fatal(const char* msg)
{
    cerr << "file2c: " << msg << endl;
    exit(1);
}

int main(int argc, const char** argv)
{
    if (argc != 3)
        fatal("No enough arguments, use file2c VARIABLE_NAME FILE_TO_BE_ENCODED");

    const char* variableName = argv[1];
    const char* sourceFile = argv[2];


    ifstream input(sourceFile);
    if (!input)
        fatal("Input file not found!");

    cout << "extern const char " << variableName << "[] = {";
    int byteCount = 0;
    while(true) {
        unsigned int c = input.get();
        if (!input.good())
            break;

        if (byteCount % 16 == 0)
            cout << endl;
        cout << "0x" << hex << setw(2) << setfill('0') << c << ", " ;
        byteCount++;
    }
    cout << "0x00 };" << endl;
}
