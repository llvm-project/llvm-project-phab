.. title:: clang-tidy - bugprone-misplaced-operator-in-strlen-in-alloc

bugprone-misplaced-operator-in-strlen-in-alloc
==============================================

Finds cases where ``1`` is added to or subtracted from the string in the
parameter of ``strlen()``, ``strnlen()``, ``strnlen_s()``, ``wcslen()``,
``wcsnlen()`` and ``wcsnlen_s()`` functions instead of to the result and use its
return value as an argument of a memory allocation function (``malloc()``,
``calloc()``, ``realloc()``, ``alloca()``). Cases where ``1`` is added both to
the parameter and the result of the ``strlen()``-like function are ignored,
similarily to cases where the whole addition is surrounded by extra parentheses.

Example code:

.. code-block:: c

    void bad_malloc(char *str) {
      char *c = (char*) malloc(strlen(str + 1));
    }


The suggested fix is to add ``1`` to the return value of ``strlen()`` and not
to its argument. In the example above the fix would be

.. code-block:: c

      char *c = (char*) malloc(strlen(str) + 1);

