struct Structure {
  int MyCField;
};

void testMember(struct Structure *x) {
  x->MyField = 0;
}

@interface MyInterface

@property int MyProperty;

- (int)MyImplicit;
- (void)setMyImplicit:(int)x;

@property (class) int MyClassProperty;

- (void)MyMethod;

@end

@implementation MyInterface {
  int MyIVar;
}

- (void) method {
  MyiVar = 0;
  self->MyiVar = 1;

  self.MyProprty = 0;
  int x = self.MyProprty;

  int y = self.MyImplict;
  self.MyImplict = 2;

  MyInterface.MyClassProprty = 0;
  int z = MyInterface.MyClassProprty;

  [self MyMethd];
}

@end

// RUN: c-index-test \

// RUN:  -file-refs-at=%s:2:7 \
// CHECK:      FieldDecl=MyCField:2:7 (Definition)
// CHECK-NEXT: FieldDecl=MyCField:2:7 (Definition) =[2:7 - 2:15]
// CHECK-NOT:  MemberRefExpr=MyCField

// RUN:  -file-refs-at=%s:23:7 \
// CHECK-NEXT: ObjCIvarDecl=MyIVar:23:7 (Definition)
// CHECK-NEXT: ObjCIvarDecl=MyIVar:23:7 (Definition) =[23:7 - 23:13]
// CHECK-NOT:  MemberRefExpr=MyIVar

// RUN:  -file-refs-at=%s:11:15 \
// CHECK-NEXT: ObjCPropertyDecl=MyProperty:11:15
// CHECK-NEXT: ObjCPropertyDecl=MyProperty:11:15 =[11:15 - 11:25]
// CHECK-NOT:  MemberRefExpr=MyProperty

// RUN:  -file-refs-at=%s:13:8 \
// CHECK-NEXT: ObjCInstanceMethodDecl=MyImplicit:13:8
// CHECK-NEXT: ObjCInstanceMethodDecl=MyImplicit:13:8 =[13:8 - 13:18]
// CHECK-NOT:  MyImplicit

// RUN:  -file-refs-at=%s:16:23 \
// CHECK-NEXT: ObjCPropertyDecl=MyClassProperty:16:23 [class,]
// CHECK-NEXT: ObjCPropertyDecl=MyClassProperty:16:23 [class,] =[16:23 - 16:38]
// CHECK-NOT:  MyClassProperty

// RUN:  -file-refs-at=%s:18:9 \
// CHECK-NEXT: ObjCInstanceMethodDecl=MyMethod:18:9
// CHECK-NEXT: ObjCInstanceMethodDecl=MyMethod:18:9 =[18:9 - 18:17]
// CHECK-NOT:  MyMethod

// RUN:   %s -fspell-checking | FileCheck %s
