// RUN: %clang_cc1 -triple x86_64-apple-darwin9.0.0 -fsyntax-only -verify -Wno-objc-root-class %s

// Warn about availability attribute when they're specified in the definition
// of the method instead of the declaration.
// rdar://15540962

@interface MissingAvailabilityThingsInInterface

- (void)missingIDO; // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}

- (void)missingDO __attribute__((availability(macos, introduced=10.1))); // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}

- (void)missingD __attribute__((availability(macos, introduced=10.1))); // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}

- (void)missingIx2; // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}
// expected-warning@-1 {{method declaration is missing an availability attribute for iOS that is specified in the definition}}

- (void)missingIDOiOS __attribute__((availability(macos, introduced=10.1))); // expected-warning {{method declaration is missing an availability attribute for iOS that is specified in the definition}}

- (void)differentIMissingD __attribute__((availability(macos, introduced=10.1))); // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}
// expected-note@-1 {{previous attribute is here}}

- (void)missingUnavailable; // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}

- (void)same
__attribute__((availability(macos, introduced=10.1)))
__attribute__((availability(ios, unavailable)));

- (void)missingDeprecatedAttr; // expected-warning {{method declaration is missing a deprecated attribute that is specified in the definition}}

- (void)sameDeprecatedAttr __attribute__((deprecated("y")));

@end

@interface MissingAvailabilityThingsInInterface()

- (void)missingInClassExtension; // expected-warning {{method declaration is missing an availability attribute for macOS that is specified in the definition}}

@end

@implementation MissingAvailabilityThingsInInterface

- (void)missingIDO
__attribute__((availability(macos, introduced=10.1, deprecated=10.2, obsoleted=10.3))) // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)missingDO
__attribute__((availability(macos, introduced=10.1)))
__attribute__((availability(macos, deprecated=10.2, obsoleted=10.3))) // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)missingD
__attribute__((availability(macos, introduced=10.1, deprecated=10.2))) // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)missingIx2
__attribute__((availability(ios, introduced=10))) // expected-note {{definition with iOS availability attribute is here}}
__attribute__((availability(macos, introduced=10.1))) // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)missingIDOiOS
__attribute__((availability(ios, introduced=10, deprecated=11, obsoleted=11.1))) // expected-note {{definition with iOS availability attribute is here}}
__attribute__((availability(macOS, introduced=10.1)))
{ }

- (void)differentIMissingD __attribute__((availability(macos, introduced=10.2, deprecated=10.3))) // expected-note {{definition with macOS availability attribute is here}}
{ } // expected-warning@-1{{availability does not match previous declaration}}

- (void)missingInClassExtension
__attribute__((availability(macos, introduced=10.1, deprecated=10.2))) // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)missingUnavailable
__attribute__((availability(macos, unavailable))); // expected-note {{definition with macOS availability attribute is here}}
{ }

- (void)same
__attribute__((availability(macos, introduced=10.1)))
__attribute__((availability(ios, unavailable)))
{ }

- (void)missingDeprecatedAttr
__attribute__((deprecated("x")))  // expected-note {{definition with deprecated attribute is here}}
{ }

- (void)sameDeprecatedAttr __attribute__((deprecated("y")))
{ }

@end

@interface MissingAvailabilityThingsInInterface (Category)

- (void)missingInCategory; // expected-warning {{method declaration is missing an availability attribute for tvOS that is specified in the definition}}

@end

@implementation MissingAvailabilityThingsInInterface (Category)

- (void)missingInCategory
__attribute__((availability(tvOS, introduced=10))) // expected-note {{definition with tvOS availability attribute is here}}
{ }

@end

@interface DontWarnOnOverrideImpl

- (void)missingIDO
__attribute__((availability(macos, introduced=10.1, deprecated=10.2, obsoleted=10.3))); // ok

@end

@implementation DontWarnOnOverrideImpl

- (void)missingIDO { }

// ok
- (void)missingDO
__attribute__((availability(macos, introduced=10.1)))
__attribute__((availability(macos, deprecated=10.2, obsoleted=10.3)))
{ }

@end

@protocol DontWarnOnProtocol

- (void)missingIDO;

@end

@interface DontWarnOnProtocolImpl<DontWarnOnProtocol>
@end

@implementation DontWarnOnProtocolImpl

- (void)missingIDO
__attribute__((availability(macos, introduced=10.1, deprecated=10.2, obsoleted=10.3)))
{}

@end
