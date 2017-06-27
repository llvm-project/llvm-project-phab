//===- llvm/unittest/Demangle/MicrosoftDemangleTest.cpp -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Demangle/Demangle.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(MicrosoftDemanglerTest, Basic) {
  auto Demangle = [&](const std::string &S) {
    std::string Err;
    std::string Ret = microsoftDemangle(S, Err);
    EXPECT_EQ("", Err);
    return Ret;
  };

  EXPECT_EQ("int x", Demangle("?x@@3HA"));
  EXPECT_EQ("int*x", Demangle("?x@@3PEAHEA"));
  EXPECT_EQ("int**x", Demangle("?x@@3PEAPEAHEA"));
  EXPECT_EQ("int(*x)[3]", Demangle("?x@@3PEAY02HEA"));
  EXPECT_EQ("int(*x)[3][5]", Demangle("?x@@3PEAY124HEA"));
  EXPECT_EQ("int const(*x)[3]", Demangle("?x@@3PEAY02$$CBHEA"));
  EXPECT_EQ("unsigned char*x", Demangle("?x@@3PEAEEA"));
  EXPECT_EQ("int(*x)[3500][6]", Demangle("?x@@3PEAY1NKM@5HEA"));
  EXPECT_EQ("void x(float,int)", Demangle("?x@@YAXMH@Z"));
  EXPECT_EQ("void x(float,int)", Demangle("?x@@YAXMH@Z"));
  EXPECT_EQ("int(*x)(float,double,int)", Demangle("?x@@3P6AHMNH@ZEA"));
  EXPECT_EQ("int(*x)(int(*)(float),double)", Demangle("?x@@3P6AHP6AHM@ZN@ZEA"));

  EXPECT_EQ("int ns::x", Demangle("?x@ns@@3HA"));

  // Microsoft's undname returns "int const * const x" for this symbol.
  // I believe there's a bug in their demangler.
  EXPECT_EQ("int const*x", Demangle("?x@@3PEBHEB"));

  EXPECT_EQ("int const*const x", Demangle("?x@@3QEBHEB"));

  EXPECT_EQ("struct ty*x", Demangle("?x@@3PEAUty@@EA"));
  EXPECT_EQ("union ty*x", Demangle("?x@@3PEATty@@EA"));
  EXPECT_EQ("struct ty*x", Demangle("?x@@3PEAUty@@EA"));
  EXPECT_EQ("enum ty*x", Demangle("?x@@3PEAW4ty@@EA"));
  EXPECT_EQ("class ty*x", Demangle("?x@@3PEAVty@@EA"));

  EXPECT_EQ("class tmpl<int>*x", Demangle("?x@@3PEAV?$tmpl@H@@EA"));
  EXPECT_EQ("class klass instance", Demangle("?instance@@3Vklass@@A"));
  EXPECT_EQ("void(*instance$initializer$)(void)",
            Demangle("?instance$initializer$@@3P6AXXZEA"));
  EXPECT_EQ("klass::klass(void)", Demangle("??0klass@@QEAA@XZ"));
  EXPECT_EQ("klass::~klass(void)", Demangle("??1klass@@QEAA@XZ"));
}

} // end namespace
