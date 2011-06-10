Scope Variables
===============

Meique scopes could be interpreted as a syntax sugar for if's, their were inspired in qmake inline scopes.

Suppose you want to add a file, flag, whatever to your project only on a debug build on an Linux box,
with meique scopes you could write:

::

    DEBUG:LINUX:proj:addFile("linuxdbg.cpp")

This line reads "*if it's a debug build and we are in a Linux box, do what I'm telling you to do*".

Meique provide some :ref:`predefined scopes <predefined_scopes>` describing the system and the build type, but scopes can also be returned
by some meique script functions or your own functions.

Nesting scopes is equivalent to an *and* operation, if you want the equivalent of an *or* operation you can write:

::

(DEBUG or LINUX):addFile("linuxdbg.cpp")


Returning scopes on user functions
**********************************

TBW
