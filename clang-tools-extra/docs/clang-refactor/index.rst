==============
Clang-Refactor
==============

.. contents::

See also:

.. toctree::
   :maxdepth: 1

:program:`clang-refactor` is a Clang-based refactoring "master" tool. It is home
for refactoring submodules, such as `rename`. :program:`clang-refactor` is only
a prototype at the moment and most of its parts may significantly change.

.. code-block:: console

  $ clang-refactor --help

  USAGE: clang-refactor [subcommand] [options] <source0> [... <sourceN>]

  Subcommands:
    rename: rename the symbol found at <offset>s or by <qualified-name>s in <source0>.

=======
Modules
=======

.. toctree::
   :maxdepth: 1

   rename.rst
