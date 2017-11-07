//===- ValueLattice.cpp - Value constraint analysis -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/ValueLattice.h"


namespace llvm {


static DenseMap<ConstantRange, unsigned>  ConstantRangePool;


ConstantRange* addConstantRange(ConstantRange CR, DenseMap<ConstantRange, unsigned> &CRP) {
  auto I = CRP.insert({CR, 1});
  if (!I.second)
    I.first->second += 1;
  return const_cast<ConstantRange*>(&I.first->first);
}


raw_ostream &operator<<(raw_ostream &OS, const ValueLatticeElement &Val) {
  if (Val.isUndefined())
    return OS << "undefined";
  if (Val.isOverdefined())
    return OS << "overdefined";

  if (Val.isNotConstant())
    return OS << "notconstant<" << *Val.getNotConstant() << ">";
  if (Val.isConstantRange())
    return OS << "constantrange<" << Val.getConstantRange().getLower() << ", "
              << Val.getConstantRange().getUpper() << ">";
  return OS << "constant<" << *Val.getConstant() << ">";
}
} // end namespace llvm
