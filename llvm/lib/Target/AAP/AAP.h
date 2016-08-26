//===-- AAP.h - Top-level interface for AAP representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// AAP back-end.
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_AAP_H
#define TARGET_AAP_H

#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"

namespace AAPCC {
// AAP specific condition codes
enum CondCode {
  COND_EQ = 0,
  COND_NE = 1,
  COND_LTS = 2,
  COND_LES = 3,
  COND_LTU = 4,
  COND_LEU = 5,
  COND_INVALID = -1
};
} // end namespace AAP

namespace llvm {

namespace AAP {
// Various helper methods to define operand ranges used throughout the backend
static bool inline isImm3(int64_t I) { return isUInt<3>(I); }
static bool inline isImm6(int64_t I) { return isUInt<6>(I); }
static bool inline isImm9(int64_t I) { return isUInt<9>(I); }
static bool inline isImm10(int64_t I) { return isUInt<10>(I); }
static bool inline isImm12(int64_t I) { return isUInt<12>(I); }

static bool inline isOff3(int64_t I) { return isInt<3>(I); }
static bool inline isOff6(int64_t I) { return isInt<6>(I); }
static bool inline isOff9(int64_t I) { return isInt<9>(I); }
static bool inline isOff10(int64_t I) { return isInt<10>(I); }

static bool inline isField16(int64_t I) {
  return isInt<16>(I) || isUInt<16>(I);
}

static bool inline isShiftImm3(int64_t I) { return (I >= 1) && (I <= 8); }
static bool inline isShiftImm6(int64_t I) { return (I >= 1) && (I <= 64); }

} // end of namespace AAP
} // end namespace llvm

#endif
