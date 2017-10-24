This directory contains LLVM bindings for the OCaml programming language
(http://ocaml.org).

Prerequisites
-------------

* OCaml 4.00.0+.
* ocamlfind.
* ctypes 0.4+.
* oUnit 2+ (only required for tests).
* CMake (to build LLVM).
* Python 2.7+ (to build LLVM).

Building the bindings
---------------------

If all dependencies are present, the bindings will be built and installed
as a part of the default CMake configuration, with no further action.
They will only work with the specific OCaml compiler detected during the build.

The bindings can also be built out-of-tree, i.e. targeting a preinstalled
LLVM. To do this, configure the LLVM build tree as follows:

    $ cmake -DLLVM_OCAML_EXTERNAL_LLVM_LIBDIR=[Location of the preinstalled LLVM] \
            -DCMAKE_BUILD_TYPE=[Build-mode of the preinstalled LLVM] \
            -DLLVM_TARGETS_TO_BUILD=[Targets built of the preinstalled LLVM] \
            -DLLVM_OCAML_INSTALL_PATH=[OCaml install prefix] \
            [... any other options]

You might also want to set -DBUILD_SHARED_LIBS depending on whether you want a statically or dynamically linked library.
This requires at least that you check if your preinstalled LLVM supports dynamic linking.

then build and install it as:

    $ make ocaml_all
    $ cmake -P bindings/ocaml/cmake_install.cmake
