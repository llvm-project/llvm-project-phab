.. title:: clang-tidy - bugprone-misplaced-operator-in-strlen-in-alloc

bugprone-misplaced-operator-in-strlen-in-alloc
==============================================

Finds cases where ``1`` is added to the string in the parameter of ``strlen()``,
``strnlen()``, ``strnlen_s()``, ``wcslen()``, ``wcsnlen()`` and ``wcsnlen_s()``
functions instead of to the result and use its return value as an argument of a
memory allocation function (``malloc()``, ``calloc()``, ``realloc()``,
``alloca()``) or the ``new[]`` operator in `C++`. Cases where ``1`` is added
both to the parameter and the result of the ``strlen()``-like function are
ignored, as are cases where the whole addition is surrounded by extra
parentheses.

`C` example code:

.. code-block:: c

    void bad_malloc(char *str) {
      char *c = (char*) malloc(strlen(str + 1));
    }


The suggested fix is to add ``1`` to the return value of ``strlen()`` and not
to its argument. In the example above the fix would be

.. code-block:: c

      char *c = (char*) malloc(strlen(str) + 1);


`C++` example code:

.. code-block:: c++

    void bad_new(char *str) {
      char *c = new char[strlen(str + 1)];
    }


As in the `C` code with the ``malloc()`` function, the suggested fix is to
add ``1`` to the return value of ``strlen()`` and not to its argument. In the
example above the fix would be

.. code-block:: c++

      char *c = new char[strlen(str) + 1];


Example for silencing the bug report:

.. code-block:: c

    void bad_malloc(char *str) {
      char *c = (char*) malloc(strlen((str + 1)));
    }
