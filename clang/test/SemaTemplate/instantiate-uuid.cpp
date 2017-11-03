// RUN: %clang_cc1 -fsyntax-only -verify -fms-extensions -fms-compatibility %s
// expected-no-diagnostics

typedef struct _GUID {
  int i;
} IID;
template <const IID *piid>
class A {};

struct
    __declspec(uuid("{DDB47A6A-0F23-11D5-9109-00E0296B75D3}"))
        S1 {};

struct
    __declspec(uuid("{DDB47A6A-0F23-11D5-9109-00E0296B75D3}"))
        S2 {};

struct __declspec(dllexport)
    C1 : public A<&__uuidof(S1)> {};

struct __declspec(dllexport)
    C2 : public A<&__uuidof(S2)> {};
