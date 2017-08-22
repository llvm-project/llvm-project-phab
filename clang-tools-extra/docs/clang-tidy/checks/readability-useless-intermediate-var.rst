.. title:: clang-tidy - readability-useless-intermediate-var

readability-useless-intermediate-var
====================================

Detects useless intermediate variables before return statements returning the
result of a simple comparison. This checker also suggests to directly inline the
initializer expression of the variable declaration into the return expression.

.. code-block:: c++

  // the checker detects

  auto test = 1;
  return (test == 2);

  // and suggests to fix it into

  return (1 == 2);
