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
  // Targets defined in the vector function ABI.
  enum TargetProcessor {
    Pentium4,      // ISA extension = SSE2,     ISA class = XMM
    Pentium4SSE3,  // ISA extension = SSE3,     ISA class = XMM
    Core2DuoSSSE3, // ISA extension = SSSE3,    ISA class = XMM
    Core2DuoSSE41, // ISA extension = SSE4_1,   ISA class = XMM
    CoreI7SSE42,   // ISA extension = SSE4_2,   ISA class = XMM
    Core2ndGenAVX, // ISA extension = AVX,      ISA class = YMM1
    Core3rdGenAVX, // ISA extension = AVX,      ISA class = YMM1
    Core4thGenAVX, // ISA extension = AVX2,     ISA class = YMM2
    Mic,           // ISA extension = Xeon Phi, ISA class = MIC(ZMM)
    FutureCpu22,   // ISA extension = AVX512,   ISA class = ZMM
    FutureCpu23,   // ISA extension = AVX512,   ISA class = ZMM
  };

  // ISA classes defined in the vector function ABI.
  enum ISAClass {
    XMM,  // (SSE2)
    YMM1, // (AVX1)
    YMM2, // (AVX2)
    ZMM,  // (MIC)
    ISAClassesNum
  };

  ISAClass Isa;
  bool Mask;
  unsigned int Vlen;
  std::vector<VectorKind> Parameters;

  static std::string prefix() { return "_ZGV"; }

  /// \brief Determine the maximum vector register width based on the ISA classes
  /// defined in the vector function ABI.
  static unsigned int maximumSizeofISAClassVectorRegister(ISAClass I, Type *Ty);

public:
  VectorVariant(ISAClass I, bool M, unsigned int V,
                const std::vector<VectorKind> &P)
      : Isa(I), Mask(M), Vlen(V), Parameters(P) {
    if (Mask) {
      // Masked variants will have an additional mask parameter
      VectorKind VKind(VECTOR_KIND, VectorKind::notAValue());
      Parameters.push_back(VKind);
    }
  }

  VectorVariant(const VectorVariant &Other)
      : Isa(Other.Isa), Mask(Other.Mask), Vlen(Other.Vlen),
        Parameters(Other.Parameters) {}

  VectorVariant(StringRef FuncName);

  /// \brief Get the ISA corresponding to this vector variant.
  ISAClass getISA() { return Isa; }

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
    SST << prefix() << encodeISAClass(Isa) << encodeMask(Mask) << Vlen;

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
    return promoteToSupportedType(Ty, *this);
  }

  /// \brief Check to see if this is a vector variant based on the function
  /// name.
  static bool isVectorVariant(StringRef FuncName) {
    return FuncName.startswith(prefix());
  }

  /// \brief Some targets do not support particular types, so promote to a type
  /// that is supported.
  static Type *promoteToSupportedType(Type *Ty, VectorVariant &Variant) {
    ISAClass I;

    I = Variant.getISA();
    // On ZMM promote char and short to int
    if (I == ISAClass::ZMM && (Ty->isIntegerTy(8) || Ty->isIntegerTy(16)))
      return Type::getInt32Ty(Ty->getContext());

    return Ty;
  }

  /// \brief Get the ISA class corresponding to a particular target processor.
  static ISAClass targetProcessorISAClass(TargetProcessor Target) {

    switch (Target) {
    case Pentium4:
    case Pentium4SSE3:
    case Core2DuoSSSE3:
    case Core2DuoSSE41:
    case CoreI7SSE42:
      return XMM;
    case Core2ndGenAVX:
    case Core3rdGenAVX:
      return YMM1;
    case Core4thGenAVX:
      return YMM2;
    case Mic:
    case FutureCpu22:
    case FutureCpu23:
      return ZMM;
    default:
      llvm_unreachable("unsupported target processor");
      return XMM;
    }
  }

  /// \brief Get an ISA class string based on ISA class enum.
  static std::string ISAClassToString(ISAClass IsaClass) {
    switch (IsaClass) {
    case XMM:
      return "XMM";
    case YMM1:
      return "YMM1";
    case YMM2:
      return "YMM2";
    case ZMM:
      return "ZMM";
    default:
      assert(false && "unsupported ISA class");
      return "?";
    }
  }

  /// \brief Get an ISA class enum based on ISA class string.
  static ISAClass ISAClassFromString(std::string IsaClass) {
    if (IsaClass == "XMM")
      return XMM;
    if (IsaClass == "YMM1")
      return YMM1;
    if (IsaClass == "YMM2")
      return YMM2;
    if (IsaClass == "ZMM")
      return ZMM;
    assert(false && "unsupported ISA class");
    return ISAClassesNum;
  }

  /// \brief Encode the ISA class for the mangled variant name.
  static char encodeISAClass(ISAClass IsaClass) {

    switch (IsaClass) {
    case XMM:
      return 'b';
    case YMM1:
      return 'c';
    case YMM2:
      return 'd';
    case ZMM:
      return 'e';
    default:
      break;
    }

    assert(false && "unsupported ISA class");
    return '?';
  }

  /// \brief Decode the ISA class from the mangled variant name.
  static ISAClass decodeISAClass(char IsaClass) {

    switch (IsaClass) {
    case 'b':
      return XMM;
    case 'c':
      return YMM1;
    case 'd':
      return YMM2;
    case 'e':
      return ZMM;
    default:
      llvm_unreachable("unsupported ISA class");
      return XMM;
    }
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
  static unsigned int calcVlen(ISAClass I, Type *Ty);
};

} // llvm namespace

#endif // LLVM_ANALYSIS_VECTORVARIANT_H
