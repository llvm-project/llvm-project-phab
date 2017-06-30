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
  EXPECT_EQ("int(*x)(int(*)(float),int(*)(float))",
            Demangle("?x@@3P6AHP6AHM@Z0@ZEA"));

  EXPECT_EQ("int ns::x", Demangle("?x@ns@@3HA"));

  // Microsoft"s undname returns "int const * const x" for this symbol.
  // I believe it"s their bug.
  EXPECT_EQ("int const*x", Demangle("?x@@3PEBHEB"));

  EXPECT_EQ("int*const x", Demangle("?x@@3QEAHEB"));
  EXPECT_EQ("int const*const x", Demangle("?x@@3QEBHEB"));

  EXPECT_EQ("int const&x", Demangle("?x@@3AEBHEB"));

  EXPECT_EQ("struct ty*x", Demangle("?x@@3PEAUty@@EA"));
  EXPECT_EQ("union ty*x", Demangle("?x@@3PEATty@@EA"));
  EXPECT_EQ("struct ty*x", Demangle("?x@@3PEAUty@@EA"));
  EXPECT_EQ("enum ty*x", Demangle("?x@@3PEAW4ty@@EA"));
  EXPECT_EQ("class ty*x", Demangle("?x@@3PEAVty@@EA"));

  EXPECT_EQ("class tmpl<int>*x", Demangle("?x@@3PEAV?$tmpl@H@@EA"));
  EXPECT_EQ("struct tmpl<int>*x", Demangle("?x@@3PEAU?$tmpl@H@@EA"));
  EXPECT_EQ("union tmpl<int>*x", Demangle("?x@@3PEAT?$tmpl@H@@EA"));
  EXPECT_EQ("class klass instance", Demangle("?instance@@3Vklass@@A"));
  EXPECT_EQ("void(*instance$initializer$)(void)",
            Demangle("?instance$initializer$@@3P6AXXZEA"));
  EXPECT_EQ("klass::klass(void)", Demangle("??0klass@@QEAA@XZ"));
  EXPECT_EQ("klass::~klass(void)", Demangle("??1klass@@QEAA@XZ"));
  EXPECT_EQ("int x(class klass*,class klass&)",
            Demangle("?x@@YAHPEAVklass@@AEAV1@@Z"));
  EXPECT_EQ("class ns::klass<int,int>*ns::x",
            Demangle("?x@ns@@3PEAV?$klass@HH@1@EA"));
  EXPECT_EQ("unsigned int ns::klass<int>::fn(void)const",
            Demangle("?fn@?$klass@H@ns@@QEBAIXZ"));

  EXPECT_EQ("class klass const&klass::operator=(class klass const&)",
            Demangle("??4klass@@QEAAAEBV0@AEBV0@@Z"));
  EXPECT_EQ("bool klass::operator!(void)", Demangle("??7klass@@QEAA_NXZ"));
  EXPECT_EQ("bool klass::operator==(class klass const&)",
            Demangle("??8klass@@QEAA_NAEBV0@@Z"));
  EXPECT_EQ("bool klass::operator!=(class klass const&)",
            Demangle("??9klass@@QEAA_NAEBV0@@Z"));
  EXPECT_EQ("int klass::operator[](uint64_t)", Demangle("??Aklass@@QEAAH_K@Z"));
  EXPECT_EQ("int klass::operator->(void)", Demangle("??Cklass@@QEAAHXZ"));
  EXPECT_EQ("int klass::operator*(void)", Demangle("??Dklass@@QEAAHXZ"));
  EXPECT_EQ("int klass::operator++(void)", Demangle("??Eklass@@QEAAHXZ"));
  EXPECT_EQ("int klass::operator++(int)", Demangle("??Eklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator--(void)", Demangle("??Fklass@@QEAAHXZ"));
  EXPECT_EQ("int klass::operator--(int)", Demangle("??Fklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator+(int)", Demangle("??Hklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator-(int)", Demangle("??Gklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator&(int)", Demangle("??Iklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator->*(int)", Demangle("??Jklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator/(int)", Demangle("??Kklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator<(int)", Demangle("??Mklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator<=(int)", Demangle("??Nklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator>(int)", Demangle("??Oklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator>=(int)", Demangle("??Pklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator,(int)", Demangle("??Qklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator()(int)", Demangle("??Rklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator~(void)", Demangle("??Sklass@@QEAAHXZ"));
  EXPECT_EQ("int klass::operator^(int)", Demangle("??Tklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator|(int)", Demangle("??Uklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator&&(int)", Demangle("??Vklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator||(int)", Demangle("??Wklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator*=(int)", Demangle("??Xklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator+=(int)", Demangle("??Yklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator-=(int)", Demangle("??Zklass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator/=(int)", Demangle("??_0klass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator%=(int)", Demangle("??_1klass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator>>=(int)", Demangle("??_2klass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator<<=(int)", Demangle("??_3klass@@QEAAHH@Z"));
  EXPECT_EQ("int klass::operator^=(int)", Demangle("??_6klass@@QEAAHH@Z"));
  EXPECT_EQ("class klass const&operator<<(class klass const&,int)",
            Demangle("??6@YAAEBVklass@@AEBV0@H@Z"));
  EXPECT_EQ("class klass const&operator>>(class klass const&,uint64_t)",
            Demangle("??5@YAAEBVklass@@AEBV0@_K@Z"));
  EXPECT_EQ("void*operator new(uint64_t,class klass&)",
            Demangle("??2@YAPEAX_KAEAVklass@@@Z"));
  EXPECT_EQ("void*operator new[](uint64_t,class klass&)",
            Demangle("??_U@YAPEAX_KAEAVklass@@@Z"));
  EXPECT_EQ("void operator delete(void*,class klass&)",
            Demangle("??3@YAXPEAXAEAVklass@@@Z"));
  EXPECT_EQ("void operator delete[](void*,class klass&)",
            Demangle("??_V@YAXPEAXAEAVklass@@@Z"));
}

} // end namespace
