// RUN: %check_clang_tidy %s objc-property-declaration %t \
// RUN: -config='{CheckOptions: \
// RUN:  [{key: objc-property-declaration.Acronyms, value: "ABC;TGIF"}]}' \
// RUN: --

@interface Foo
@property(assign, nonatomic) int AbcNotRealPrefix;
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: property 'AbcNotRealPrefix' is not in proper format according to property naming convention. It should be in the format of lowerCamelCase or has special acronyms [objc-property-declaration]
// CHECK-FIXES: @property(assign, nonatomic) int abcNotRealPrefix;
@property(assign, nonatomic) int ABCCustomPrefix;
@end
