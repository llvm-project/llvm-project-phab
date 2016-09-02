======
Rename
======

.. contents::

See also:

.. toctree::
   :maxdepth: 1


`rename` is a module of `clang-refactor`, a C++ "master" refactoring tool. Its
purpose is to perform efficient renaming actions in large-scale projects such as
renaming classes, functions, variables, arguments, namespaces etc.

The tool is in a very early development stage, so you might encounter bugs and
crashes. Submitting reports with information about how to reproduce the issue
to `the LLVM bugtracker <https://llvm.org/bugs>`_ will definitely help the
project. If you have any ideas or suggestions, you might want to put a feature
request there.

Using Rename module
===================

:program:`clang-refactor` is a `LibTooling
<http://clang.llvm.org/docs/LibTooling.html>`_-based tool, and it's easier to
work with if you set up a compile command database for your project (for an
example of how to do this see `How To Setup Tooling For LLVM
<http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html>`_). You can also
specify compilation options on the command line after `--`:

.. code-block:: console

  $ clang-refactor rename -offset=42 -new-name=foo test.cpp -- -Imy_project/include -DMY_DEFINES ...


To get an offset of a symbol in a file run

.. code-block:: console

  $ grep -FUbo 'foo' file.cpp


The tool currently supports renaming actions inside a single translation unit
only. It is planned to extend the tool's functionality to support multi-TU
renaming actions in the future.

:program:`clang-refactor` also aims to be easily integrated into popular text
editors, such as Vim and Emacs, and improve the workflow of users.

Although a command line interface exists, it is highly recommended to use the
text editor interface instead for better experience.

You can also identify one or more symbols to be renamed by giving the fully
qualified name:

.. code-block:: console

  $ clang-refactor rename-old-name=foo -new-name=bar test.cpp


Alternatively, { offset | old name } -> new name pairs can be put into a YAML
file:

.. code-block:: yaml

  ---
  - OldName:        foo
    NewName:        bar
  - Offset:         42
    NewName:        baz
  ...


That way you can avoid spelling out all the names as command line arguments:

.. code-block:: console

  $ clang-refactor rename -input=test.yaml test.cpp


Vim Integration
===============

You can call :program:`clang-refactor rename` directly from Vim! To set up
:program:`clang-refactor-rename` integration for Vim see
`clang-refactor/editor-integrations/clang-refactor-rename.py
<http://reviews.llvm.org/diffusion/L/browse/clang-tools-extra/trunk/clang-refactor/editor-integrations/clang-refactor-rename.py>`_.

Please note that **you have to save all buffers, in which the replacement will
happen before running the tool**.

Once installed, you can point your cursor to symbols you want to rename, press
`<leader>cr` and type new desired name. The `<leader> key
<http://vim.wikia.com/wiki/Mapping_keys_in_Vim_-_Tutorial_(Part_3)#Map_leader>`_
is a reference to a specific key defined by the mapleader variable and is bound
to backslash by default.

Emacs Integration
=================

You can also use :program:`clang-refactor` while using Emacs! To set up
:program:`clang-refactor rename` integration for Emacs see
`clang-refactor/editor-integrations/clang-refactor-rename.el
<http://reviews.llvm.org/diffusion/L/browse/clang-tools-extra/trunk/clang-refactor/editor-integrations/clang-refactor-rename.el>`_.

Once installed, you can point your cursor to symbols you want to rename, press
`M-X`, type `clang-refactor rename` and new desired name.

Please note that **you have to save all buffers, in which the replacement will
happen before running the tool**.
