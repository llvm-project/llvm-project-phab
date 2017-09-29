//===- llvm/unittest/ADT/APFloat.cpp - APFloat unit tests
//---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <initializer_list>
#include <string>

#include "llvm/ADT/ChunkedList.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

using namespace llvm;

TEST(ChunkedListTest, withinFirstChunk) {
  ChunkedList<std::string, 3> CL;
  for (std::string S : {"1", "2", "3"})
    CL.push_back(S);

  int Counter = 3;
  for (std::string C : CL) {
    EXPECT_EQ(std::to_string(Counter), C);
    Counter--;
  }
}

TEST(ChunkedListTest, inThreeChunks) {
  ChunkedList<std::string, 3> CL;
  for (std::string S : {"1", "2", "3", "4", "5", "6", "7"})
    CL.push_back(S);

  int Counter = 7;
  for (std::string C : CL) {
    EXPECT_EQ(std::to_string(Counter), C);
    Counter--;
  }
}

TEST(ChunkedListTest, empty) {
  ChunkedList<std::string, 3> CL;

  for (std::string C : CL) {
    (void)C;
    ADD_FAILURE() << "List should have been empty!";
  }

  EXPECT_TRUE(CL.empty());
}

TEST(ChunkedListTest, moveSemantics) {
  ChunkedList<std::string, 3> SourceCL;

  for (std::string i : {"1", "2", "3", "4"})
    SourceCL.push_back(i);

  ChunkedList<std::string, 3> DestCL(std::move(SourceCL));

  int Counter = 4;
  for (std::string C : DestCL) {
    EXPECT_EQ(std::to_string(Counter), C);
    Counter--;
  }

  for (std::string C : SourceCL) {
    (void)C;
    ADD_FAILURE() << "List should have been empty!";
  }

  EXPECT_TRUE(SourceCL.empty());
}

static void checkValues(const ChunkedList<std::string, 3> &SourceCL,
                        const ChunkedList<std::string, 3> &DestCL,
                        const SmallVector<std::string, 3> &Values) {
  int Idx = Values.size() - 1;
  for (std::string V : DestCL) {
    EXPECT_EQ(V, Values[Idx]);
    Idx--;
  }

  Idx = Values.size() - 1;
  for (std::string V : SourceCL) {
    EXPECT_EQ(V, Values[Idx]);
    Idx--;
  }

  EXPECT_EQ(Values.empty(), SourceCL.empty());
  EXPECT_EQ(Values.empty(), DestCL.empty());
}

static void copySemanticsTest(const SmallVector<std::string, 3> &Values) {
  ChunkedList<std::string, 3> SourceCL;

  for (std::string i : Values)
    SourceCL.push_back(i);

  {
    ChunkedList<std::string, 3> DestCLViaCopyConstructor(SourceCL);
    checkValues(SourceCL, DestCLViaCopyConstructor, Values);
  }
  {
    ChunkedList<std::string, 3> DestCLViaOperatorEquals;
    DestCLViaOperatorEquals = SourceCL;
    checkValues(SourceCL, DestCLViaOperatorEquals, Values);
  }
}

TEST(ChunkedListTest, copySemanticsTwoChunks) {
  copySemanticsTest({"1", "2", "3", "4"});
}

TEST(ChunkedListTest, copySemanticsOneChunkA) {
  copySemanticsTest({"1", "2", "3"});
}

TEST(ChunkedListTest, copySemanticsOneChunkB) {
  copySemanticsTest({"1", "2", "3"});
}

TEST(ChunkedListTest, copySemanticsEmpty) { copySemanticsTest({}); }

struct StructWithDestructor {
  int *Counter;

  ~StructWithDestructor() {
    EXPECT_NE(Counter, nullptr);
    (*Counter)++;
    Counter = nullptr;
  }
};

static void destructorTest(int Count) {
  int DestructorCounter = 0;
  {
    ChunkedList<StructWithDestructor, 3> CL;
    for (int i = 0; i < Count; i++)
      CL.push_back(StructWithDestructor({&DestructorCounter}));
    DestructorCounter = 0;
  }

  EXPECT_EQ(Count, DestructorCounter);
}

TEST(ChunkedListTest, destructorTestOneChunkA) { destructorTest(2); }

TEST(ChunkedListTest, destructorTestOneChunkB) { destructorTest(3); }

TEST(ChunkedListTest, destructorTestTwoChunks) { destructorTest(4); }

TEST(ChunkedListTest, destructorTestEmpty) { destructorTest(0); }
