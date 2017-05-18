.. title:: clang-tidy - misc-throw-with-noexcept

misc-throw-with-noexcept
========================

This check finds cases of using ``throw`` in a function declared
with a non-throwing exception specification.

The warning is *not* issued when the exception is caught in the same function.

It removes the exception specification as a fix.


  .. code-block:: c++

    void f() noexcept {
    	throw 42;
    }

    // Will be changed to
    void f() {
    	throw 42;
    }

    void f() noexcept {
      try {
        throw 42; // perfectly ok
      } catch(int x) {
      }
    }
