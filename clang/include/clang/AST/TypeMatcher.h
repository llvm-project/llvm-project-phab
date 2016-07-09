//===--- TypeMatcher.h - Visitor for Type subclasses ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the TypeMatcher interface, which recursively compares
//  two types.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_TYPEMATCHER_H
#define LLVM_CLANG_AST_TYPEMATCHER_H

#include "clang/AST/Type.h"

namespace clang {

// A helper macro to implement short-circuiting when recursing.  It invokes
// CALL_EXPR, which must be a method call, on the derived object.
#define TRY_TO(CALL_EXPR)                                                      \
  do {                                                                         \
    if (!getDerived().CALL_EXPR)                                               \
      return false;                                                            \
  } while (0)


/// \brief A visitor that traverses one type, a "pattern" and compares it with
/// another type which is traversed synchronously.
///
/// The visitor is used much like other visitor classes, for instance,
/// RecursiveASTVisitor. The necessary functionality is implemented in a new
/// class that must be inherited from this template according to Curiously
/// Recurring Template Pattern:
///
/// \code
///     class AMatcher : public TypeMatcher<AMatcher> {
///       ...
///     };
/// \endcode
///
/// The derived class overrides methods declared in TypeMatcher to implement the
/// necessary behavior.
///
/// Most of matching is made by miscellaneous Visit methods that are called for
/// pairs of QualTypes found in type hierarchy. There are Visit methods for each
/// subclass of Type, including abstract types. So, for a particular pattern
/// type usually there are several Visit methods that can be called. Other
/// methods of this visitor defines which Visit methods are called and in what
/// order.
///
/// Pattern matching starts by calling method TraverseType, which implements
/// polymorphic operation on an a pair of objects of type derived from Type:
///
/// \code
///     TraverseType(PatternType, EvaluatedType)
/// \endcode
///
/// The function returns true if \c EvaluatedType matches \c PatternType, false
/// otherwise.
///
/// Depending on actual type of \c PatternType this function dispatches call to
/// function that processes particular type, for instance
/// \c TraverseConstantArrayType, if the pattern type is \c ConstantArrayType.
/// This function first calls function \c WalkUpFromConstantArrayType, which
/// calls appropriate Visit methods. Then it recursively calls itself on
/// components of the pattern type.
///
/// Function \c WalkUpFrom* exists for every type. It calls Visit methods for
/// the particular pattern type. Default implementation makes calls the starting
/// from the most general type, so if \c PatternType is ConstantArrayType, then
/// sequence of calls is as follows:
///
/// \li VisitType
/// \li VisitArrayType
/// \li VisitConstantArrayType
///
/// If any of \c Visit calls returns false, types do not match, any other checks
/// are not made.
///
template<typename ImplClass>
class TypeMatcher {
public:

  // Entry point of matcher.
  bool match(QualType PatternT, QualType InstT) {
    return TraverseType(PatternT.getCanonicalType(), InstT.getCanonicalType());
  }

  // Dispatches call to particular Traverse* method.
  bool TraverseType(QualType PatternT, QualType InstT) {
    switch (PatternT->getTypeClass()) {
#define ABSTRACT_TYPE(CLASS, BASE)
#define TYPE(CLASS, BASE)                                                      \
  case Type::CLASS:                                                            \
    return getDerived().Traverse##CLASS##Type(PatternT, InstT);
#include "clang/AST/TypeNodes.def"
    default:
      llvm_unreachable("Unknown type class!");
    }
    return false;
  }

  // Declare Traverse*() for all concrete Type classes.
#define ABSTRACT_TYPE(CLASS, BASE)
#define TYPE(CLASS, BASE) bool Traverse##CLASS##Type(QualType P, QualType T);
#include "clang/AST/TypeNodes.def"

