// RUN: %check_clang_tidy %s objc-property-declaration %t
@class NSString;

@interface Foo
@property(assign, nonatomic) int NotCamelCase;
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: property 'NotCamelCase' is not in proper format according to property naming convention. It should be in the format of lowerCamelCase or has special acronyms [objc-property-declaration]
// CHECK-FIXES: @property(assign, nonatomic) int notCamelCase;
@property(assign, nonatomic) int camelCase;
@property(strong, nonatomic) NSString *URLString;
@end
