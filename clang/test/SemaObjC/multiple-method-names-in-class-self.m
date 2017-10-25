// RUN: %clang_cc1 -Wobjc-multiple-method-names -x objective-c -verify %s
// RUN: %clang_cc1 -Wobjc-multiple-method-names -x objective-c -verify -fobjc-arc -DARC %s
#ifdef ARC
// expected-no-diagnostics
#endif

@interface NSObj

+ (instancetype) alloc;

+ (_Nonnull instancetype) globalObject;

@end

@interface SelfAllocReturn: NSObj

- (instancetype)initWithFoo:(int)x;
#ifndef ARC
// expected-note@-2 2 {{using}}
#endif

@end

@interface SelfAllocReturn2: NSObj

- (instancetype)initWithFoo:(SelfAllocReturn *)x;
#ifndef ARC
// expected-note@-2 2 {{also found}}
#endif

@end

@implementation SelfAllocReturn

- (instancetype)initWithFoo:(int)x {
    return self;
}

+ (instancetype) thingWithFoo:(int)x {
    return [[self alloc] initWithFoo: x];
#ifndef ARC
// expected-warning@-2 {{multiple methods named 'initWithFoo:' found}}
#endif
}

+ (void) initGlobal {
  (void)[[self globalObject] initWithFoo: 20];
#ifndef ARC
// expected-warning@-2 {{multiple methods named 'initWithFoo:' found}}
#endif
}

@end
