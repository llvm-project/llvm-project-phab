//===--------- VectorVariant.cpp - Vector function ABI -*- C++ -*----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the VectorVariant class and corresponding utilities.
/// VectorVariant objects are associated with a scalar function and are used
/// to generate new functions that can be vectorized. VectorVariants are
/// determined by inspecting the function attributes associated with the scalar
/// function. When a mangled function name is found in the attributes (indicated
/// as "_ZGV"), a VectorVariant object is created. The class and utilities
/// in this file follow the standards defined in the vector function ABI.
///
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/VectorVariant.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

/// \brief Generate a vector variant by decoding the mangled string for the
/// variant contained in the original scalar function's attributes. For
/// example: "_ZGVxN4". The name mangling is defined in the vector function
/// ABI. Based on this string, the parameter kinds (uniform, linear, vector),
/// vector length, parameter alignment, and masking are determined.
VectorVariant::VectorVariant(StringRef FuncName,
                             TargetTransformInfo *TTI) : TTI(TTI) {

  assert(isVectorVariant(FuncName) && "invalid vector variant format");

  std::stringstream SST(FuncName.drop_front(prefix().size()));

  // mandatory annotations
  char EncodedISA;
  SST.get(EncodedISA);
  Isa = TTI->decodeISAClass(EncodedISA);

  char EncodedMask;
  SST.get(EncodedMask);
  Mask = decodeMask(EncodedMask);
  SST >> Vlen;

  // optional parameter annotations
  while (SST.peek() != '_') {

    char Kind;
    int Stride = VectorKind::notAValue();
    int StrideSign = POSITIVE;
    int Alignment = NOT_ALIGNED;

    // Get parameter kind
    SST.get(Kind);

    // Default stride for linear is 1. If the stride for a parameter is 1,
    // then the front-end will not encode it and we will not have set the
    // correct stride below.
    if (Kind == LINEAR_KIND)
      Stride = 1;

    // Handle optional stride
    if (SST.peek() == 'n') {
      // Stride is negative
      SST.ignore(1);
      StrideSign = NEGATIVE;
    }

    if (std::isdigit(SST.peek())) {
      // Extract constant stride
      SST >> Stride;
      assert((Kind != STRIDE_KIND || Stride >= 0) &&
             "variable stride argument index cannot be negative");
    }

    Stride *= StrideSign;
    // Handle optional alignment
    if (SST.peek() == 'a') {
      SST.ignore(1);
      SST >> Alignment;
    }

    VectorKind VecKind(Kind, Stride, Alignment);
    Parameters.push_back(VecKind);
  }

  if (Mask) {
    // Masked variants will have an additional mask parameter
    VectorKind VecKind(VECTOR_KIND, VectorKind::notAValue());
    Parameters.push_back(VecKind);
  }
}

/// \brief Determine the vector variant's vector length based on the
/// characteristic data type defined in the vector function ABI and target
/// vector register width.
unsigned VectorVariant::calcVlen(TargetTransformInfo::ISAClass I,
                                 Type *CharacteristicDataType) {
  assert(CharacteristicDataType &&
         CharacteristicDataType->getPrimitiveSizeInBits() != 0 &&
         "expected characteristic data type to have a primitive size in bits");

  unsigned VectorRegisterSize =
      TTI->maximumSizeofISAClassVectorRegister(I, CharacteristicDataType);

  return VectorRegisterSize / CharacteristicDataType->getPrimitiveSizeInBits();
}
