.. title:: clang-tidy - cert-dcl51-cpp

cert-dcl51-cpp
==============

This check will catch the following errors due to these are
names declared or defined as a reserved identifier:
    - Names in the global namespace should not start with an underscore.
    - Names should not start with an underscore followed by an uppercase letter.
    - Names should not contain a double underscore.
    - Names defined in macros should not be identical to keywords.

Here's an example using variables:

.. code-block:: c++

    #define override
    // warning: macro name should not be identical to keyword

    int _global_variable;
    // warning: global name starts with an underscore

    void function() {
        int loval__variable;
        // warning: name contains a double underscore
    
        int _Uppercase_variable;
        // warning: name starts with an underscore followed by an uppercase letter
    }

This check does not include warnings for reserved macro names and 
user defined literals, since checks have already been written to those:
- Wreserved-id-macro
- Wuser-defined-literals

This check corresponds to the CERT C++ Coding Standard rule
`DCL51-CPP. Do not declare or define a reserved identifier
<https://www.securecoding.cert.org/confluence/display/cplusplus/DCL51-CPP.+Do+not+declare+or+define+a+reserved+identifier>`_.

