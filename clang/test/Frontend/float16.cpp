// RUN: %clang_cc1 -std=c++11 -ast-dump %s | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -ast-dump -fnative-half-type %s | FileCheck %s --check-prefix=CHECK-NATIVE

/*  Various contexts where type _Float16 can appear. */

/*  Namespace */
namespace {
  _Float16 f1n;
  _Float16 f2n = 33.f16;
  _Float16 arr1n[10];
  _Float16 arr2n[] = { 1.2, 3.0, 3.e4 };
  const volatile _Float16 func1n(const _Float16 &arg) {
    return arg + f2n + arr1n[4] - arr2n[1];
  }
}

//CHECK: |-NamespaceDecl
//CHECK: | |-VarDecl {{.*}} f1n '_Float16'
//CHECK: | |-VarDecl {{.*}} f2n '_Float16' cinit
//CHECK: | | `-FloatingLiteral {{.*}} '_Float16' 3.300000e+01
//CHECK: | |-VarDecl {{.*}} arr1n '_Float16 [10]'
//CHECK: | |-VarDecl {{.*}} arr2n '_Float16 [3]' cinit
//CHECK: | | `-InitListExpr {{.*}} '_Float16 [3]'
//CHECK: | |   |-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: | |   | `-FloatingLiteral {{.*}} 'double' 1.200000e+00
//CHECK: | |   |-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: | |   | `-FloatingLiteral {{.*}} 'double' 3.000000e+00
//CHECK: | |   `-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: | |     `-FloatingLiteral {{.*}} 'double' 3.000000e+04
//CHECK: | `-FunctionDecl {{.*}} func1n 'const volatile _Float16 (const _Float16 &)'

/* File */
_Float16 f1f;
_Float16 f2f = 32.4;
_Float16 arr1f[10];
_Float16 arr2f[] = { -1.2, -3.0, -3.e4 };
_Float16 func1f(_Float16 arg);

//CHECK: |-VarDecl {{.*}} f1f '_Float16'
//CHECK: |-VarDecl {{.*}} f2f '_Float16' cinit
//CHECK: | `-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: |   `-FloatingLiteral {{.*}} 'double' 3.240000e+01
//CHECK: |-VarDecl {{.*}} arr1f '_Float16 [10]'
//CHECK: |-VarDecl {{.*}} arr2f '_Float16 [3]' cinit
//CHECK: | `-InitListExpr {{.*}} '_Float16 [3]'
//CHECK: |   |-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: |   | `-UnaryOperator {{.*}} 'double' prefix '-'
//CHECK: |   |   `-FloatingLiteral {{.*}} 'double' 1.200000e+00
//CHECK: |   |-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: |   | `-UnaryOperator {{.*}} 'double' prefix '-'
//CHECK: |   |   `-FloatingLiteral {{.*}} 'double' 3.000000e+00
//CHECK: |   `-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK: |     `-UnaryOperator {{.*}} 'double' prefix '-'
//CHECK: |       `-FloatingLiteral {{.*}} 'double' 3.000000e+04
//CHECK: |-FunctionDecl {{.*}} func1f '_Float16 (_Float16)'
//CHECK: | `-ParmVarDecl {{.*}} arg '_Float16'


// Mixing __fp16 and Float16 types:
// The _Float16 type is first converted to __fp16 type and then the operation
// is completed as if both operands were of __fp16 type.

__fp16 B = -0.1;
auto C = -1.0f16 + B;

// When we do *not* have native half types, we expect __fp16 to be promoted to
// float, and consequently also _Float16 promotions to float:

//CHECK: -VarDecl {{.*}} used B '__fp16' cinit
//CHECK: | `-ImplicitCastExpr {{.*}} '__fp16' <FloatingCast>
//CHECK: |   `-UnaryOperator {{.*}} 'double' prefix '-'
//CHECK: |     `-FloatingLiteral {{.*}} 'double' 1.000000e-01
//CHECK: |-VarDecl {{.*}} C 'float':'float' cinit
//CHECK: | `-BinaryOperator {{.*}} 'float' '+'
//CHECK: |   |-ImplicitCastExpr {{.*}} 'float' <FloatingCast>
//CHECK: |   | `-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK: |   |   `-FloatingLiteral {{.*}} '_Float16' 1.000000e+00
//CHECK: |   `-ImplicitCastExpr {{.*}} 'float' <FloatingCast>
//CHECK: |     `-ImplicitCastExpr {{.*}} '__fp16' <LValueToRValue>
//CHECK: |       `-DeclRefExpr {{.*}} '__fp16' lvalue Var 0x{{.*}} 'B' '__fp16'

