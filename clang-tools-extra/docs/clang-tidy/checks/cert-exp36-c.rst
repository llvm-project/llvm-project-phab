.. title:: clang-tidy - cert-exp36-c

cert-exp36-c
============

This check will give a warning if a pointer value is converted to
a pointer type that is more strictly aligned than the referenced type.
 
 Here's an example:
 
 .. code-block:: c
 
    char c = 'x';
    int *ip = (int *)&c;
    // warning: do not cast pointers into more strictly aligned pointer types
 
 This check does not completely include warnings for types with explicitly
 specified alignment, this remains a possible future extension.

 See the example:

  .. code-block:: c

    // Works fine:
    struct x {
      _Alignas(int) char c;
    };

    void function3(void) {
      struct x c = {'x'};
      int *ip = (int *)&c;
    }

    // Won't work:
    void function4(void) {
      _Alignas(int) char c = 'x';
      int *ip = (int *)&c;
      // the check will give a warning for this
    }

 This check corresponds to the CERT C Coding Standard rule
 `EXP36-C. Do not cast pointers into more strictly aligned pointer types
<https://www.securecoding.cert.org/confluence/display/c/EXP36-C.+Do+not+cast+pointers+into+more+strictly+aligned+pointer+types>`_.
