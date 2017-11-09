.. title:: clang-tidy - objc-property-declaration

objc-property-declaration
=========================

Finds property declarations in Objective-C files that do not follow the pattern
of property names in Apple's programming guide. The property name should be
in the format of Lower Camel Case.

For code:

.. code-block:: objc

@property(nonatomic, assign) int LowerCamelCase;

The fix will be:

.. code-block:: objc

@property(nonatomic, assign) int lowerCamelCase;

The check will do best effort to give a fix, however, in some cases it is
difficult to give a proper fix since the property name could be complicated.
Users will need to come up with a proper name by their own.

The corresponding style rule: https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/CodingGuidelines/Articles/NamingIvarsAndTypes.html#//apple_ref/doc/uid/20001284-1001757
