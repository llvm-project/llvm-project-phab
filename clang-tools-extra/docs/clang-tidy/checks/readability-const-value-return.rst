.. title:: clang-tidy - readability-const-value-return

readability-const-value-return
==============================

Checks for functions that return a ``const``-qualified value type. That use of
``const`` is superfluous, and prevents moving the object. Therefore, it should
be avoided.

Examples:

.. code-block:: c++

  const Foo f_bad();
  Foo f_good();

  Foo foo;
  foo = f_bad();   // This requires a copy.
  foo = f_good();  // This can use a move.


Note that this does not apply to returning pointers or references to const
objects:

.. code-block:: c++

  const Foo* f_ptr();  // No issue.
  const Foo& f_ref();  // No issue.