// When do have native half types, we expect to see promotions XXXXX:

//CHECK-NATIVE: |-VarDecl {{.*}} used B '__fp16' cinit
//CHECK-NATIVE: | `-ImplicitCastExpr {{.*}} '__fp16' <FloatingCast>
//CHECK-NATIVE: |   `-UnaryOperator {{.*}} 'double' prefix '-'
//CHECK-NATIVE: |     `-FloatingLiteral {{.*}} 'double' 1.000000e-01
//CHECK-NATIVE: |-VarDecl {{.*}} C '__fp16':'__fp16' cinit
//CHECK-NATIVE: | `-BinaryOperator {{.*}} '__fp16' '+'
//CHECK-NATIVE: |   |-ImplicitCastExpr {{.*}} '__fp16' <FloatingCast>
//CHECK-NATIVE: |   | `-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK-NATIVE: |   |   `-FloatingLiteral {{.*}} '_Float16' 1.000000e+00
//CHECK-NATIVE: |   `-ImplicitCastExpr {{.*}} '__fp16' <LValueToRValue>
//CHECK-NATIVE: |     `-DeclRefExpr {{.*}} '__fp16' lvalue Var 0x{{.*}} 'B' '__fp16'


/* Class */

class C1 {
  _Float16 f1c;
  static const _Float16 f2c;
  volatile _Float16 f3c;
public:
  C1(_Float16 arg) : f1c(arg), f3c(arg) { }
  _Float16 func1c(_Float16 arg ) {
    return f1c + arg;
  }
  static _Float16 func2c(_Float16 arg) {
    return arg * C1::f2c;
  }
};

//CHECK: |-CXXRecordDecl {{.*}} referenced class C1 definition
//CHECK: | |-CXXRecordDecl {{.*}} implicit referenced class C1
//CHECK: | |-FieldDecl {{.*}} referenced f1c '_Float16'
//CHECK: | |-VarDecl {{.*}} used f2c 'const _Float16' static
//CHECK: | |-FieldDecl {{.*}} f3c 'volatile _Float16'
//CHECK: | |-AccessSpecDecl
//CHECK: | |-CXXConstructorDecl {{.*}} used C1 'void (_Float16)'
//CHECK: | | |-ParmVarDecl {{.*}} used arg '_Float16'
//CHECK: | | |-CXXCtorInitializer Field {{.*}} 'f1c' '_Float16'
//CHECK: | | | `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | | |   `-DeclRefExpr {{.*}} '_Float16' lvalue ParmVar 0x{{.*}} 'arg' '_Float16'
//CHECK: | | |-CXXCtorInitializer Field {{.*}} 'f3c' 'volatile _Float16'
//CHECK: | | | `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | | |   `-DeclRefExpr {{.*}} '_Float16' lvalue ParmVar 0x{{.*}} 'arg' '_Float16'
//CHECK: | | `-CompoundStmt
//CHECK: | |-CXXMethodDecl {{.*}} used func1c '_Float16 (_Float16)'
//CHECK: | | |-ParmVarDecl {{.*}} used arg '_Float16'
//CHECK: | | `-CompoundStmt
//CHECK: | |   `-ReturnStmt
//CHECK: | |     `-BinaryOperator {{.*}} '_Float16' '+'
//CHECK: | |       |-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | |       | `-MemberExpr {{.*}} '_Float16' lvalue ->f1c 0x{{.*}}
//CHECK: | |       |   `-CXXThisExpr {{.*}} 'class C1 *' this
//CHECK: | |       `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | |         `-DeclRefExpr {{.*}} '_Float16' lvalue ParmVar 0x{{.*}} 'arg' '_Float16'
//CHECK: | |-CXXMethodDecl {{.*}} used func2c '_Float16 (_Float16)' static
//CHECK: | | |-ParmVarDecl {{.*}} used arg '_Float16'
//CHECK: | | `-CompoundStmt
//CHECK: | |   `-ReturnStmt
//CHECK: | |     `-BinaryOperator {{.*}} '_Float16' '*'
//CHECK: | |       |-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | |       | `-DeclRefExpr {{.*}} '_Float16' lvalue ParmVar 0x{{.*}} 'arg' '_Float16'
//CHECK: | |       `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK: | |         `-DeclRefExpr {{.*}} 'const _Float16' lvalue Var 0x{{.*}} 'f2c' 'const _Float16'


