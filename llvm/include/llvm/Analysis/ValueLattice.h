//===- ValueLattice.h - Value constraint analysis ---------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_VALUELATTICE_H
#define LLVM_ANALYSIS_VALUELATTICE_H

#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
//
//===----------------------------------------------------------------------===//
//                               ValueLatticeElement
//===----------------------------------------------------------------------===//

/// This class represents lattice values for constants.
///
/// FIXME: This is basically just for bringup, this can be made a lot more rich
/// in the future.
///

namespace llvm {

constexpr size_t const_min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

// We use this trait to set NumLowBitsAvailable based on the alignment of
// Constant and ConstantRange.
struct ConstantAndConstantRangePointerTraits {
  static inline void *getAsVoidPointer(void *P) { return P; }

  static inline void *getFromVoidPointer(void *P) {
    return P;
  }

  enum { NumLowBitsAvailable =
             const_min(detail::ConstantLog2<alignof(Constant)>::value,
                       detail::ConstantLog2<alignof(ConstantRange)>::value)  };
};



/// Provide DenseMapInfo for ConstantRange
template<> struct DenseMapInfo<ConstantRange> {
  static inline ConstantRange getEmptyKey() {
    return ConstantRange(DenseMapAPIntKeyInfo::getEmptyKey(),
                         DenseMapAPIntKeyInfo::getEmptyKey());
  }

  static inline ConstantRange getTombstoneKey() {
    return ConstantRange(DenseMapAPIntKeyInfo::getEmptyKey(),
                         DenseMapAPIntKeyInfo::getTombstoneKey());
  }

  static unsigned getHashValue(const ConstantRange &V) {
    return static_cast<unsigned>(
        hash_combine(hash_value(V.getLower()), hash_value(V.getUpper())));
  }

  static bool isEqual(const ConstantRange &LHS,
                      const ConstantRange &RHS) {
    return LHS.getBitWidth() == RHS.getBitWidth() && LHS == RHS;
  }
};


ConstantRange* addConstantRange(ConstantRange CR, DenseMap<ConstantRange, unsigned> &CRP);

class ValueLatticeElement {
  enum ValueLatticeElementTy {
    /// This Value has no known value yet.  As a result, this implies the
    /// producing instruction is dead.  Caution: We use this as the starting
    /// state in our local meet rules.  In this usage, it's taken to mean
    /// "nothing known yet".
    undefined,

    /// This Value has a specific constant value.  (For constant integers,
    /// constantrange is used instead.  Integer typed constantexprs can appear
    /// as constant.)
    constant,

    /// This Value is known to not have the specified value.  (For constant
    /// integers, constantrange is used instead.  As above, integer typed
    /// constantexprs can appear here.)
    notconstant,

    /// The Value falls within this range. (Used only for integer typed values.)
    constantrange,

    /// We can not precisely model the dynamic values this value might take.
    overdefined
  };

  /// Val: This stores the current lattice value along with the Constant* for
  /// the constant if this is a 'constant' or 'notconstant' value.
  PointerIntPair<void*, 3, unsigned,
                 ConstantAndConstantRangePointerTraits> ValuePtr;

public:
  ValueLatticeElement() : ValuePtr(nullptr, undefined) {}

  static ValueLatticeElement get(Constant *C,
                                 DenseMap<ConstantRange, unsigned> &CRP) {
    ValueLatticeElement Res;
    if (!isa<UndefValue>(C))
      Res.markConstant(C, CRP);
    return Res;
  }
  static ValueLatticeElement getNot(Constant *C,
                                    DenseMap<ConstantRange, unsigned> &CRP ) {
    ValueLatticeElement Res;
    if (!isa<UndefValue>(C))
      Res.markNotConstant(C, CRP);
    return Res;
  }
  static ValueLatticeElement getRange(ConstantRange CR,
                                      DenseMap<ConstantRange, unsigned> &CRP ) {
    ValueLatticeElement Res;
    Res.markConstantRange(CR, CRP);
    return Res;
  }
  static ValueLatticeElement getOverdefined() {
    ValueLatticeElement Res;
    Res.markOverdefined();
    return Res;
  }

  unsigned getTag() const { return ValuePtr.getInt(); }
  bool isUndefined() const { return getTag() == undefined; }
  bool isConstant() const { return getTag() == constant; }
  bool isNotConstant() const { return getTag() == notconstant; }
  bool isConstantRange() const { return getTag() == constantrange; }
  bool isOverdefined() const { return getTag() == overdefined; }

  Constant *getConstant() const {
    assert(isConstant() && "Cannot get the constant of a non-constant!");
    return static_cast<Constant*>(ValuePtr.getPointer());
  }

  Constant *getNotConstant() const {
    assert(isNotConstant() && "Cannot get the constant of a non-notconstant!");
    return static_cast<Constant*>(ValuePtr.getPointer());
  }

