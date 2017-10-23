======================================
Control Flow Verification Tool Design
======================================

.. contents::
   :local:

Objective
=========

This document provides an overview of an external tool to verify the protection
mechanisms implemented by Clang's *Control Flow Integrity* (CFI) schemes
(``-fsanitize=cfi``). This tool, provided a binary or DSO, should infer whether
indirect control flow operations are protected by CFI, and should output a
summary of these results in a human-readable form.

Location
========

This tool will be present as a part of the LLVM toolchain, and will reside in
the "/tools/llvm-cfi-verify" directory, relative to the LLVM trunk. It will
be tested in two methods:

- Unit tests to validate code sections, present in "/unittests/tools/llvm-cfi-
  verify".

Background
==========

This tool disassembles and analyses executable binaries and DSO's to check
whether CFI is protecting all indirect control flow instructions. We perform
this analysis on the produced machine code, as any bugs present in the
compiler/linker may manifest in corruption of the CFI instrumentation, and we
want to ensure the final executable code is protected.

Unprotected indirect control flow instructions will be flagged for manual
review. These unexpected control flows may simply have not been accounted for in
the compiler implementation of CFI (e.g. indirect jumps to facilitate switch
statements may not be fully protected).

It may be possible in the future to extend this tool to flag unnecessary CFI
directives (e.g. CFI directives around a static call to a non-polymorphic base
type). This type of directive has no security implications, but may present
performance impacts.

Design Ideas
============

This tool disassembles binaries and DSO's from their machine code format and
analyse the disassembled machine code. The tool will inspect virtual calls and
indirect function calls. This tool will also inspect indirect jumps, as inlined
functions and jump tables should also be subject to CFI protections. Non-virtual
calls (``-fsanitize=cfi-nvcall``) and cast checks (``-fsanitize=cfi-*cast*``)
are not inspected due to a lack of information provided by the bytecode.

The tool operates by first disassembling all executable sections in the provided
file. All instructions that modify the control flow to a position that
deterministic before runtime are recorded. Indirect control flow instructions
are identified, and the tool will find all code blocks that can transfer control
flow to this indirect CF. Each of these code blocks will meet exactly one of the
following conditions:

- If the top of the block terminates from a conditional branch, the other
  possible path from the conditional branch is explored. This other path must
  terminate at a CFI-trap (``ud2`` on x86) instruction, otherwise the conditional
  branch is marked as CFI-unprotected.

- If the top of the block terminates from something other than a conditional
  branch, this "orphaned" block is marked as CFI-unprotected.

If **any** of these code blocks are CFI-unprotected, the source indirect CF
instruction is marked as CFI-unprotected.

Note that in the first case outlined above (where the top of the block
terminates from a conditional branch) a secondary 'spill graph' is constructed,
to ensure the register argument used by the indirect CF is not spilled from the
stack at any point between the conditional branch and the indirect CF. If there
are any spills that affect the target register, the target is marked as CFI-
unprotected.

The tool will print all indirect CF instructions, and their CFI-protection
status. A summary of protected versus unprotected instruction count will also be
provided.

Other Design Notes
~~~~~~~~~~~~~~~~~~

Only machine code sections that are marked as executable will be subject to this
analysis. Non-executable sections do not require analysis as any execution
present in these sections has already violated the control flow integrity.

Suitable extensions may be made at a later date to include anaylsis for indirect
control flow operations across DSO boundaries. Currently, these CFI features are
only experimental with an unstable ABI, making them unsuitable for analysis.

This method of analysis is free of any false-negative identifications (i.e. the
tool will not report CFI-unprotected instructions as CFI-protected). There are,
however, three possible sources of false-positives that have been identified
during testing.

- There is often a small amount of data present in the executable sections of a
  binary. In a perfect world there would be no "executable data", but this
  assumption unfortunately can't be made for most programs. To attempt to
  reduce these false positives, we utilise the DWARF line table information to
  cross-reference indirect CF instructions with. If the indirect CF instruction
  doesn't match a line table entry within a range (specified by
  ``--dwarf-search-range``), the instruction is treated as data and subsequently
  ignored. This scheme requires all statically linked objects into the file to
  be compiled with ``-g``. This scheme can also be disabled by using the
  ``--ignore-dwarf`` argument.

- During compilation, users can specify a *sanitizer special case list*, which
  disables CFI for a set of functions/types/files. Currently all CFI-unprotected
  instructions resultant from this blacklist are simply displayed as
  CFI-unprotected. There are plans to extend the tool to attempt to explain
  *why* a certain instruction was CFI-unprotected. This would allow users to
  differentiate between CFI-unprotected instructions due to blacklist entries,
  and unprotected instructions due to bugs in the instrumentation. This also
  provides a secondary benefit in allowing users to see the impact of each
  rule in the blacklist. Each blacklist entry that disables CFI instrumentation
  is a potential security risk, and such entries should be minimised. Users
  will be easily able to identify which blacklist entries are the cause of large
  amounts of CFI-unprotected instructions, and also be able to see if their rule
  has affected more targets than intended at a glance.

- There are operations in the source code that are implemented by the compiler
  using indirect CF instructions. The most ubiquitous of which is the simple
  ``switch`` statement. If the operand range checked by the switch statement is
  dense, it's implemented using a range-checked indirect jump. Currently this is
  recorded as a CFI-unprotected instruction, even though they're properly range
  checked. Further analysis needs to be done to enumerate all common sources of
  indirect CF instructions resultant from normal compiler behaviour, as it's
  unknown whether these should be instrumented by CFI.
