#===- gen-msvc-exports.py - Generate C API export file -------*- python -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
#
# Generate an export file from a list of given LIB files. Only exports symbols
# that starts with LLVM*, so is only usefull for export the LLVM C API.
#
# To use, build LLVM with Visual Studio, use the Visual Studio Command prompt
# to navigate to the directory with the .lib files (Debug\lib etc). Then run
#     python C:\Path\To\gen-msvc-exports.py LLVM*.lib
#
# If you're generating a 64 bit DLL, use the `--arch=x64` flag.
#
# You can use the --output flag to set the name of the export file.
#
# Generate the 64 bit DLL from the 64 bit prompt, 32 bit from the 32 bit prompt.
#
#===------------------------------------------------------------------------===#
from tempfile import mkstemp
from contextlib import contextmanager
from subprocess import check_call
import argparse
import os
import re


_ARCH_REGEX = {
    'x64': re.compile(r"^\s+\w+\s+(LLVM.*)$"),
    'x86': re.compile(r"^\s+\w+\s+_(LLVM.*)$")
}


@contextmanager
def removing(path):
    try:
        yield path
    finally:
        os.unlink(path)


def touch_tempfile(*args, **kwargs):
    fd, name = mkstemp(*args, **kwargs)
    os.close(fd)
    return name


def gen_llvm_dll(output, arch, libs):
    with removing(touch_tempfile(prefix='dumpout', suffix='.txt')) as dumpout:

        # Get the right regex for this arch.
        p = _ARCH_REGEX[arch]

        with open(output, 'w+t') as output_f:

            # For each lib get the LLVM* functions it exports.
            for lib in libs:
                # Call dumpbin.
                with open(dumpout, 'w+t') as dumpout_f:
                    check_call(['dumpbin', '/linkermember:1', lib], stdout=dumpout_f)

                # Get the matching lines.
                with open(dumpout) as dumpbin:
                    for line in dumpbin:
                        m = p.match(line)
                        if m is not None:
                            output_f.write(m.group(1) + '\n')


def main():
    parser = argparse.ArgumentParser('gen-msvc-exports')

    parser.add_argument(
        '-o', '--output', help='output filename', default='LLVM-C.exports'
    )
    parser.add_argument(
        '--arch', help='architecture', default='x86', choices=['x86', 'x64'], type=str.lower
    )
    parser.add_argument(
        'libs', metavar='LIBS', nargs='+', help='list of libraries to generate export from'
    )

    ns = parser.parse_args()

    gen_llvm_dll(ns.output, ns.arch, ns.libs)


if __name__ == '__main__':
    main()
