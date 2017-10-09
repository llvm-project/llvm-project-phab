struct Structure {};

Structure *test555() {
  Structure *myOwnStringOrig = 0;
  return myOwnStringNew;
}

class AClass {
public:
  int MyField;
  void MyMethod();

  void anotherMethod();

  AClass() : MyFild(0) {}
};

void AClass::anotherMethod() {
  MYField = 0;
  MYMethod();
}

void refMember(AClass *C) {
  C->MyFild = 1;
  C->MyMethd();
}

// RUN: c-index-test \

// RUN:  -file-refs-at=%s:4:14 \
// CHECK:      VarDecl=myOwnStringOrig:4:14 (Definition)
// CHECK-NEXT: VarDecl=myOwnStringOrig:4:14 (Definition) =[4:14 - 4:29]
// CHECK-NOT:  DeclRefExpr=myOwnStringOrig

// RUN:  -file-refs-at=%s:10:7 \
// CHECK-NEXT: FieldDecl=MyField:10:7 (Definition)
// CHECK-NEXT: FieldDecl=MyField:10:7 (Definition) =[10:7 - 10:14]
// CHECK-NOT:  =MyField

// RUN:  -file-refs-at=%s:11:8 \
// CHECK-NEXT: CXXMethod=MyMethod:11:8
// CHECK-NEXT: CXXMethod=MyMethod:11:8 =[11:8 - 11:16]
// CHECK-NOT:  MemberRefExpr=MyMethod

// RUN:   %s -fspell-checking | FileCheck %s
