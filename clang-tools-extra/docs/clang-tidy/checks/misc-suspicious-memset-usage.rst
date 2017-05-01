.. title:: clang-tidy - misc-suspicious-memset-usage

misc-suspicious-memset-usage
============================

This check finds memset calls with potential mistakes in their arguments.
Considering the function as ``void* memset(void* destination, int fill_value,
size_t byte_count)``, the following cases are covered:

**Case 1: Fill value is a character '0'**

Filling up a memory area with ASCII code 48 characters is not customary,
possibly integer zeroes were intended instead.
The check offers a replacement of ``'0'`` with ``0``. Memsetting character
pointers with ``'0'`` is allowed.

**Case 2: Fill value is truncated**

Memset converts ``fill_value`` to ``unsigned char`` before using it. If
``fill_value`` is out of unsigned character range, it gets truncated
and memory will not contain the desired pattern.

**Case 3: Destination is a this pointer**

If the class containing the memset call has a virtual function, using
memset on the ``this`` pointer might corrupt the virtual method table.
Inner structs form an exception.

Examples:

.. code-block:: c++

  void SuspiciousFillValue() {

    int i[5] = {1, 2, 3, 4, 5};
    int *ip = i;
    int l = 5;
    char c = '1';
    char *cp = &z;

    // Case 1
    memset(ip, '0', l); // suspicious
    memset(cp, '0', 1); // OK

    // Case 2
    memset(ip, 0xabcd, l); // fill value gets truncated
    memset(ip, 0x00, l);   // OK

  }

  // Case 3
  class WithVirtualFunction() {
    virtual void f() {}
    WithVirtualFunction() {
      memset(this, 0, sizeof(*this)); // might reset virtual pointer
    }
  }