  // Define WalkUpFrom*() for all Type classes.
#define TYPE(CLASS, BASE)                                                      \
  bool WalkUpFrom##CLASS##Type(QualType P, QualType S) {                       \
    TRY_TO(WalkUpFrom##BASE(P, S));                                            \
    TRY_TO(Visit##CLASS##Type(P, S));                                          \
    return true;                                                               \
  }
#include "clang/AST/TypeNodes.def"

  // Generic version of WalkUpFrom*, which is called first for every type
  // matching.
  bool WalkUpFromType(QualType PatternT, QualType T) {
    return getDerived().VisitType(PatternT, T);
  }

  // Define default Visit*() for all Type classes.
#define TYPE(CLASS, BASE)                                                      \
  bool Visit##CLASS##Type(QualType P, QualType T) {                            \
    return P->getTypeClass() == T->getTypeClass();                             \
  }
#include "clang/AST/TypeNodes.def"

  // Generic version of Visit*, it is called first for every type matching.
  bool VisitType(QualType PatternT, QualType T) {
    return true;
  }

  /// \brief Return a reference to the derived class.
  ImplClass &getDerived() { return *static_cast<ImplClass *>(this); }

  bool TraverseTemplateName(const TemplateName &PN, const TemplateName &TN) {
    return PN.getKind() == TN.getKind();
  }

  bool TraverseTemplateArguments(const TemplateArgument *PArgs, unsigned PNum,
                                 const TemplateArgument *SArgs, unsigned SNum) {
    if (PNum != SNum)
      return false;
    for (unsigned I = 0; I != PNum; ++I) {
      TRY_TO(TraverseTemplateArgument(PArgs[I], SArgs[I]));
    }
    return true;
  }

  bool TraverseTemplateArgument(const TemplateArgument &PArg,
                                const TemplateArgument &SArg) {
    if (PArg.getKind() != SArg.getKind())
      return false;
    switch (PArg.getKind()) {
    case TemplateArgument::Null:
    case TemplateArgument::Declaration:
    case TemplateArgument::Integral:
    case TemplateArgument::NullPtr:
      return true;

    case TemplateArgument::Type:
      return getDerived().TraverseType(PArg.getAsType(), SArg.getAsType());

    case TemplateArgument::Template:
    case TemplateArgument::TemplateExpansion:
      return getDerived().TraverseTemplateName(
        PArg.getAsTemplateOrTemplatePattern(),
        SArg.getAsTemplateOrTemplatePattern());

    case TemplateArgument::Expression:
      return getDerived().TraverseStmt(PArg.getAsExpr(), SArg.getAsExpr());

    case TemplateArgument::Pack:
      return getDerived().TraverseTemplateArguments(PArg.pack_begin(),
                         PArg.pack_size(), SArg.pack_begin(), SArg.pack_size());
    }

    return true;
  }

  bool TraverseNestedNameSpecifier(NestedNameSpecifier *PNNS,
                                   NestedNameSpecifier *SNNS) {
    if (!PNNS)
      return !SNNS;
    if (!SNNS)
      return false;

    TRY_TO(TraverseNestedNameSpecifier(PNNS->getPrefix(), SNNS->getPrefix()));

    if (PNNS->getKind() != SNNS->getKind())
      return false;
    switch (PNNS->getKind()) {
    case NestedNameSpecifier::Identifier:
    case NestedNameSpecifier::Namespace:
    case NestedNameSpecifier::NamespaceAlias:
    case NestedNameSpecifier::Global:
    case NestedNameSpecifier::Super:
      return true;

    case NestedNameSpecifier::TypeSpec:
    case NestedNameSpecifier::TypeSpecWithTemplate:
      TRY_TO(TraverseType(QualType(PNNS->getAsType(), 0),
                          QualType(SNNS->getAsType(), 0)));
    }

    return true;
  }

