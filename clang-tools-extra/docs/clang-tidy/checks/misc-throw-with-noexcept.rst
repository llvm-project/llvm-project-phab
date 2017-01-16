.. title:: clang-tidy - misc-throw-with-noexcept

mist-throw-with-noexcept
========================

This check finds cases of using ``throw`` in a function declared as noexcept.
Please note that the warning is issued even if the exception is caught within
the same function, as that would be probably a bad style anyway.

It removes the noexcept specifier as a fix.


  .. code-block:: c++

    void f() noexcept {
    	throw 42;
    }

    // Will be changed to
    void f() {
    	throw 42;
    }