  const ConstantRange &getConstantRange() const {
    assert(isConstantRange() &&
           "Cannot get the constant-range of a non-constant-range!");
    return *static_cast<ConstantRange*>(ValuePtr.getPointer());
  }

  Optional<APInt> asConstantInteger() const {
    if (isConstant() && isa<ConstantInt>(getConstant())) {
      return cast<ConstantInt>(getConstant())->getValue();
    } else if (isConstantRange() && getConstantRange().isSingleElement()) {
      return *getConstantRange().getSingleElement();
    }
    return None;
  }

private:
  void markOverdefined() {
    if (isOverdefined())
      return;
    ValuePtr.setInt(overdefined);
  }

  void markConstant(Constant *V, DenseMap<ConstantRange, unsigned> &CRP) {
    assert(V && "Marking constant with NULL");
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
      markConstantRange(ConstantRange(CI->getValue()), CRP);
      return;
    }
    if (isa<UndefValue>(V))
      return;

    assert((!isConstant() || getConstant() == V) &&
           "Marking constant with different value");
    assert(isUndefined());
    ValuePtr.setInt(constant);
    ValuePtr.setPointer(static_cast<void*>(V));
  }

  void markNotConstant(Constant *V, DenseMap<ConstantRange, unsigned> &CRP) {
    assert(V && "Marking constant with NULL");
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
      markConstantRange(ConstantRange(CI->getValue() + 1, CI->getValue()), CRP);
      return;
    }
    if (isa<UndefValue>(V))
      return;

    assert((!isConstant() || getConstant() != V) &&
           "Marking constant !constant with same value");
    assert((!isNotConstant() || getNotConstant() == V) &&
           "Marking !constant with different value");
    assert(isUndefined() || isConstant());
    ValuePtr.setInt(notconstant);
    ValuePtr.setPointer(static_cast<void*>(V));
  }

  void markConstantRange(ConstantRange NewR,
                         DenseMap<ConstantRange, unsigned> &CRP) {
    if (isConstantRange()) {
      if (NewR.isEmptySet())
        markOverdefined();
      else {
        ValuePtr.setPointer(static_cast<void*>(addConstantRange(NewR, CRP)));
      }
      return;
    }

    assert(isUndefined());
    if (NewR.isEmptySet())
      markOverdefined();
    else {
      ValuePtr.setInt(constantrange);
      ValuePtr.setPointer(static_cast<void*>(addConstantRange(NewR, CRP)));
    }
  }

public:
  /// Updates this object to approximate both this object and RHS. Returns
  /// true if this object has been changed.
  bool mergeIn(const ValueLatticeElement &RHS, const DataLayout &DL,
               DenseMap<ConstantRange, unsigned> &CRP) {
    if (RHS.isUndefined() || isOverdefined())
      return false;
    if (RHS.isOverdefined()) {
      markOverdefined();
      return true;
    }

    if (isUndefined()) {
      *this = RHS;
      return !RHS.isUndefined();
    }

    if (isConstant()) {
      if (RHS.isConstant() && getConstant() == RHS.getConstant())
        return false;
      markOverdefined();
      return true;
    }

    if (isNotConstant()) {
      if (RHS.isNotConstant() && getNotConstant() == RHS.getNotConstant())
        return false;
      markOverdefined();
      return true;
    }

    assert(isConstantRange() && "New ValueLattice type?");
    if (!RHS.isConstantRange()) {
      // We can get here if we've encountered a constantexpr of integer type
      // and merge it with a constantrange.
      markOverdefined();
      return true;
    }
    ConstantRange NewR = getConstantRange().unionWith(RHS.getConstantRange());
    if (NewR.isFullSet())
      markOverdefined();
    else
      markConstantRange(std::move(NewR), CRP);
    return true;
  }

  ConstantInt *getConstantInt() const {
    assert(isConstant() && isa<ConstantInt>(getConstant()) &&
           "No integer constant");
    return cast<ConstantInt>(getConstant());
  }

  bool satisfiesPredicate(CmpInst::Predicate Pred,
                          const ValueLatticeElement &Other) const {
    // TODO: share with LVI getPredicateResult.

    if (isUndefined() || Other.isUndefined())
      return true;

    if (isConstant() && Other.isConstant() && Pred == CmpInst::FCMP_OEQ)
      return getConstant() == Other.getConstant();

    // Integer constants are represented as ConstantRanges with single
    // elements.
    if (!isConstantRange() || !Other.isConstantRange())
      return false;

    const auto &CR = getConstantRange();
    const auto &OtherCR = Other.getConstantRange();
    return ConstantRange::makeSatisfyingICmpRegion(Pred, OtherCR).contains(CR);
  }
};

raw_ostream &operator<<(raw_ostream &OS, const ValueLatticeElement &Val);

} // end namespace llvm
#endif
