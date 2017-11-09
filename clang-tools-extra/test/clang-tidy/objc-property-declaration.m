// RUN: %check_clang_tidy %s objc-property-declaration %t

@interface Foo
@property(assign, nonatomic) int NotCamelCase;
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: property 'NotCamelCase' is not in proper format according to property naming convention [objc-property-declaration]
// CHECK-FIXES: @property(assign, nonatomic) int notCamelCase;
@property(assign, nonatomic) int camelCase;
// CHECK-MESSAGES-NOT: :[[@LINE-1]]:34: warning: property 'camelCase' is not in proper format according to property naming convention [objc-property-declaration]
@property(assign, nonatomic) int _WithPrefix;
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: property '_WithPrefix' is not in proper format according to property naming convention [objc-property-declaration]
// CHECK-FIXES: @property(assign, nonatomic) int withPrefix;
@end
