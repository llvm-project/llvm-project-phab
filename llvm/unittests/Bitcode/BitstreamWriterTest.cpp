//===- BitstreamWriterTest.cpp - Tests for BitstreamWriter ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Bitcode/BitstreamWriter.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(BitstreamWriterTest, emitBlob) {
  SmallString<64> Buffer;
  BitstreamWriter W(Buffer);
  W.emitBlob("str", /* ShouldEmitSize */ false);
  EXPECT_EQ(StringRef("str\0", 4), Buffer);
}

TEST(BitstreamWriterTest, emitBlobWithSize) {
  SmallString<64> Buffer;
  {
    BitstreamWriter W(Buffer);
    W.emitBlob("str");
  }
  SmallString<64> Expected;
  {
    BitstreamWriter W(Expected);
    W.EmitVBR(3, 6);
    W.FlushToWord();
    W.Emit('s', 8);
    W.Emit('t', 8);
    W.Emit('r', 8);
    W.Emit(0, 8);
  }
  EXPECT_EQ(StringRef(Expected), Buffer);
}

TEST(BitstreamWriterTest, emitBlobEmpty) {
  SmallString<64> Buffer;
  BitstreamWriter W(Buffer);
  W.emitBlob("", /* ShouldEmitSize */ false);
  EXPECT_EQ(StringRef(""), Buffer);
}

TEST(BitstreamWriterTest, emitBlob4ByteAligned) {
  SmallString<64> Buffer;
  BitstreamWriter W(Buffer);
  W.emitBlob("str0", /* ShouldEmitSize */ false);
  EXPECT_EQ(StringRef("str0"), Buffer);
}

#ifndef NDEBUG
TEST(BitstreamWriterTest, trapOnSmallAbbrev) {
  SmallString<64> Buffer;
  BitstreamWriter W(Buffer);
  W.EnterSubblock(bitc::FIRST_APPLICATION_BLOCKID, /*CodeLen*/2);
  auto Abbrev = std::make_shared<BitCodeAbbrev>();
  Abbrev->Add(BitCodeAbbrevOp(42U));
  unsigned AbbrevCode = W.EmitAbbrev(std::move(Abbrev));
  EXPECT_DEATH({ W.EmitRecordWithAbbrev(AbbrevCode, makeArrayRef(42U)); },
               "Block code length is too small");
  W.ExitBlock();
}

TEST(BitstreamWriterTest, trapOnSmallAbbrevUsingBlockInfo) {
  SmallString<64> Buffer;
  BitstreamWriter W(Buffer);
  W.EnterBlockInfoBlock();
  auto Abbrev = std::make_shared<BitCodeAbbrev>();
  Abbrev->Add(BitCodeAbbrevOp(42U));
  unsigned AbbrevCode = W.EmitBlockInfoAbbrev(bitc::FIRST_APPLICATION_BLOCKID,
                                              std::move(Abbrev));
  W.ExitBlock();
  W.EnterSubblock(bitc::FIRST_APPLICATION_BLOCKID, /*CodeLen*/2);
  EXPECT_DEATH({ W.EmitRecordWithAbbrev(AbbrevCode, makeArrayRef(42U)); },
               "Block code length is too small");
  W.ExitBlock();
}
#endif

} // end namespace