/*  Template */

template <class C> C func1t(C arg) {
  return arg * 2.f16;
}

//CHECK: |-FunctionTemplateDecl {{.*}}  func1t
//CHECK: | |-TemplateTypeParmDecl  {{.*}} C
//CHECK: | |-FunctionDecl {{.*}} func1t 'C (C)'
//CHECK: | | |-ParmVarDecl {{.*}} referenced arg 'C'
//CHECK: | | `-CompoundStmt
//CHECK: | |   `-ReturnStmt
//CHECK: | |     `-BinaryOperator {{.*}} '<dependent type>' '*'
//CHECK: | |       |-DeclRefExpr {{.*}} 'C' lvalue ParmVar {{.*}} 'arg' 'C'
//CHECK: | |       `-FloatingLiteral {{.*}} '_Float16' 2.000000e+00
//CHECK: | `-FunctionDecl {{.*}} used func1t '_Float16 (_Float16)'
//CHECK: |   |-TemplateArgument type '_Float16'
//CHECK: |   |-ParmVarDecl {{.*}} used arg '_Float16':'_Float16'
//CHECK: |   `-CompoundStmt
//CHECK: |     `-ReturnStmt
//CHECK: |       `-BinaryOperator {{.*}} '_Float16' '*'
//CHECK: |         |-ImplicitCastExpr {{.*}} '_Float16':'_Float16' <LValueToRValue>
//CHECK: |         | `-DeclRefExpr {{.*}} '_Float16':'_Float16' lvalue ParmVar {{.*}} 'arg' '_Float16':'_Float16'
//CHECK: |         `-FloatingLiteral {{.*}} '_Float16' 2.000000e+00


template <class C> struct S1 {
  C mem1;
};

//CHECK: |-ClassTemplateDecl {{.*}} S1
//CHECK: | |-TemplateTypeParmDecl {{.*}} referenced class depth 0 index 0 C
//CHECK: | |-CXXRecordDecl {{.*}} struct S1 definition
//CHECK: | | |-CXXRecordDecl {{.*}} implicit struct S1
//CHECK: | | `-FieldDecl {{.*}} mem1 'C'
//CHECK: | `-ClassTemplateSpecialization {{.*}} 'S1'

template <> struct S1<_Float16> {
  _Float16 mem2;
};


/* Local */

extern int printf (const char *__restrict __format, ...);

