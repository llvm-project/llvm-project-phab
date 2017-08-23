.. title:: clang-tidy - readability-unnecessary-intermediate-var

readability-unnecessary-intermediate-var
====================================

Detects unnecessary intermediate variables before `return` statements returning the
result of a simple comparison. This checker also suggests to directly inline the
initializer expression of the variable declaration into the `return` expression.

Example:
.. code-block:: c++

  // the checker detects

  auto test = foo();
  return (test == MY_CONST);

  // and suggests to fix it into

  return (foo() == MY_CONST);
