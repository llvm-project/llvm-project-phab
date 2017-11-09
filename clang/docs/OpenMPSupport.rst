.. raw:: html

  <style type="text/css">
    .none { background-color: #FFCCCC }
    .partial { background-color: #FFFF99 }
    .good { background-color: #CCFF99 }
  </style>

.. role:: none
.. role:: partial
.. role:: good

==================
OpenMP Support
==================

Clang fully supports OpenMP 3.5 + some elements of OpenMP 4.5. Clang supports offloading to X86_64, AArch64, PPC64[LE] and Cuda devices.
The status of major OpenMP 4.5 features support in Clang.

Standalone directives
=====================

* #pragma omp [for] simd: :good:`Complete`.

* #pragma omp declare simd: :partial:`Partial`.  We support parsing/semantic
  analysis + generation of special attributes for X86 target, but still
  missing the LLVM pass for vectorization.

* #pragma omp taskloop [simd]: :good:`Complete`.

* #pragma omp target [enter|exit] data: :good:`Mostly complete`.  Some rework is
  required for better stability.

* #pragma omp target update: :good:`Mostly complete`.  Some rework is
  required for better stability.

* #pragma omp target: :partial:`Partial`.  No support for the `reduction`,
  `nowait` and `depend` clauses.

* #pragma omp declare target: :partial:`Partial`.  No full codegen support.

* #pragma omp teams: :good:`Complete`.

* #pragma omp distribute [simd]: :good:`Complete`.

* #pragma omp distribute parallel for [simd]: :partial:`Partial`. No full codegen support.

Combined directives
===================

* #pragma omp parallel for simd: :good:`Complete`.

* #pragma omp target parallel: :good:`Complete`.

* #pragma omp target parallel for [simd]: :good:`Complete`.

* #pragma omp target simd: :partial:`Partial`.  No full codegen support.

* #pragma omp target teams: :partial:`Partial`.  No full codegen support.

* #pragma omp teams distribute [simd]: :partial:`Partial`.  No full codegen support.

* #pragma omp target teams distribute [simd]: :partial:`Partial`.  No full codegen support.

* #pragma omp teams distribute parallel for [simd]: :partial:`Partial`.  No full codegen support.

* #pragma omp target teams distribute parallel for [simd]: :partial:`Partial`.  No full codegen support.

Clang does not support any constructs/updates from upcoming OpenMP 5.0 except for `reduction`-based clauses in the `task`-based directives.

