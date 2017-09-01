#!/usr/bin/env python
#===----------------------------------------------------------------------===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.
#
#===----------------------------------------------------------------------===##

# USAGE: check_experimental_visibility <libcpp-include-dir>
# This script checks if each header in the given include dir use the correct
# visibility macros. Experimental headers should use _LIBCPPX macros whereas
# non-experimental headers should use the regular _LIBCPP macros.

import itertools
import os
import sys

visibility_macros = [
    "HIDDEN",
    "FUNC_VIS",
    "EXTERN_VIS",
    "OVERRIDABLE_FUNC_VIS",
    "INLINE_VISIBILITY",
    "ALWAYS_INLINE",
    "TYPE_VIS",
    "TEMPLATE_VIS",
    "ENUM_VIS",
    "EXTERN_TEMPLATE_TYPE_VIS",
    "CLASS_TEMPLATE_INSTANTIATION_VIS",
    "METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS",
    "EXTERN_TEMPLATE_INLINE_VISIBILITY",
    "EXCEPTION_ABI",
]

regular_visibility_macros      = ["_LIBCPP_{}".format(m)  for m in visibility_macros]
experimental_visibility_macros = ["_LIBCPPX_{}".format(m) for m in visibility_macros]

def main():
    if len(sys.argv) != 2:
        sys.stderr.write("Expected only one argument: the libcxx include path\n")
        return 1

    include_dir = sys.argv[1]
    if not os.path.isdir(include_dir):
        sys.stderr.write("Given include path isn't a directory\n")
        return 1

    # [(rel_file_path, line_number)]
    invalid_regular_files = []
    invalid_experimental_files = []

    files_to_check = itertools.chain.from_iterable((
        ((dir, fname) for fname in filenames if fname != "__config")
        for (dir, _, filenames) in os.walk(include_dir)
    ))
    for (dir, fname) in files_to_check:
        parent_path = os.path.relpath(dir, include_dir)

        header_path = os.path.join(parent_path, fname) if parent_path != '.' else fname
        experimental = os.path.split(header_path)[0] == "experimental"

        # List of macros that we shouldn't see in this file.
        problem_macros = regular_visibility_macros if experimental \
                         else experimental_visibility_macros

        with open(os.path.join(dir, fname), 'r') as f:
            problem_lines = (i for (i, line) in enumerate(f, 1)
                             if any(m in line for m in problem_macros))
            (invalid_experimental_files if experimental else invalid_regular_files) \
                .extend((header_path, l) for l in problem_lines)

    if not invalid_regular_files and not invalid_experimental_files:
        # No problems found
        return 0

    if invalid_regular_files:
        sys.stderr.write(
            "Found usage of experimental visibility macros in non-experimental files.\n"
            "These should be changed to the corresponding _LIBCPP macros.\n")
        for (path, line_num) in invalid_regular_files:
            sys.stderr.write("  <{}>:{:d}\n".format(path, line_num))
        if invalid_experimental_files:
            sys.stderr.write("\n\n")

    if invalid_experimental_files:
        sys.stderr.write(
            "Found usage of non-experimental visibility macros in experimental files.\n"
            "These should be changed to the corresponding _LIBCPPX macros\n")
        for (path, line_num) in invalid_experimental_files:
            sys.stderr.write("  <{}>:{:d}\n".format(path, line_num))

    return 1

if __name__ == '__main__':
    sys.exit(main())