  // This function in fact compares expressions, that may be found as a part of
  // type definition (as in VariableArrayType for instance).
  bool TraverseStmt(Stmt *P, Stmt *S) {
    if (!P || !S)
      return !P == !S;
    Expr *PE = cast<Expr>(P);
    Expr *SE = cast<Expr>(S);
    return TraverseType(PE->getType(), SE->getType());
  }
};

// Implementation of Traverse* methods.
//
// These methods are obtained from corresponding definitions of
// RecursiveASTVisitor almost mechanically.  The main difference is that
// Traverse* functions get arguments in pairs: for pattern type and for sample.

// Defines method TypeMatcher<Derived>::Traverse* for the given TYPE.
// This macro makes available variables P and T, that represent pattern type
// class and sample type class respectively.
#define DEF_TRAVERSE_TYPE(TYPE, CODE)                                          \
  template <typename Derived>                                                  \
  bool TypeMatcher<Derived>::Traverse##TYPE(QualType PQ, QualType TQ) {        \
    if (!WalkUpFrom##TYPE(PQ, TQ))                                             \
      return false;                                                            \
    const TYPE *P = cast<TYPE>(PQ.getTypePtr());                               \
    const TYPE *T = cast<TYPE>(TQ.getTypePtr());                               \
    (void)P; (void)T;                                                          \
    CODE;                                                                      \
    return true;                                                               \
  }

DEF_TRAVERSE_TYPE(BuiltinType, {})

DEF_TRAVERSE_TYPE(ComplexType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(PointerType, {
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType()));
})

DEF_TRAVERSE_TYPE(BlockPointerType, {
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType()));
})

DEF_TRAVERSE_TYPE(LValueReferenceType, {
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType()));
})

DEF_TRAVERSE_TYPE(RValueReferenceType, {
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType()));
})

DEF_TRAVERSE_TYPE(MemberPointerType, {
  TRY_TO(TraverseType(QualType(P->getClass(), 0), QualType(T->getClass(), 0)));
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType()));
})

DEF_TRAVERSE_TYPE(AdjustedType, {
  TRY_TO(TraverseType(P->getOriginalType(), T->getOriginalType()));
})

DEF_TRAVERSE_TYPE(DecayedType, {
  TRY_TO(TraverseType(P->getOriginalType(), T->getOriginalType()));
})

DEF_TRAVERSE_TYPE(ConstantArrayType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(IncompleteArrayType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(VariableArrayType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
  TRY_TO(TraverseStmt(P->getSizeExpr(), T->getSizeExpr()));
})

DEF_TRAVERSE_TYPE(DependentSizedArrayType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
  TRY_TO(TraverseStmt(P->getSizeExpr(), T->getSizeExpr()));
})

