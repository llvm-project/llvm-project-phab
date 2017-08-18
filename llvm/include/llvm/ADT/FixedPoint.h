//===- llvm/ADT/FixedPoint.h - Fixed point arithmetic ---*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
/// This file declares a class to represent fixed point arithmetic.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_FIXEDPOINT_H
#define LLVM_ADT_FIXEDPOINT_H

#include <assert.h>
#include <cstdint>

/// This is very simple and fast implementation of unsigned FixedPoint
/// arithmetic.
/// The class include operators: +,-,/,*,<,>,>=,<=,!=,==.
/// The is no overflow check. It is user resposibility to check overflows.
/// Adding 1 to UINT32_MAX will result in 0.
class FixedPoint64 {
private:
  uint64_t Value;
  FixedPoint64(uint64_t N) : Value(N) {}
  FixedPoint64(uint32_t Hi, uint32_t Lo) {
    Value = (((uint64_t)Hi) << 32) | Lo;
  }
  uint32_t getLo() const {
    return (uint32_t)Value;
  }
  uint32_t getHi() const {
    return (uint32_t)(Value >> 32);
  }
public:
  FixedPoint64() : Value(0) {}
  FixedPoint64(uint32_t N) {
    Value = FixedPoint64(N, 0).Value;
  }
  float getFloat() const {
    return (float)Value / ((uint64_t)1 << 32);
  }
  FixedPoint64 operator+(const FixedPoint64 Add) const {
    return FixedPoint64(Value + Add.Value);
  }
  FixedPoint64 operator-(const FixedPoint64 Sub) const {
    return FixedPoint64(Value - Sub.Value);
  }
  FixedPoint64 operator/(const FixedPoint64 &Divider) const {
    assert(Divider.Value && "Division by zero!");
    uint32_t Hi = (uint32_t)(Value / Divider.Value);
    uint32_t Lo = (uint32_t)((Value << 32) / Divider.Value);
    if (Divider.Value >> 32)
      Lo += (uint32_t)((Value) / (Divider.Value >> 32));
    return FixedPoint64(Hi, Lo);
  }
  FixedPoint64 operator*(const FixedPoint64 Multiplier) const {
    uint64_t Sum = (uint64_t)this->getHi() * Multiplier.getLo();
    Sum += (uint64_t)this->getLo() * Multiplier.getHi();
    Sum += ((uint64_t)this->getLo() * Multiplier.getLo()) >> 32;
    uint32_t Hi = (uint32_t)(this->getHi() * Multiplier.getHi() + (Sum >> 32));
    return FixedPoint64(Hi, (uint32_t)Sum);
  }
  bool operator<(const FixedPoint64 Right) const {
    return Value < Right.Value;
  }
  bool operator>(const FixedPoint64 Right) const {
    return Value > Right.Value;
  }
  bool operator<=(const FixedPoint64 Right) const {
    return Value <= Right.Value;
  }
  bool operator>=(const FixedPoint64 Right) const {
    return Value >= Right.Value;
  }
  bool operator!=(const FixedPoint64 Right) const {
    return Value != Right.Value;
  }
  bool operator==(const FixedPoint64 Right) const {
    return Value == Right.Value;
  }
};
#endif // LLVM_ADT_FIXEDPOINT_H
