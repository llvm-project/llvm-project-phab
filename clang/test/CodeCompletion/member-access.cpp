struct Base1 {
  int member1;
  float member2;
};

struct Base2 {
  int member1;
  double member3;
  void memfun1(int);
};

struct Base3 : Base1, Base2 {
  void memfun1(float);
  void memfun1(double) const;
  void memfun2(int);
};

struct Derived : Base3 {
  int member4;
  int memfun3(int);
};

class Proxy {
public:
  Derived *operator->() const;
};

void test(const Proxy &p) {
  p->
}

struct Test1 {
  Base1 b;

  static void sfunc() {
    b. // expected-error {{invalid use of member 'b' in static member function}}
  }
};

struct Foo {
  void foo() const;
  static void foo(bool);
};

struct Bar {
  void foo(bool param) {
    Foo::foo(  );// unresolved member expression with an implicit base
  }
};

  // RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:29:6 %s -o - | FileCheck -check-prefix=CHECK-CC1 %s
  // CHECK-CC1: Base1 : Base1::
  // CHECK-CC1: member1 : [#int#][#Base1::#]member1
  // CHECK-CC1: member1 : [#int#][#Base2::#]member1
  // CHECK-CC1: member2 : [#float#][#Base1::#]member2
  // CHECK-CC1: member3
  // CHECK-CC1: member4
  // CHECK-CC1: memfun1 : [#void#][#Base3::#]memfun1(<#float#>)
  // CHECK-CC1: memfun1 : [#void#][#Base3::#]memfun1(<#double#>)[# const#]
  // CHECK-CC1: memfun1 (Hidden) : [#void#]Base2::memfun1(<#int#>)
  // CHECK-CC1: memfun2 : [#void#][#Base3::#]memfun2(<#int#>)
  // CHECK-CC1: memfun3 : [#int#]memfun3(<#int#>)

  // RUN: c-index-test -code-completion-at=%s:29:6 %s -o - |FileCheck -check-prefix=CHECK-CC2 %s
  // CHECK-CC2: FieldDecl:{ResultType int}{Informative Base1::}{TypedText member1} (37) (source location: 2:7)
  // CHECK-CC2: CXXMethod:{ResultType Base1 &}{Text Base1::}{TypedText operator=}{LeftParen (}{Placeholder const Base1 &}{RightParen )} (36) (source location: 0:0)
  // CHECK-CC2: CXXDestructor:{ResultType void}{TypedText ~Derived}{LeftParen (}{RightParen )} (34) (source location: 0:0)

// Make sure this doesn't crash
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:36:7 %s -verify

// Make sure this also doesn't crash
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:47:14 %s


template<typename T>
class BaseTemplate {
public:
  T baseTemplateFunction();

  T baseTemplateField;
};

template<typename T, typename S>
class TemplateClass: public Base1 , public BaseTemplate<T> {
public:
  T function() { }
  T field;

  void overload1(const T &);
  void overload1(const S &);
};

template<typename T, typename S>
void completeDependentMembers(TemplateClass<T, S> &object,
                              TemplateClass<int, S> *object2) {
  object.field;
  object2->field;
// CHECK-CC3: baseTemplateField : [#T#][#BaseTemplate<T>::#]baseTemplateField
// CHECK-CC3: baseTemplateFunction : [#T#][#BaseTemplate<T>::#]baseTemplateFunction()
// CHECK-CC3: field : [#T#]field
// CHECK-CC3: function : [#T#]function()
// CHECK-CC3: member1 : [#int#][#Base1::#]member1
// CHECK-CC3: member2 : [#float#][#Base1::#]member2
// CHECK-CC3: overload1 : [#void#]overload1(<#const T &#>)
// CHECK-CC3: overload1 : [#void#]overload1(<#const S &#>)

// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:97:10 %s -o - | FileCheck -check-prefix=CHECK-CC3 %s
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:98:12 %s -o - | FileCheck -check-prefix=CHECK-CC3 %s
}


void completeDependentSpecializedMembers(TemplateClass<int, double> &object,
                                         TemplateClass<int, double> *object2) {
  object.field;
  object2->field;
// CHECK-CC4: baseTemplateField : [#int#][#BaseTemplate<int>::#]baseTemplateField
// CHECK-CC4: baseTemplateFunction : [#int#][#BaseTemplate<int>::#]baseTemplateFunction()
// CHECK-CC4: field : [#int#]field
// CHECK-CC4: function : [#int#]function()
// CHECK-CC4: member1 : [#int#][#Base1::#]member1
// CHECK-CC4: member2 : [#float#][#Base1::#]member2
// CHECK-CC4: overload1 : [#void#]overload1(<#const int &#>)
// CHECK-CC4: overload1 : [#void#]overload1(<#const double &#>)

// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:115:10 %s -o - | FileCheck -check-prefix=CHECK-CC4 %s
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:116:12 %s -o - | FileCheck -check-prefix=CHECK-CC4 %s
}

template <typename T>
class Template {
public:
  BaseTemplate<int> o1;
  BaseTemplate<T> o2;

  void function() {
    o1.baseTemplateField;
// CHECK-CC5: BaseTemplate : BaseTemplate::
// CHECK-CC5: baseTemplateField : [#int#]baseTemplateField
// CHECK-CC5: baseTemplateFunction : [#int#]baseTemplateFunction()
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:137:8 %s -o - | FileCheck -check-prefix=CHECK-CC5 %s
    o2.baseTemplateField;
// CHECK-CC6: BaseTemplate : BaseTemplate::
// CHECK-CC6: baseTemplateField : [#T#]baseTemplateField
// CHECK-CC6: baseTemplateFunction : [#T#]baseTemplateFunction()
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:142:8 %s -o - | FileCheck -check-prefix=CHECK-CC6 %s
    this->o1;
// CHECK-CC7: [#void#]function()
// CHECK-CC7: o1 : [#BaseTemplate<int>#]o1
// CHECK-CC7: o2 : [#BaseTemplate<T>#]o2
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:147:11 %s -o - | FileCheck -check-prefix=CHECK-CC7 %s
  }

  static void staticFn(T &obj);

  struct Nested { };
};

template<typename T>
void dependentColonColonCompletion() {
  Template<T>::staticFn();
// CHECK-CC8: function : [#void#]function()
// CHECK-CC8: Nested : Nested
// CHECK-CC8: o1 : [#BaseTemplate<int>#]o1
// CHECK-CC8: o2 : [#BaseTemplate<T>#]o2
// CHECK-CC8: staticFn : [#void#]staticFn(<#T &obj#>)
// CHECK-CC8: Template : Template
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:161:16 %s -o - | FileCheck -check-prefix=CHECK-CC8 %s
  typename Template<T>::Nested m;
// RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:169:25 %s -o - | FileCheck -check-prefix=CHECK-CC8 %s
}
