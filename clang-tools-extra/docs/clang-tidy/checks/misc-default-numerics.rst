.. title:: clang-tidy - misc-default-numerics

misc-default-numerics
=====================

This check flags usages of ``std::numeric_limits<T>::{min,max}()`` for
unspecialized types. It is dangerous because the calls return ``T()``
in this case, which is unlikely to represent the minimum or maximum value for
the type.

Consider scenario:
.. code-block:: c++

  // 1. Have a:
  typedef long long BigInt

  // 2. Use
  std::numeric_limits<BigInt>::min()

  // 3. Replace the BigInt typedef with class implementing BigIntegers
  class BigInt { ;;; };

  // 4. Your code continues to compile, but the call to min() returns BigInt{}
