.. title:: clang-tidy - bugprone-misplaced-operator-in-strlen-in-alloc

bugprone-misplaced-operator-in-strlen-in-alloc
==============================================

Finds cases a value is added to or subtracted from the string in the parameter
of ``strlen()`` method instead of to the result and use its return value as an
argument of a memory allocation function (``malloc()``, ``calloc()``,
``realloc()``).

Example code:

.. code-block:: c

    void bad_malloc(char *str) {
      char *c = (char*) malloc(strlen(str + 1));
    }


The suggested fix is to add value to the return value of ``strlen()`` and not
to its argument. In the example above the fix would be

.. code-block:: c

      char *c = (char*) malloc(strlen(str) + 1);

