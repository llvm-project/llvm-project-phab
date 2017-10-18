// RUN: %clang_cc1 -triple i386-pc-windows -emit-llvm -fms-compatibility %s -x c++ -o - | FileCheck %s

// CHECK: declare dllimport x86_thiscallcc void @"\01??1VirtualClass@test1@@UAE@XZ"
// CHECK: declare dllimport x86_thiscallcc void @"\01??1VirtualClass@test2@@UAE@XZ"
// CHECK-NOT: ??_DVirtualClass@@QEAAXXZ

namespace test1 {
struct __declspec(dllimport) VirtualClass {
public:
  virtual ~VirtualClass(){};
};

int main() {
  VirtualClass c;
  return 0;
}
} // namespace test1

namespace test2 {
class BaseClass {
public:
  virtual ~BaseClass(){};
};

class __declspec(dllimport) VirtualClass : public BaseClass {
public:
  virtual ~VirtualClass(){};
};

int main() {
  VirtualClass c;
  return 0;
}
} // namespace test2

namespace test3 {
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
} // namespace test3
