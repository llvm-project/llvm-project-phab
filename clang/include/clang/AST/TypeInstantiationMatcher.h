//===--- TypeInstantiationMatcher.h -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  Defines TypeMatcher that is used to check if one type is or could be an
//  instantiation of other type.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_TYPEINSTANTIATIONMATCHER_H
#define LLVM_CLANG_AST_TYPEINSTANTIATIONMATCHER_H

#include "clang/AST/TypeMatcher.h"
#include "clang/AST/DeclTemplate.h"
#include <map>

namespace clang {

/// \brief Visitor class used to check if one type is instantiation of another.
///
class TypeInstantiationMatcher : public TypeMatcher<TypeInstantiationMatcher> {
  std::map<const TemplateTypeParmType *, QualType> ParamMapping;

public:

  bool VisitType(QualType PQ, QualType TQ) {
    return PQ.getQualifiers() == TQ.getQualifiers();
  }

  bool VisitBuiltinType(QualType PQ, QualType TQ) {
    return PQ.getCanonicalType() == TQ.getCanonicalType();
  }

  bool VisitConstantArrayType(QualType PQ, QualType TQ) {
    const auto *P = cast<ConstantArrayType>(PQ.getTypePtr());
    const auto *T = cast<ConstantArrayType>(TQ.getTypePtr());
    return P->getSize() == T->getSize();
  }

  bool VisitVectorType(QualType PQ, QualType TQ) {
    const auto *P = PQ->getAs<VectorType>();
    const auto *T = TQ->getAs<VectorType>();
    return P->getNumElements() == T->getNumElements();
  }

  bool VisitFunctionProtoType(QualType PQ, QualType TQ) {
    const auto *P = PQ->getAs<FunctionProtoType>();
    const auto *T = TQ->getAs<FunctionProtoType>();
    if (P->getNumParams() != T->getNumParams())
      return false;
    if (P->exceptions().size() != T->exceptions().size())
      return false;
    return true;
  }

  bool VisitTemplateTypeParmType(QualType PQ, QualType TQ) {
    auto P = PQ->getCanonicalTypeInternal()->getAs<TemplateTypeParmType>();
    auto Ptr = ParamMapping.find(P);
    if (Ptr != ParamMapping.end())
      return Ptr->second == TQ;
    ParamMapping[P] = TQ;
    return true;
  }

  bool TraverseTemplateTypeParmType(QualType PQ, QualType TQ) {
    return WalkUpFromTemplateTypeParmType(PQ, TQ);
  }

  bool matchTemplateParametersAndArguments(const TemplateParameterList &PL,
                                           const TemplateArgumentList &AL) {
    if (PL.size() != AL.size())
      return false;
    for (unsigned I = 0, E = PL.size(); I < E; ++I) {
      const NamedDecl *PD = PL.getParam(I);
      const TemplateArgument &TA = AL.get(I);
      TemplateArgument::ArgKind kind = TA.getKind();
      if (const auto *TTP = dyn_cast<TemplateTypeParmDecl>(PD)) {
        if (kind != TemplateArgument::Type)
          return false;
        auto Type = TTP->getTypeForDecl()->getCanonicalTypeInternal()
            ->getAs<TemplateTypeParmType>();
        auto Ptr = ParamMapping.find(Type);
        if (Ptr != ParamMapping.end())
          return Ptr->second == TA.getAsType();
        ParamMapping[Type] = TA.getAsType();
      }
    }
    return true;
  }

  bool TraverseTemplateSpecializationType(QualType PQ, QualType TQ) {
    return WalkUpFromTemplateSpecializationType(PQ, TQ);
  }

  bool VisitTemplateSpecializationType(QualType PQ, QualType TQ) {
    const auto *P = PQ->getAs<TemplateSpecializationType>();
    TemplateName PTN = P->getTemplateName();
    if (PTN.getKind() != TemplateName::Template)
      return false;
    TemplateDecl *PT = PTN.getAsTemplateDecl();
    auto *PTD = dyn_cast<ClassTemplateDecl>(PT);
    if (!PTD)
      return false;
    auto PParams = PTD->getTemplateParameters();

    if (const CXXRecordDecl *CD = TQ->getAsCXXRecordDecl()) {
      if (const auto *SD = dyn_cast<ClassTemplateSpecializationDecl>(CD))
        if (ClassTemplateDecl *TD = SD->getSpecializedTemplate()) {
          if (PTD->getCanonicalDecl() != TD->getCanonicalDecl())
            return false;
          const TemplateArgumentList &TArgs = SD->getTemplateArgs();
          if (PParams->size() != TArgs.size())
            return false;
          return matchTemplateParametersAndArguments(*PParams, TArgs);
        }
      return false;
    }

    return true;
  }

  bool VisitInjectedClassNameType(QualType PQ, QualType TQ) {
    const auto *P = PQ->getAs<InjectedClassNameType>();
    return match(QualType(P->getInjectedSpecializationType()),
                 TQ.getUnqualifiedType());
  }

  bool TraverseInjectedClassNameType(QualType PQ, QualType TQ) {
    return WalkUpFromInjectedClassNameType(PQ, TQ);
  }

  bool VisitObjCObjectType(QualType PQ, QualType TQ) {
    const auto *P = PQ->getAs<ObjCObjectType>();
    const auto *T = TQ->getAs<ObjCObjectType>();
    return P->getTypeArgsAsWritten().size() == T->getTypeArgsAsWritten().size();
  }
};

}
#endif
