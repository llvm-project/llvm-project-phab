//===- llvm/Transforms/Utils/VectorVariant.h - Vector utilities -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This header file defines the VectorVariant class and implements the encoding
/// and decoding utilities for VectorVariant objects. Multiple VectorVariant
/// objects can be created (masked, non-masked, etc.) and associated with the
/// original scalar function. These objects are then used to clone new functions
/// that can be vectorized. This class follows the standards defined in the
/// vector function ABI.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_VECTORVARIANT_H
#define LLVM_ANALYSIS_VECTORVARIANT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include <vector>
#include <sstream>
#include <cctype>

using namespace llvm;

namespace llvm {

#define STRIDE_KIND 's'
#define LINEAR_KIND 'l'
#define UNIFORM_KIND 'u'
#define VECTOR_KIND 'v'

#define NOT_ALIGNED 1

#define POSITIVE 1
#define NEGATIVE -1

class VectorKind {

public:
  VectorKind(char K, int S, int A = NOT_ALIGNED) {

    assert((S == notAValue() || K == STRIDE_KIND || K == LINEAR_KIND) &&
           "only linear vectors have strides");

    assert((K != LINEAR_KIND || S != notAValue()) &&
           "linear vectors must have a stride");

    assert((K != STRIDE_KIND || S != notAValue()) &&
           "variable stride vectors must have a stride");

    assert((K != STRIDE_KIND || S >= 0) &&
           "variable stride position must be non-negative");

    assert(A > 0 && "alignment must be positive");

    Kind = K;
    Stride = S;
    Alignment = A;
  }

  VectorKind(const VectorKind &Other) {
    Kind = Other.Kind;
    Stride = Other.Stride;
    Alignment = Other.Alignment;
  }

  /// \brief Is the stride for a linear parameter a uniform variable? (i.e.,
  /// the stride is stored in a variable but is uniform)
  bool isVariableStride() { return Kind == STRIDE_KIND; }

  /// \brief Is the stride for a linear variable non-unit stride?
  bool isNonUnitStride() { return Kind == LINEAR_KIND && Stride != 1; }

  /// \brief Is the stride for a linear variable unit stride?
  bool isUnitStride() { return Kind == LINEAR_KIND && Stride == 1; }

  /// \brief Is this a linear parameter?
  bool isLinear() {
    return isVariableStride() || isNonUnitStride() || isUnitStride();
  }

  /// \brief Is this a uniform parameter?
  bool isUniform() { return Kind == UNIFORM_KIND; }

  /// \brief Is this a vector parameter?
  bool isVector() { return Kind == VECTOR_KIND; }

  /// \brief Is the parameter aligned?
  bool isAligned() { return Alignment != NOT_ALIGNED; }

  /// \brief Get the stride associated with a linear parameter.
  int getStride() { return Stride; }

  /// \brief Get the alignment associated with a linear parameter.
  int getAlignment() { return Alignment; }

  /// \brief Represents a don't care value for strides of parameters other
  /// than linear parameters.
  static int notAValue() { return -1; }

  /// \brief Encode the parameter information into a mangled string
  /// corresponding to the standards defined in the vector function ABI.
  std::string encode() {
    std::stringstream SST;
    SST << Kind;

    if (isNonUnitStride()) {
      if (Stride >= 0)
        SST << Stride;
      else
        SST << "n" << -Stride;
    }

    if (isVariableStride())
      SST << Stride;

    if (isAligned())
      SST << 'a' << Alignment;

    return SST.str();
  }

private:
  char Kind;      // linear, uniform, vector
  int  Stride;
  int  Alignment;
};

class VectorVariant {
private:
  TargetTransformInfo *TTI;
  TargetTransformInfo::ISAClass Isa;
  bool Mask;
  unsigned int Vlen;
  std::vector<VectorKind> Parameters;

  static std::string prefix() { return "_ZGV"; }

public:
  VectorVariant(StringRef FuncName, TargetTransformInfo *TTI);

  /// \brief Get the ISA corresponding to this vector variant.
  TargetTransformInfo::ISAClass getISA() { return Isa; }

  /// \brief Is this a masked vector function variant?
  bool isMasked() { return Mask; }

  /// \brief Get the vector length of the vector variant.
  unsigned int getVlen() { return Vlen; }

  /// \brief Get the parameters of the vector variant.
  std::vector<VectorKind> &getParameters() { return Parameters; }

  /// \brief Build the mangled name for the vector variant. This function
  /// builds a mangled name by including the encodings for the ISA class,
  /// mask information, and all parameters.
  std::string encode() {

    std::stringstream SST;
    SST << prefix() << TTI->encodeISAClass(Isa) << encodeMask(Mask) << Vlen;

    std::vector<VectorKind>::iterator It = Parameters.begin();
    std::vector<VectorKind>::iterator End = Parameters.end();

    if (isMasked())
      End--; // mask parameter is not encoded

    for (; It != End; ++It)
      SST << (*It).encode();

    SST << "_";

    return SST.str();
  }

  /// \brief Generate a function name corresponding to a vector variant.
  std::string generateFunctionName(StringRef ScalarFuncName) {

    static StringRef ManglingPrefix("_Z");
    std::string Name = encode();

    if (ScalarFuncName.startswith(ManglingPrefix))
      return Name + ScalarFuncName.drop_front(ManglingPrefix.size()).str();
    else
      return Name + ScalarFuncName.str();
  }

  /// \brief Some targets do not support particular types, so promote to a type
  /// that is supported.
  Type *promoteToSupportedType(Type *Ty) {
    return TTI->promoteToSupportedType(Ty, getISA());
  }

  /// \brief Check to see if this is a vector variant based on the function
  /// name.
  static bool isVectorVariant(StringRef FuncName) {
    return FuncName.startswith(prefix());
  }

  /// \brief Encode the mask information for the mangled variant name.
  static char encodeMask(bool EncodeMask) {

    switch (EncodeMask) {
    case true:
      return 'M';
    case false:
      return 'N';
    }

    llvm_unreachable("unsupported mask");
  }

  /// \brief Decode the mask information from the mangled variant name.
  static bool decodeMask(char MaskToDecode) {

    switch (MaskToDecode) {
    case 'M':
      return true;
    case 'N':
      return false;
    }

    llvm_unreachable("unsupported mask");
  }

  /// \brief Calculate the vector length for the vector variant.
  unsigned calcVlen(TargetTransformInfo::ISAClass I, Type *Ty);
};

} // llvm namespace

#endif // LLVM_ANALYSIS_VECTORVARIANT_H
