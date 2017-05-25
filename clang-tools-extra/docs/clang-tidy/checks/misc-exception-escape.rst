.. title:: clang-tidy - misc-exception-escape

misc-exception-escape
=====================

Finds functions which should not throw exceptions:
+ Destructors
+ Copy constructors
+ Copy assignment operators
+ The main() functions
+ swap() functions
+ Functions marked with throw() or noexcept
+ Other functions given as option

A destructor throwing an exception may result in undefined behavior, resource
leaks or unexpected termination of the program. Throwing move constructor or
move assignment also may result in undefined behavior or resource leak. Swap
operations expected to be non throwing most of the cases and they are always
possible to implement in a non throwing way. Non throwing ``swap()`` operations
are also used to create move operations. A throwing ``main()`` function also
results in unexpected termination.

Options
-------

.. option:: EnabledFunctions

   Comma separated list containing function names which should not throw. An
   example for using this parameter is the function WinMain in the Windows API.
   Default vale is empty string.

.. option:: IgnoredExceptions

   Comma separated list containing type names which are not counted as thrown
   exceptions in the checker. Default value is empty string.