int main(void) {
  _Float16 f1l = 1e3f16;
//CHECK:     |-DeclStmt
//CHECK-NEXT:| `-VarDecl {{.*}} used f1l '_Float16' cinit
//CHECK-NEXT:|   `-FloatingLiteral {{.*}} '_Float16' 1.000000e+03

  _Float16 f2l = -0.f16;
//CHECK:     | `-VarDecl {{.*}} used f2l '_Float16' cinit
//CHECK:     |   `-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK:     |     `-FloatingLiteral {{.*}} '_Float16' 0.000000e+00

  _Float16 f3l = 1.000976562;
//CHECK:     | `-VarDecl {{.*}} used f3l '_Float16' cinit
//CHECK:     |   `-ImplicitCastExpr {{.*}} '_Float16' <FloatingCast>
//CHECK:     |     `-FloatingLiteral {{.*}} 'double' 1.000977e+00

  C1 c1(f1l);
//CHECK:     | `-VarDecl{{.*}} used c1 'class C1' callinit
//CHECK:     |   `-CXXConstructExpr {{.*}} 'class C1' 'void (_Float16)'
//CHECK:     |     `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |       `-DeclRefExpr {{.*}} '_Float16' lvalue Var 0x{{.*}} 'f1l' '_Float16'

  S1<_Float16> s1 = { 132.f16 };
//CHECK:     |-DeclStmt
//CHECK:     | `-VarDecl {{.*}} used s1 'S1<_Float16>':'struct S1<_Float16>' cinit
//CHECK:     |   `-InitListExpr {{.*}} 'S1<_Float16>':'struct S1<_Float16>'
//CHECK:     |     `-FloatingLiteral {{.*}} '_Float16' 1.320000e+02

  _Float16 f4l = func1n(f1l)  + func1f(f2l) + c1.func1c(f3l) + c1.func2c(f1l) +
    func1t(f1l) + s1.mem2 - f1n + f2n;
//CHECK:     |-DeclStmt
//CHECK:     | `-VarDecl {{.*}} used f4l '_Float16' cinit
//CHECK:     |   `-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     |-BinaryOperator {{.*}} '_Float16' '-'
//CHECK:     |     | |-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     | | |-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     | | | |-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     | | | | |-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     | | | | | |-BinaryOperator {{.*}} '_Float16' '+'
//CHECK:     |     | | | | | | |-CallExpr {{.*}} '_Float16'
//CHECK:     |     | | | | | | | |-ImplicitCastExpr {{.*}}  'const volatile _Float16 (*)(const _Float16 &)' <FunctionToPointerDecay>
//CHECK:     |     | | | | | | | | `-DeclRefExpr {{.*}} 'const volatile _Float16 (const _Float16 &)' lvalue Function {{.*}} 'func1n' 'const volatile _Float16 (const _Float16 &)'
//CHECK:     |     | | | | | | | `-ImplicitCastExpr {{.*}} 'const _Float16' lvalue <NoOp>
//CHECK:     |     | | | | | | |   `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f1l' '_Float16'
//CHECK:     |     | | | | | | `-CallExpr {{.*}} '_Float16'
//CHECK:     |     | | | | | |   |-ImplicitCastExpr {{.*}} '_Float16 (*)(_Float16)' <FunctionToPointerDecay>
//CHECK:     |     | | | | | |   | `-DeclRefExpr {{.*}} '_Float16 (_Float16)' lvalue Function {{.*}} 'func1f' '_Float16 (_Float16)'
//CHECK:     |     | | | | | |   `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     | | | | | |     `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f2l' '_Float16'
//CHECK:     |     | | | | | `-CXXMemberCallExpr {{.*}} '_Float16'
//CHECK:     |     | | | | |   |-MemberExpr {{.*}} '<bound member function type>' .func1c {{.*}}
//CHECK:     |     | | | | |   | `-DeclRefExpr {{.*}} 'class C1' lvalue Var {{.*}} 'c1' 'class C1'
//CHECK:     |     | | | | |   `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     | | | | |     `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f3l' '_Float16'
//CHECK:     |     | | | | `-CallExpr {{.*}} '_Float16'
//CHECK:     |     | | | |   |-ImplicitCastExpr {{.*}} '_Float16 (*)(_Float16)' <FunctionToPointerDecay>
//CHECK:     |     | | | |   | `-MemberExpr {{.*}} '_Float16 (_Float16)' lvalue .func2c {{.*}}
//CHECK:     |     | | | |   |   `-DeclRefExpr {{.*}} 'class C1' lvalue Var {{.*}} 'c1' 'class C1'
//CHECK:     |     | | | |   `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     | | | |     `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f1l' '_Float16'
//CHECK:     |     | | | `-CallExpr {{.*}} '_Float16':'_Float16'
//CHECK:     |     | | |   |-ImplicitCastExpr {{.*}} '_Float16 (*)(_Float16)' <FunctionToPointerDecay>
//CHECK:     |     | | |   | `-DeclRefExpr {{.*}} '_Float16 (_Float16)' lvalue Function {{.*}} 'func1t' '_Float16 (_Float16)' (FunctionTemplate {{.*}} 'func1t')
//CHECK:     |     | | |   `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     | | |     `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f1l' '_Float16'
//CHECK:     |     | | `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     | |   `-MemberExpr {{.*}} '_Float16' lvalue .mem2 {{.*}}
//CHECK:     |     | |     `-DeclRefExpr {{.*}} 'S1<_Float16>':'struct S1<_Float16>' lvalue Var {{.*}} 's1' 'S1<_Float16>':'struct S1<_Float16>'
//CHECK:     |     | `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |     |   `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f1n' '_Float16'
//CHECK:     |     `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |       `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f2n' '_Float16'

  auto f5l = -1.f16, *f6l = &f2l, f7l = func1t(f3l);
//CHECK:     |-DeclStmt {{.*}}
//CHECK:     | |-VarDecl {{.*}} f5l '_Float16':'_Float16' cinit
//CHECK:     | | `-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK:     | |   `-FloatingLiteral {{.*}} '_Float16' 1.000000e+00
//CHECK:     | |-VarDecl {{.*}} f6l '_Float16 *' cinit
//CHECK:     | | `-UnaryOperator {{.*}} '_Float16 *' prefix '&'
//CHECK:     | |   `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f2l' '_Float16'
//CHECK:     | `-VarDecl {{.*}} f7l '_Float16':'_Float16' cinit
//CHECK:     |   `-CallExpr {{.*}} '_Float16':'_Float16'
//CHECK:     |     |-ImplicitCastExpr {{.*}} '_Float16 (*)(_Float16)' <FunctionToPointerDecay>
//CHECK:     |     | `-DeclRefExpr {{.*}} '_Float16 (_Float16)' lvalue Function {{.*}} 'func1t' '_Float16 (_Float16)' (FunctionTemplate {{.*}} 'func1t')
//CHECK:     |     `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:     |       `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f3l' '_Float16'

  _Float16 f8l = f4l++;
//CHECK:     |-DeclStmt {{.*}}
//CHECK:     | `-VarDecl {{.*}} f8l '_Float16' cinit
//CHECK:     |   `-UnaryOperator {{.*}} '_Float16' postfix '++'
//CHECK:     |     `-DeclRefExpr {{.*}} '_Float16' lvalue Var {{.*}} 'f4l' '_Float16'

  _Float16 arr1l[] = { -1.f16, -0.f16, -11.f16 };
//CHECK:     |-DeclStmt {{.*}}
//CHECK:       `-VarDecl {{.*}} arr1l '_Float16 [3]' cinit
//CHECK:         `-InitListExpr {{.*}} '_Float16 [3]'
//CHECK:           |-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK:           | `-FloatingLiteral {{.*}} '_Float16' 1.000000e+00
//CHECK:           |-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK:           | `-FloatingLiteral {{.*}} '_Float16' 0.000000e+00
//CHECK:           `-UnaryOperator {{.*}} '_Float16' prefix '-'
//CHECK:             `-FloatingLiteral {{.*}} '_Float16' 1.100000e+01


// Here we are testing that Float16 gets promoted to Double by the
// "DefaultVariadicArgumentPromotion" rules:

  printf("Hello Float16 World: %f", f2l);

//CHECK:      `-CallExpr {{.*}} 'int'
//CHECK:            |-ImplicitCastExpr {{.*}} 'int (*)(const char *__restrict, ...)' <FunctionToPointerDecay>
//CHECK:            | `-DeclRefExpr {{.*}} 'int (const char *__restrict, ...)' lvalue Function 0x{{.*}} 'printf' 'int (const char *__restrict, ...)'
//CHECK:            |-ImplicitCastExpr {{.*}} 'const char *' <ArrayToPointerDecay>
//CHECK:            | `-StringLiteral {{.*}} 'const char [24]' lvalue "Hello Float16 World: %f"
//CHECK:            `-ImplicitCastExpr {{.*}} 'double' <FloatingCast>
//CHECK:              `-ImplicitCastExpr {{.*}} '_Float16' <LValueToRValue>
//CHECK:                `-DeclRefExpr {{.*}} '_Float16' lvalue Var 0x{{.*}} 'f2l' '_Float16'
}
