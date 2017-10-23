// RUN: %clang_cc1 -triple x86_64-apple-darwin -Wno-objc-root-class -fobjc-arc -emit-llvm -o - %s | FileCheck %s

// rdar://problem/21054495
typedef char FlexibleArray[];

@interface FlexibleArrayMember {
  FlexibleArray flexible_array;
}
@end
@implementation FlexibleArrayMember
@end
// CHECK: @OBJC_METH_VAR_NAME_{{.*}} = private unnamed_addr constant {{.*}} c"flexible_array\00"
// CHECK-NEXT: @OBJC_METH_VAR_TYPE_{{.*}} = private unnamed_addr constant {{.*}} c"^c\00"


struct Packet {
  int size;
  FlexibleArray data;
};

@interface VariableSizeIvar {
  struct Packet flexible_struct;
}
@end
@implementation VariableSizeIvar
@end
// CHECK: @OBJC_METH_VAR_NAME_{{.*}} = private unnamed_addr constant {{.*}} c"flexible_struct\00"
// CHECK-NEXT: @OBJC_METH_VAR_TYPE_{{.*}} = private unnamed_addr constant {{.*}} c"{Packet=\22size\22i\22data\22[0c]}\00"
