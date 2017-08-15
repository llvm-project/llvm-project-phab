//===- unittest/Tooling/QualTypeNameTest.cpp ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TestVisitor.h"
#include "clang/Tooling/Core/QualTypeNames.h"
using namespace clang;

namespace {
struct TypeNameVisitor : TestVisitor<TypeNameVisitor> {
  llvm::StringMap<std::string> ExpectedQualTypeNames;
  bool WithGlobalNsPrefix = false;
  bool CustomPrintingPolicy = false;

  // ValueDecls are the least-derived decl with both a qualtype and a
  // name.
  bool traverseDecl(Decl *D) {
    return true; // Always continue
  }

  bool VisitValueDecl(const ValueDecl *VD) {
    std::string ExpectedName =
        ExpectedQualTypeNames.lookup(VD->getNameAsString());
    if (ExpectedName != "") {
      std::string ActualName;
      if (!CustomPrintingPolicy)
        ActualName = TypeName::getFullyQualifiedName(VD->getType(), *Context,
                                                     WithGlobalNsPrefix);
      else {
        PrintingPolicy Policy(Context->getPrintingPolicy());
        Policy.SuppressScope = false;
        Policy.AnonymousTagLocations = true;
        Policy.PolishForDeclaration = true;
        Policy.SuppressUnwrittenScope = true;
        ActualName = TypeName::getFullyQualifiedName(
            VD->getType(), *Context, Policy, WithGlobalNsPrefix);
      }

      if (ExpectedName != ActualName) {
        // A custom message makes it much easier to see what declaration
        // failed compared to EXPECT_EQ.
        EXPECT_TRUE(false) << "Typename::getFullyQualifiedName failed for "
                           << VD->getQualifiedNameAsString() << std::endl
                           << "   Actual: " << ActualName << std::endl
                           << " Exepcted: " << ExpectedName;
      }
    }
    return true;
  }
};

// named namespaces inside anonymous namespaces

TEST(QualTypeNameTest, getFullyQualifiedName) {
  TypeNameVisitor Visitor;
  // Simple case to test the test framework itself.
  Visitor.ExpectedQualTypeNames["CheckInt"] = "int";

  // Keeping the names of the variables whose types we check unique
  // within the entire test--regardless of their own scope--makes it
  // easier to diagnose test failures.

  // Simple namespace qualifier
  Visitor.ExpectedQualTypeNames["CheckA"] = "A::B::Class0";
  // Lookup up the enclosing scopes, then down another one. (These
  // appear as elaborated type in the AST. In that case--even if
  // policy.SuppressScope = 0--qual_type.getAsString(policy) only
  // gives the name as it appears in the source, not the full name.
  Visitor.ExpectedQualTypeNames["CheckB"] = "A::B::C::Class1";
  // Template parameter expansion.
  Visitor.ExpectedQualTypeNames["CheckC"] =
      "A::B::Template0<A::B::C::MyInt, A::B::AnotherClass>";
  // Recursive template parameter expansion.
  Visitor.ExpectedQualTypeNames["CheckD"] =
      "A::B::Template0<A::B::Template1<A::B::C::MyInt, A::B::AnotherClass>, "
      "A::B::Template0<int, long> >";
  // Variadic Template expansion.
  Visitor.ExpectedQualTypeNames["CheckE"] =
      "A::Variadic<int, A::B::Template0<int, char>, "
      "A::B::Template1<int, long>, A::B::C::MyInt>";
  // Using declarations should be fully expanded.
  Visitor.ExpectedQualTypeNames["CheckF"] = "A::B::Class0";
  // Elements found within "using namespace foo;" should be fully
  // expanded.
  Visitor.ExpectedQualTypeNames["CheckG"] = "A::B::C::MyInt";
  // Type inside function
  Visitor.ExpectedQualTypeNames["CheckH"] = "struct X";
  // Anonymous Namespaces
  Visitor.ExpectedQualTypeNames["CheckI"] = "aClass";
  // Keyword inclusion with namespaces
  Visitor.ExpectedQualTypeNames["CheckJ"] = "struct A::aStruct";
  // Anonymous Namespaces nested in named namespaces and vice-versa.
  Visitor.ExpectedQualTypeNames["CheckK"] = "D::aStruct";
  // Namespace alias
  Visitor.ExpectedQualTypeNames["CheckL"] = "A::B::C::MyInt";
  Visitor.ExpectedQualTypeNames["non_dependent_type_var"] =
      "Foo<X>::non_dependent_type";
  Visitor.ExpectedQualTypeNames["AnEnumVar"] = "EnumScopeClass::AnEnum";
  Visitor.ExpectedQualTypeNames["AliasTypeVal"] = "A::B::C::InnerAlias<int>";
  Visitor.ExpectedQualTypeNames["CheckM"] = "const A::B::Class0 *";
  Visitor.ExpectedQualTypeNames["CheckN"] = "const X *";
  Visitor.runOver(
      R"(int CheckInt;
         template <typename>
         class OuterTemplateClass { };
         namespace A {
          namespace B {
            class Class0 { };
            namespace C {
              typedef int MyInt;
              template <typename T>
              using InnerAlias = OuterTemplateClass<T>;
              InnerAlias<int> AliasTypeVal;
            }
            template<class X, class Y> class Template0;
            template<class X, class Y> class Template1;
            typedef B::Class0 AnotherClass;
            void Function1(Template0<C::MyInt,
                           AnotherClass> CheckC);
            void Function2(Template0<Template1<C::MyInt, AnotherClass>,
                                     Template0<int, long> > CheckD);
            void Function3(const B::Class0* CheckM);
           }
         template<typename... Values> class Variadic {};
         Variadic<int, B::Template0<int, char>, 
                  B::Template1<int, long>, 
                  B::C::MyInt > CheckE;
          namespace BC = B::C;
          BC::MyInt CheckL;
         }
         using A::B::Class0;
         void Function(Class0 CheckF);
         using namespace A::B::C;
         void Function(MyInt CheckG);
         void f() {
           struct X {} CheckH;
         }
         struct X;
         void f(const ::X* CheckN) {}
         namespace {
           class aClass {};
            aClass CheckI;
         }
         namespace A {
           struct aStruct {} CheckJ;
         }
         namespace {
           namespace D {
             namespace {
               class aStruct {};
               aStruct CheckK;
             }
           }
         }
         template<class T> struct Foo {
           typedef typename T::A dependent_type;
           typedef int non_dependent_type;
           dependent_type dependent_type_var;
           non_dependent_type non_dependent_type_var;
         };
         struct X { typedef int A; };
         Foo<X> var;
         void F() {
           var.dependent_type_var = 0;
         var.non_dependent_type_var = 0;
         }
         class EnumScopeClass {
         public:
           enum AnEnum { ZERO, ONE };
         };
         EnumScopeClass::AnEnum AnEnumVar;)",
      TypeNameVisitor::Lang_CXX11);

