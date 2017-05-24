.. title:: clang-tidy - misc-default-numerics

misc-default-numerics
=====================

This check flags usages of ``std::numeric_limits<T>::{min,max}()`` for
unspecialized types. It is dangerous because returns T(), which might is rarely
minimum or maximum for this type.

Consider scenario:
.. code-block:: c++

  // 1. Have a:
  typedef long long BigInt

  // 2. Use
  std::numeric_limits<BigInt>::min()


  // 3. Replace the BigInt typedef with class implementing BigIntegers
  class BigInt { ;;; };

  // 4. Your code compiles silently and you a few years later you find an
  // of by 9223372036854775808 error.
