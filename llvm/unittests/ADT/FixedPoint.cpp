//===- llvm/unittest/ADT/FixedPoint.cpp - FixedPoint unit tests ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/FixedPoint.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(FixedPoint64, Add) {
  FixedPoint64 One(1U);
  FixedPoint64 Two(2U);
  EXPECT_TRUE((One + One) == Two);
}

TEST(FixedPoint64, Sub) {
  FixedPoint64 One(1U);
  FixedPoint64 Two(2U);
  EXPECT_TRUE((Two - One) == One);
}

TEST(FixedPoint64, Mul) {
  FixedPoint64 Two(2U);
  FixedPoint64 Four(4U);
  EXPECT_TRUE((Two * Two) == Four);
}

TEST(FixedPoint64, Div) {
  FixedPoint64 One(1U);
  FixedPoint64 Two(2U);
  FixedPoint64 Four(4U);
  EXPECT_TRUE((Two / Four) == (One / Two));
}

TEST(FixedPoint64, Cmp) {
  FixedPoint64 One(1U);
  FixedPoint64 Two(2U);
  FixedPoint64 Half = One / Two;
  EXPECT_TRUE(One != Half);
  EXPECT_TRUE(Two == Two);
  EXPECT_TRUE(Half < One);
  EXPECT_TRUE(One > Half);
  EXPECT_TRUE(Half <= Two);
  EXPECT_TRUE(Two > One);
}

} // end anonymous namespace