  TypeNameVisitor Complex;
  Complex.ExpectedQualTypeNames["CheckTX"] = "B::TX";
  Complex.runOver(R"(namespace A {
                       struct X {};
                     }
                     using A::X;
                     namespace fake_std {
                       template<class... Types > class tuple {};
                     }
                     namespace B {
                       using fake_std::tuple;
                       typedef tuple<X> TX;
                       TX CheckTX;
                       struct A { typedef int X; };
                     })");

  TypeNameVisitor GlobalNsPrefix;
  GlobalNsPrefix.WithGlobalNsPrefix = true;
  GlobalNsPrefix.ExpectedQualTypeNames["IntVal"] = "int";
  GlobalNsPrefix.ExpectedQualTypeNames["BoolVal"] = "bool";
  GlobalNsPrefix.ExpectedQualTypeNames["XVal"] = "::A::B::X";
  GlobalNsPrefix.ExpectedQualTypeNames["IntAliasVal"] = "::A::B::Alias<int>";
  GlobalNsPrefix.ExpectedQualTypeNames["ZVal"] = "::A::B::Y::Z";
  GlobalNsPrefix.ExpectedQualTypeNames["GlobalZVal"] = "::Z";
  GlobalNsPrefix.ExpectedQualTypeNames["CheckK"] = "D::aStruct";
  GlobalNsPrefix.runOver(R"(namespace A {
                              namespace B {
                                int IntVal;
                                bool BoolVal;
                                struct X {};
                                X XVal;
                                template <typename T> class CCC { };
                                template <typename T>
                                using Alias = CCC<T>;
                                Alias<int> IntAliasVal;
                                struct Y { struct Z {}; };
                                Y::Z ZVal;
                              }
                             }
                            struct Z {};
                            Z GlobalZVal;
                            namespace {
                              namespace D {
                                namespace {
                                  class aStruct {};
                                  aStruct CheckK;
                                }
                              }
                            })");

  TypeNameVisitor PrintingPolicy;
  PrintingPolicy.CustomPrintingPolicy = true;
  PrintingPolicy.ExpectedQualTypeNames["a"] = "short";
  PrintingPolicy.ExpectedQualTypeNames["un_in_st_1"] =
      "union (anonymous struct at input.cc:1:1)::(anonymous union at "
      "input.cc:2:31)";
  PrintingPolicy.ExpectedQualTypeNames["b"] = "short";
  PrintingPolicy.ExpectedQualTypeNames["un_in_st_2"] =
      "union (anonymous struct at input.cc:1:1)::(anonymous union at "
      "input.cc:5:31)";
  PrintingPolicy.ExpectedQualTypeNames["anon_st"] =
      "struct (anonymous struct at input.cc:1:1)";
  PrintingPolicy.runOver(R"(struct {
                              union {
                                short a;
                              } un_in_st_1;
                              union {
                                short b;
                              } un_in_st_2;
                            } anon_st;)");
}

} // end anonymous namespace
