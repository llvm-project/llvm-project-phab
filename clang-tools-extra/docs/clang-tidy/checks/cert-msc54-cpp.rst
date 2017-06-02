.. title:: clang-tidy - cert-msc54-cpp

cert-msc54-cpp
==============

This check will give a warning if a signal handler is not defined
as an 'extern C' function or if the declaration of the function
contains C++ representation.

Here's an example:
  
 .. code-block:: c++

    static void sig_handler(int sig) {}
    // warning: use 'external C' prefix for signal handlers

    void install_signal_handler() {
    if (SIG_ERR == std::signal(SIGTERM, sig_handler))
      return;
    }


    extern "C" void cpp_signal_handler(int sig) {
    //warning: do not use C++ representations in signal handlers
      throw "error message";
    }

    void install_cpp_signal_handler() {
    if (SIG_ERR == std::signal(SIGTERM, cpp_signal_handler))
      return;
    }


This check corresponds to the CERT CPP Coding Standard rule
`MSC54-CPP. A signal handler must be a plain old function
<https://www.securecoding.cert.org/confluence/display/cplusplus/MSC54-CPP.+A+signal+handler+must+be+a+plain+old+function>`_.
