.. title:: clang-tidy - misc-throw-with-noexcept

misc-throw-with-noexcept
========================

This check finds cases of using ``throw`` in a function declared
with a non-throwing exception specification.

Please note that the warning is issued even if the exception is caught within
the same function, as that would be probably a bad style anyway.

It removes the exception specification as a fix.


  .. code-block:: c++

    void f() noexcept {
    	throw 42;
    }

    // Will be changed to
    void f() {
    	throw 42;
    }
