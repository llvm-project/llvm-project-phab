// RUN: c-index-test -test-annotate-tokens=%s:1:1:33:1 %s -fspell-checking | FileCheck %s

struct Structure {};

Structure *test555() {
  Structure *myOwnStringOrig = 0;
  return myOwnStringNew; // CHECK: Identifier: "myOwnStringNew" {{\[}}[[@LINE]]:10 - [[@LINE]]:24{{\]}} ReturnStmt=
}

class AClass {
public:
  int MyField;
  void MyMethod();

  void anotherMethod();

  AClass() : MyFild(0) {} // CHECK: Identifier: "MyFild" {{\[}}[[@LINE]]:14 - [[@LINE]]:20{{\]}} CXXConstructor=AClass
};

void AClass::anotherMethod() {
  MYField = 0; // CHECK: Identifier: "MYField" {{\[}}[[@LINE]]:3 - [[@LINE]]:10{{\]}} BinaryOperator=
  MYMethod();  // CHECK: Identifier: "MYMethod" {{\[}}[[@LINE]]:3 - [[@LINE]]:11{{\]}} CallExpr=MyMethod
}

@interface MyInterface

@property int MyProperty;

@end

void foo(MyInterface *I) {
  I.MyProprty = 0; // CHECK: Identifier: "MyProprty" {{\[}}[[@LINE]]:5 - [[@LINE]]:14{{\]}} BinaryOperator=
}
