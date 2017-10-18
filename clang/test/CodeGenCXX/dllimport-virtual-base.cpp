// RUN: %clang_cc1 -triple i386-pc-windows -emit-llvm -fms-compatibility %s -x c++ -o - | FileCheck %s

namespace test1 {
struct BaseClass {
  ~BaseClass();
};

struct __declspec(dllimport) Concrete : virtual BaseClass {
};

Concrete c;
} // namespace test1

namespace test2 {
class BaseClass {
public:
  virtual ~BaseClass(){};
};

class __declspec(dllimport) VirtualClass : public virtual BaseClass {
public:
  virtual ~VirtualClass(){};
};

int main() {
  VirtualClass c;
  return 0;
}
} // namespace test2

namespace test3 {
class IVirtualBase {
public:
  virtual ~IVirtualBase(){};
  virtual void speak() = 0;
};

class VirtualClass : public virtual IVirtualBase {
public:
  virtual ~VirtualClass(){};
  virtual void eat() = 0;
};

class __declspec(dllimport) ConcreteClass : public VirtualClass {
public:
  ConcreteClass(int nn);
  void speak();
  void eat();
  virtual ~ConcreteClass();

private:
  int n;
};

int main() {
  ConcreteClass c(10);
  c.speak();
  c.eat();
  return 0;
}
} // namespace test3

// CHECK-LABEL: declare dllimport x86_thiscallcc %"struct.test1::Concrete"* @"\01??0Concrete@test1@@QAE@XZ"
// CHECK-LABEL: declare dllimport x86_thiscallcc %"class.test2::VirtualClass"* @"\01??0VirtualClass@test2@@QAE@XZ"
// CHECK-LABEL: declare dllimport x86_thiscallcc %"class.test3::ConcreteClass"* @"\01??0ConcreteClass@test3@@QAE@H@Z"
// CHECK-NOT: ??_DVirtualClass@@QEAAXXZ
// CHECK-NOT: ??_DConcreteClass@@QEAAXXZ
