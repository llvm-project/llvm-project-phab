// RUN: %clang_cc1 -std=c++11 -emit-llvm -o - %s | FileCheck %s

struct S {
  int a[4];
  S(int *, int * __attribute__((noescape)));
  S &operator=(int * __attribute__((noescape)));
  void m0(int *, int * __attribute__((noescape)));
  virtual void vm1(int *, int * __attribute__((noescape)));
};

// CHECK: define void @_ZN1SC2EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture)
// CHECK: define void @_ZN1SC1EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture) {{.*}} {
// CHECK: call void @_ZN1SC2EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture {{.*}})

S::S(int *, int * __attribute__((noescape))) {}

// CHECK: define {{.*}} %struct.S* @_ZN1SaSEPiU8noescape(%struct.S* {{.*}}, {{.*}} nocapture)
S &S::operator=(int * __attribute__((noescape))) { return *this; }

// CHECK: define void @_ZN1S2m0EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}} nocapture)
void S::m0(int *, int * __attribute__((noescape))) {}

// CHECK: define void @_ZN1S3vm1EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}} nocapture)
void S::vm1(int *, int * __attribute__((noescape))) {}

// CHECK-LABEL: define void @_Z5test0P1SPiS1_(
// CHECK: call void @_ZN1SC1EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture {{.*}})
// CHECK: call {{.*}} %struct.S* @_ZN1SaSEPiU8noescape(%struct.S* {{.*}}, {{.*}} nocapture {{.*}})
// CHECK: call void @_ZN1S2m0EPiS0_U8noescape(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture {{.*}})
// CHECK: call void {{.*}}(%struct.S* {{.*}}, {{.*}}, {{.*}} nocapture {{.*}})
void test0(S *s, int *p0, int *p1) {
  S t(p0, p1);
  t = p1;
  s->m0(p0, p1);
  s->vm1(p0, p1);
}

namespace std {
  typedef decltype(sizeof(0)) size_t;
}

// CHECK: define {{.*}} @_ZnwmPvU8noescape({{.*}}, {{.*}} nocapture {{.*}})
void *operator new(std::size_t, void * __attribute__((noescape)) p) {
  return p;
}

// CHECK-LABEL: define i8* @_Z5test1Pv(
// CHECK : %call = call {{.*}} @_ZnwmPv({{.*}}, {{.*}} nocapture {{.*}})
void *test1(void *p0) {
  return ::operator new(16, p0);
}

// CHECK-LABEL: define void @_Z5test2PiS_(
// CHECK: call void @"_ZZ5test2PiS_ENK3$_0clES_S_U8noescape"({{.*}}, {{.*}}, {{.*}} nocapture {{.*}})
// CHECK: define internal void @"_ZZ5test2PiS_ENK3$_0clES_S_U8noescape"({{.*}}, {{.*}}, {{.*}} nocapture)
void test2(int *p0, int *p1) {
  auto t = [](int *, int * __attribute__((noescape))){};
  t(p0, p1);
}
