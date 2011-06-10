Introduction
============

Short description: Meique is a build system for C and C++ that uses some kind of `Lua <http://www.lua.org>`_ files to describe the build process.

The short description is useless without some kind of "hands on" introduction to know how those files looks like and how meique is supposed to be used, this is the aim of this page.

Before starting, let's list some characteristics of meique:


* It was made with :ref:`out of source builds <out_of_source_builds>` in mind, it's possible to use meique without a separate build directory but not recommended.
* It's not a meta build system like cmake, although the idea of use `ninja <http://martine.github.com/ninja/manual.html>`_ isn´t fully discarded yet.


The infamous Hello World example
--------------------------------

Let's start with a simple hello world file, then evolve to more complex examples.

First you need a hello world in C++ (or C), name it ``hello.cpp``

::

    #include <iostream>
 
    int main() {
        std::cout << “Hello World\n”;
    }

Meique expects you to describe your project in files named "meique.lua" (meique scripts), so you **must** have at least one file named meique.lua at your project root directory. Meique scripts can include other meique scripts, so is a good practice to create one meique.lua file per sub-directory in your project.

Here is the contents of you very first meique script, remember to name it ``meique.lua``.

::

    proj = Executable:new(“hello”)
    proj:addFiles(“hello.cpp”)


Meique uses an object oriented [#luaisntOO]_ imperative language to represent your project, however you only need to know the very basics of OO to use meique, even for the most complex use case.

Line by Line
************

::

    proj = Executable:new(“hello”)

This line says: "Create a new project named hello and put it in the variable proj".

* If you understand the OO terminology, ``new`` is a static method of ``Executable`` type.
* If you understand about Lua, ``new`` is function inside the table ``Executable`` that returns another table to simulate the OO behavior.

Note that the name of the variable has nothing to do with the name of the project and you could use any variable name you want [#reservednames]_, this may confuse you at first but it's a feature, not a bug :-P, due to this behavior you can pass your project to functions, store it in another variables and do what you want with it.


::

    proj:addFiles(“hello.cpp”)

This line just add the file "hello.cpp" to the project, note the use of ":" instead of "." or "->" to call the method addFiles.

It's all, you have one file and no extra options.

Configuring the project
***********************

Before compiling the project you need pass the configure phase where a valid C or C++ compiler besides all project dependencies needs to be found by Meique, but before doing this you also need to create a build directory, so the out of source build can be done.

Let's assume you are on project root directory.

::

    $ mkdir build
    $ cd build
    $ meique ..

The first two lines were trivial, just the creation of the build directory, the last line tell meique to configure the project using the meique.lua file found in the directory "..". Now you can build your project.

Compiling the project
*********************

Just type meique inside the build directory.

::

    $ meique

Meique accepts multiple jobs like the make tool, just pass -jN as argument where N is the number of jobs to run in parallel.

.. _out_of_source_builds:

Out of source builds
====================

When your build generates files, they have to go somewhere. An in-source build puts them in your source tree. An out-of-source build puts them in a completely separate directory, so that your source tree is unchanged.

There are innumerous advantages in using out of source builds, just believe :-)

.. rubric:: Footnotes

.. [#luaisntOO] Lua isn't an object oriented language, but with some tricks it can emulate the OO paradigm, meique uses this tricks.
.. [#reservednames] Names starting with "_meique" are reserved for internal use, so don't use them!
