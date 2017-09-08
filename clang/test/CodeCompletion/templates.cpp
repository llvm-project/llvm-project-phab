namespace std {
  template<typename T>
  class allocator { 
  public:
    void in_base();
  };
  
  template<typename T, typename Alloc = std::allocator<T> >
  class vector : Alloc {
  public:
    void foo();
    void stop();
  };
  template<typename Alloc> class vector<bool, Alloc>;
}

void f() {
  std::vector<int> v;
  v.foo();
  // RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:18:8 %s -o - | FileCheck -check-prefix=CHECK-CC1 %s
  // CHECK-CC1: allocator<<#typename T#>>
  // CHECK-CC1-NEXT: vector<<#typename T#>{#, <#typename Alloc#>#}>
  // RUN: %clang_cc1 -fsyntax-only -code-completion-at=%s:19:5 %s -o - | FileCheck -check-prefix=CHECK-CC2 %s
  // CHECK-CC2: foo
  // CHECK-CC2: in_base
  // CHECK-CC2: stop
  // RUN: c-index-test -code-completion-at=%s:18:8 %s -o - | FileCheck -check-prefix=CHECK-CC3 %s
  // CHECK-CC3: ClassTemplate:{TypedText allocator}{LeftAngle <}{Placeholder typename T}{RightAngle >} (50) (source location: 3:9)
  // CHECK-CC3-NEXT: ClassTemplate:{TypedText vector}{LeftAngle <}{Placeholder typename T}{Optional {Comma , }{Placeholder typename Alloc}}{RightAngle >} (50) (source location: 9:9)


