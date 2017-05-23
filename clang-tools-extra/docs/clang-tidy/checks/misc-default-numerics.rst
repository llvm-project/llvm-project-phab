.. title:: clang-tidy - misc-default-numerics

misc-default-numerics
=====================

This check flags usages of ``std::numeric_limits<T>::{min,max}()`` for
unspecialized types. It is dangerous because returns T(), which might is rarely
minimum or maximum for this type.

Consider scenario:
1. Have `typedef long long BigInt` in source code
2. Use `std::numeric_limits<BigInt>::min()`
3. Replace the `BigInt` typedef with class implementing BigIntegers
4. Compile silently your code and find it few years later that you have of by
9223372036854775808 error.