DEF_TRAVERSE_TYPE(DependentSizedExtVectorType, {
  TRY_TO(TraverseStmt(P->getSizeExpr(), T->getSizeExpr()));
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(VectorType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(ExtVectorType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

DEF_TRAVERSE_TYPE(FunctionNoProtoType, {
  TRY_TO(TraverseType(P->getReturnType(), T->getReturnType()));
})

DEF_TRAVERSE_TYPE(FunctionProtoType, {
  // Number of parameters must be equal for pattern and sample types, otherwise
  // we cannot compare these types in pairs. This check must be done by some of
  // Visit* methods. The same is for number of exceptions.
  assert(P->getNumParams() == T->getNumParams());
  assert(P->exceptions().size() == T->exceptions().size());

  TRY_TO(TraverseType(P->getReturnType(), T->getReturnType()));

  for (unsigned I = 0; I < P->getNumParams(); ++I) {
    TRY_TO(TraverseType(P->getParamType(I), T->getParamType(I)));
  }

  for (unsigned I = 0; I < P->exceptions().size(); ++I) {
    TRY_TO(TraverseType(P->exceptions()[I], T->exceptions()[I]));
  }

  TRY_TO(TraverseStmt(P->getNoexceptExpr(), T->getNoexceptExpr()));
})

DEF_TRAVERSE_TYPE(UnresolvedUsingType, {})
DEF_TRAVERSE_TYPE(TypedefType, {})

DEF_TRAVERSE_TYPE(TypeOfExprType, {
  TRY_TO(TraverseStmt(P->getUnderlyingExpr(), T->getUnderlyingExpr()));
})

DEF_TRAVERSE_TYPE(TypeOfType, {
  TRY_TO(TraverseType(P->getUnderlyingType(), T->getUnderlyingType()));
})

DEF_TRAVERSE_TYPE(DecltypeType, {
  TRY_TO(TraverseStmt(P->getUnderlyingExpr(), T->getUnderlyingExpr()));
})

DEF_TRAVERSE_TYPE(UnaryTransformType, {
  TRY_TO(TraverseType(P->getBaseType(), T->getBaseType()));
  TRY_TO(TraverseType(P->getUnderlyingType(), T->getUnderlyingType()));
})

DEF_TRAVERSE_TYPE(AutoType, {
  TRY_TO(TraverseType(P->getDeducedType(), T->getDeducedType()));
})

DEF_TRAVERSE_TYPE(RecordType, {})
DEF_TRAVERSE_TYPE(EnumType, {})
DEF_TRAVERSE_TYPE(TemplateTypeParmType, {})
DEF_TRAVERSE_TYPE(SubstTemplateTypeParmType, {})
DEF_TRAVERSE_TYPE(SubstTemplateTypeParmPackType, {})

DEF_TRAVERSE_TYPE(TemplateSpecializationType, {
  TRY_TO(TraverseTemplateName(P->getTemplateName(), T->getTemplateName()));
  TRY_TO(TraverseTemplateArguments(P->getArgs(), P->getNumArgs(),
                                   T->getArgs(), T->getNumArgs()));
})

DEF_TRAVERSE_TYPE(InjectedClassNameType, {})

DEF_TRAVERSE_TYPE(AttributedType, {
  TRY_TO(TraverseType(P->getModifiedType(), T->getModifiedType()));
})

DEF_TRAVERSE_TYPE(ParenType, {
  TRY_TO(TraverseType(P->getInnerType(), T->getInnerType()));
})

DEF_TRAVERSE_TYPE(ElaboratedType, {
  TRY_TO(TraverseNestedNameSpecifier(P->getQualifier(), T->getQualifier()));
  TRY_TO(TraverseType(P->getNamedType(), T->getNamedType()));
})

DEF_TRAVERSE_TYPE(DependentNameType, {
  TRY_TO(TraverseNestedNameSpecifier(P->getQualifier(), T->getQualifier()));
})

DEF_TRAVERSE_TYPE(DependentTemplateSpecializationType, {
  TRY_TO(TraverseNestedNameSpecifier(P->getQualifier(), T->getQualifier()));
  TRY_TO(TraverseTemplateArguments(P->getArgs(), P->getNumArgs(),
                                   T->getArgs(), T->getNumArgs()));
})

DEF_TRAVERSE_TYPE(PackExpansionType, {
  TRY_TO(TraverseType(P->getPattern(), T->getPattern()));
})

DEF_TRAVERSE_TYPE(ObjCInterfaceType, {})

DEF_TRAVERSE_TYPE(ObjCObjectType, {
  assert(P->getTypeArgsAsWritten().size() == T->getTypeArgsAsWritten().size());
    return false;
  TRY_TO(TraverseType(P->getBaseType(), T->getBaseType()));
  for (unsigned I = 0, E = P->getTypeArgsAsWritten().size(); I != E; ++I) {
    TRY_TO(TraverseType(P->getTypeArgsAsWritten()[I],
                        T->getTypeArgsAsWritten()[I]));
  }
})

DEF_TRAVERSE_TYPE(ObjCObjectPointerType, {
  TRY_TO(TraverseType(P->getPointeeType(), T->getPointeeType())); })

DEF_TRAVERSE_TYPE(AtomicType, {
  TRY_TO(TraverseType(P->getValueType(), T->getValueType()));
})

DEF_TRAVERSE_TYPE(PipeType, {
  TRY_TO(TraverseType(P->getElementType(), T->getElementType()));
})

#undef DEF_TRAVERSE_TYPE

}
#endif
