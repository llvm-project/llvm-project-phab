.. title:: clang-tidy - readability-redundant-keyword

readability-redundant-keyword
=============================

This checker removes the redundant `extern` and `inline` keywords from code.

`extern` is redundant in function declarations

.. code-block:: c++

  extern void h();


`inline` is redundant in function definitions within class declaration.

.. code-block:: c++

  class X {
    inline int f() { .... }
  };