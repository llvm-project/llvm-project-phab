//===- llvm/MC/LaneBitmask.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// A common definition of LaneBitmask for use in TableGen and CodeGen.
///
/// A lane mask is a bitmask representing the covering of a register with
/// sub-registers.
///
/// This is typically used to track liveness at sub-register granularity.
/// Lane masks for sub-register indices are similar to register units for
/// physical registers. The individual bits in a lane mask can't be assigned
/// any specific meaning. They can be used to check if two sub-register
/// indices overlap.
///
/// Iff the target has a register such that:
///
///   getSubReg(Reg, A) overlaps getSubReg(Reg, B)
///
/// then:
///
///   (getSubRegIndexLaneMask(A) & getSubRegIndexLaneMask(B)) != 0

#ifndef LLVM_MC_LANEBITMASK_H
#define LLVM_MC_LANEBITMASK_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Printable.h"
#include "llvm/Support/raw_ostream.h"
#include <array>

namespace llvm {

  struct LaneBitmask;
  static LLVM_ATTRIBUTE_UNUSED Printable PrintLaneMask(LaneBitmask);
  static LLVM_ATTRIBUTE_UNUSED Printable PrintLaneMaskAsInitList(LaneBitmask);

  struct LaneBitmask {
    // When changing the underlying type, change the format string as well.
    static const unsigned N = 1;
    using Type = std::array<unsigned, N>;
    enum : unsigned { BitWidth = 8*N*sizeof(unsigned) };
    constexpr static const char *const FormatStr = "%08X";

    constexpr LaneBitmask() = default;
    explicit constexpr LaneBitmask(Type V) : Mask(V) {}

    bool operator== (LaneBitmask M) const { return Mask == M.Mask; }
    bool operator!= (LaneBitmask M) const { return Mask != M.Mask; }
    bool operator< (LaneBitmask M)  const { return Mask < M.Mask; }
    bool none() const {
      for (unsigned I = 0; I != N; ++I)
        if (Mask[I] != 0)
          return false;
      return true;
    }
    bool any() const {
      for (unsigned I = 0; I != N; ++I)
        if (Mask[I] != 0)
          return true;
      return false;
    }
    bool all() const {
      for (unsigned I = 0; I != N; ++I)
        if (Mask[I] != ~0u)
          return false;
      return true;
    }

    LaneBitmask operator~() const {
      Type T;
      for (unsigned I = 0; I != N; ++I)
        T[I] = ~Mask[I];
      return LaneBitmask(T);
    }
    LaneBitmask operator|(LaneBitmask M) const {
      Type T;
      for (unsigned I = 0; I != N; ++I)
        T[I] = Mask[I] | M.Mask[I];
      return LaneBitmask(T);
    }
    LaneBitmask operator&(LaneBitmask M) const {
      Type T;
      for (unsigned I = 0; I != N; ++I)
        T[I] = Mask[I] & M.Mask[I];
      return LaneBitmask(T);
    }
    LaneBitmask &operator|=(LaneBitmask M) {
      for (unsigned I = 0; I != N; ++I)
        Mask[I] |= M.Mask[I];
      return *this;
    }
    LaneBitmask &operator&=(LaneBitmask M) {
      for (unsigned I = 0; I != N; ++I)
        Mask[I] &= M.Mask[I];
      return *this;
    }

    LaneBitmask rol(unsigned S) const {
      if (S == 0)
        return *this;
      Type T;

      // Rotate words first.
      unsigned W = S/32;
      for (unsigned I = W; I != N; ++I)
        T[I-W] = Mask[I];
      for (unsigned I = 0; I != W; ++I)
        T[I+W] = Mask[I];

      S = S % 32;
      if (S != 0) {
        // Rotate bits.
        unsigned M0 = T[0];
        for (unsigned I = 0; I != N-1; ++I)
          T[I] = (T[I] << S) | (T[I+1] >> (32-S));
        T[N-1] = (T[N-1] << S) | (M0 >> (32-S));
      }

      return LaneBitmask(T);
    }

    unsigned getNumLanes() const {
      unsigned S = 0;
      for (unsigned M : Mask)
        S += countPopulation(M);
      return S;
    }
    unsigned getHighestLane() const {
      for (unsigned I = N; I != 0; --I) {
        if (Mask[I-1] == 0)
          continue;
        return Log2_32(Mask[I-1]) + 8*sizeof(unsigned)*(I-1);
      }
      return -1u;
    }

    static LaneBitmask getNone() { return LaneBitmask(); }
    static LaneBitmask getAll()  { return ~LaneBitmask(); }
    static LaneBitmask getLane(unsigned Lane) {
      Type T;
      T[Lane / 32] = 1u << (Lane % 32);
      return LaneBitmask(T);
    }

  private:
    friend Printable PrintLaneMask(LaneBitmask LaneMask);
    friend Printable PrintLaneMaskAsInitList(LaneBitmask LaneMask);
    Type Mask = {{0}};
  };

  /// Create Printable objects to print LaneBitmasks on a \ref raw_ostream.
  static LLVM_ATTRIBUTE_UNUSED Printable PrintLaneMask(LaneBitmask LaneMask) {
    return Printable([LaneMask](raw_ostream &OS) {
      for (unsigned I = LaneBitmask::N; I != 0; --I)
        OS << format(LaneBitmask::FormatStr, LaneMask.Mask[I-1]);
    });
  }

  static LLVM_ATTRIBUTE_UNUSED Printable
  PrintLaneMaskAsInitList(LaneBitmask LaneMask) {
    return Printable([LaneMask](raw_ostream &OS) {
      OS << "{{";
      for (unsigned I = 0; I != LaneBitmask::N; ++I) {
        if (I != 0)
          OS << ',';
        OS << "0x" << format(LaneBitmask::FormatStr, LaneMask.Mask[I]);
      }
      OS << "}}";
    });
  }

} // end namespace llvm

#endif // LLVM_MC_LANEBITMASK_H
