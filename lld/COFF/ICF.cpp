//===- ICF.cpp ------------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// ICF is short for Identical Code Folding. That is a size optimization to
// identify and merge two or more read-only sections (typically functions)
// that happened to have the same contents. It usually reduces output size
// by a few percent.
//
// On Windows, ICF is enabled by default.
//
// See ELF/ICF.cpp for the details about the algortihm.
//
//===----------------------------------------------------------------------===//

#include "Chunks.h"
#include "Error.h"
#include "Symbols.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Parallel.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <atomic>
#include <vector>

using namespace llvm;

namespace lld {
namespace coff {

class ICF {
public:
  void run(const std::vector<Chunk *> &V);

private:
  void segregate(size_t Begin, size_t End, bool Constant);

  bool lessConstant(const SectionChunk *A, const SectionChunk *B);
  bool lessVariable(const SectionChunk *A, const SectionChunk *B);

  uint32_t getHash(SectionChunk *C);
  bool isEligible(SectionChunk *C);

  size_t findBoundary(size_t Begin, size_t End);

  void forEachClassRange(size_t Begin, size_t End,
                         std::function<void(size_t, size_t)> Fn);

  void forEachClass(std::function<void(size_t, size_t)> Fn);

  std::vector<SectionChunk *> Chunks;
  int Cnt = 0;
  std::atomic<bool> Repeat = {false};
};

// Returns a hash value for S.
uint32_t ICF::getHash(SectionChunk *C) {
  return hash_combine(C->getPermissions(),
                      hash_value(C->SectionName),
                      C->NumRelocs,
                      C->getAlign(),
                      uint32_t(C->Header->SizeOfRawData),
                      C->Checksum);
}

// Returns true if section S is subject of ICF.
//
// Microsoft's documentation
// (https://msdn.microsoft.com/en-us/library/bxwfs976.aspx; visited April
// 2017) says that /opt:icf folds both functions and read-only data.
// Despite that, the MSVC linker folds only functions. We found
// a few instances of programs that are not safe for data merging.
// Therefore, we merge only functions just like the MSVC tool.
bool ICF::isEligible(SectionChunk *C) {
  bool Global = C->Sym && C->Sym->isExternal();
  bool Executable = C->getPermissions() & llvm::COFF::IMAGE_SCN_MEM_EXECUTE;
  bool Writable = C->getPermissions() & llvm::COFF::IMAGE_SCN_MEM_WRITE;
  return C->isCOMDAT() && C->isLive() && Global && Executable && !Writable;
}

// Split an equivalence class into smaller classes.
void ICF::segregate(size_t Begin, size_t End, bool Constant) {

  if (Begin == End)
    return;

  auto Less = [&](SectionChunk *L, SectionChunk *R) {
    if (Constant)
      return lessConstant(L, R);
    return lessVariable(L, R);
  };

  // I'm not sure if regular sort can be used here
  std::stable_sort (Chunks.begin() + Begin, Chunks.begin() + End, Less);

  auto curGroupBegin = Begin;
  for (auto curPos = Begin + 1; curPos < End; curPos++)
  {
    if (!Less (Chunks[curGroupBegin], Chunks[curPos]))
      continue;

    // We use curPos as an equivalence class ID because every group ends with a unique index.
    for (size_t I = curGroupBegin; I < curPos; ++I)
      Chunks[I]->Class[(Cnt + 1) % 2] = curPos;

    curGroupBegin = curPos;
  }

  for (size_t I = curGroupBegin; I < End; ++I)
    Chunks[I]->Class[(Cnt + 1) % 2] = End;

  // If we created a group, we need to iterate the main loop again.
  if (curGroupBegin != Begin)
    Repeat = true;
}

// basically the same as std::lexicographical_compare but with additional data
// passing to the comparison function
// Maybe it's better to implement it using range adapters
template<class Data, class InputIt1, class InputIt2, class Compare>
bool lexicographical_compare_helper (Data add_data1, InputIt1 first1, InputIt1 last1,
                             Data add_data2, InputIt2 first2, InputIt2 last2,
                             Compare comp)
{
    for ( ; (first1 != last1) && (first2 != last2); ++first1, (void) ++first2 ) {
        if (comp(add_data1, *first1, add_data2, *first2)) return true;
        if (comp(add_data2, *first2, add_data1, *first1)) return false;
    }
    return (first1 == last1) && (first2 != last2);
}

// Compare "non-moving" part of two sections, namely everything
// except relocation targets.
bool ICF::lessConstant(const SectionChunk *A, const SectionChunk *B) {
  if (A->NumRelocs != B->NumRelocs)
    return A->NumRelocs < B->NumRelocs;

  auto to_tuple = [] (const SectionChunk * chunk) {
    return std::make_tuple (chunk->getPermissions(), chunk->SectionName, chunk->getAlign(),
                            chunk->Header->SizeOfRawData, chunk->Checksum);
  };

  auto a_tuple = to_tuple (A);
  auto b_tuple = to_tuple (B);
  if (a_tuple != b_tuple)
    return a_tuple < b_tuple;

  if (A->getContents() != B->getContents ())
    return std::lexicographical_compare (A->getContents ().begin(), A->getContents ().end(), B->getContents ().begin(), B->getContents ().end());

  // Compare relocations.
  auto Less = [this] (const SectionChunk *A, const coff_relocation &R1, const SectionChunk *B, const coff_relocation &R2) {
      auto to_tuple = [] (const coff_relocation &rel) {
        return std::make_tuple (rel.Type, rel.VirtualAddress);
      };

    if (to_tuple (R1) != to_tuple (R2))
      return to_tuple (R1) < to_tuple (R2);

    SymbolBody *B1 = A->File->getSymbolBody(R1.SymbolTableIndex);
    SymbolBody *B2 = B->File->getSymbolBody(R2.SymbolTableIndex);
    if (B1 == B2)
      return false;

    if (auto *D1 = dyn_cast<DefinedRegular>(B1))
      if (auto *D2 = dyn_cast<DefinedRegular>(B2))
        return   std::make_tuple (D1->getValue(), D1->getChunk()->Class[Cnt % 2])
               < std::make_tuple (D2->getValue(), D2->getChunk()->Class[Cnt % 2]);
    return B1 < B2;
  };

  return lexicographical_compare_helper (A, A->Relocs.begin(), A->Relocs.end(), B, B->Relocs.begin(), B->Relocs.end(),  Less);
}

// Compare "moving" part of two sections, namely relocation targets.
bool ICF::lessVariable(const SectionChunk *A, const SectionChunk *B) {
  // Compare relocations.
  auto Less = [this](const SectionChunk *A, const coff_relocation &R1, const SectionChunk *B, const coff_relocation &R2) {
    SymbolBody *B1 = A->File->getSymbolBody(R1.SymbolTableIndex);
    SymbolBody *B2 = B->File->getSymbolBody(R2.SymbolTableIndex);
    if (B1 == B2)
      return false;

    if (auto *D1 = dyn_cast<DefinedRegular>(B1))
      if (auto *D2 = dyn_cast<DefinedRegular>(B2))
        return D1->getChunk()->Class[Cnt % 2] < D2->getChunk()->Class[Cnt % 2];

    return B1 < B2;
  };
  return lexicographical_compare_helper (A, A->Relocs.begin(), A->Relocs.end(), B, B->Relocs.begin(), B->Relocs.end(), Less);
}


size_t ICF::findBoundary(size_t Begin, size_t End) {
  for (size_t I = Begin + 1; I < End; ++I)
    if (Chunks[Begin]->Class[Cnt % 2] != Chunks[I]->Class[Cnt % 2])
      return I;
  return End;
}

void ICF::forEachClassRange(size_t Begin, size_t End,
                            std::function<void(size_t, size_t)> Fn) {
  if (Begin > 0)
    Begin = findBoundary(Begin - 1, End);

  while (Begin < End) {
    size_t Mid = findBoundary(Begin, Chunks.size());
    Fn(Begin, Mid);
    Begin = Mid;
  }
}

// Call Fn on each class group.
void ICF::forEachClass(std::function<void(size_t, size_t)> Fn) {
  // If the number of sections are too small to use threading,
  // call Fn sequentially.
  if (Chunks.size() < 1024) {
    forEachClassRange(0, Chunks.size(), Fn);
    ++Cnt;
    return;
  }

  // Split sections into 256 shards and call Fn in parallel.
  size_t NumShards = 256;
  size_t Step = Chunks.size() / NumShards;
  for_each_n(parallel::par, size_t(0), NumShards, [&](size_t I) {
    size_t End = (I == NumShards - 1) ? Chunks.size() : (I + 1) * Step;
    forEachClassRange(I * Step, End, Fn);
  });
  ++Cnt;
}

// Merge identical COMDAT sections.
// Two sections are considered the same if their section headers,
// contents and relocations are all the same.
void ICF::run(const std::vector<Chunk *> &Vec) {
  // Collect only mergeable sections and group by hash value.
  uint32_t NextId = 1;
  for (Chunk *C : Vec) {
    if (auto *SC = dyn_cast<SectionChunk>(C)) {
      if (isEligible(SC))
        Chunks.push_back(SC);
      else
        SC->Class[0] = NextId++;
    }
  }

  // Initially, we use hash values to partition sections.
  for (SectionChunk *SC : Chunks)
    // Set MSB to 1 to avoid collisions with non-hash classs.
    SC->Class[0] = getHash(SC) | (1 << 31);

  // From now on, sections in Chunks are ordered so that sections in
  // the same group are consecutive in the vector.
  std::stable_sort(Chunks.begin(), Chunks.end(),
                   [](SectionChunk *A, SectionChunk *B) {
                     return A->Class[0] < B->Class[0];
                   });

  // Compare static contents and assign unique IDs for each static content.
  forEachClass([&](size_t Begin, size_t End) { segregate(Begin, End, true); });

  // Split groups by comparing relocations until convergence is obtained.
  do {
    Repeat = false;
    forEachClass(
        [&](size_t Begin, size_t End) { segregate(Begin, End, false); });
  } while (Repeat);

  log("ICF needed " + Twine(Cnt) + " iterations");

  // Merge sections in the same classs.
  forEachClass([&](size_t Begin, size_t End) {
    if (End - Begin == 1)
      return;

    log("Selected " + Chunks[Begin]->getDebugName());
    for (size_t I = Begin + 1; I < End; ++I) {
      log("  Removed " + Chunks[I]->getDebugName());
      Chunks[Begin]->replace(Chunks[I]);
    }
  });
}

// Entry point to ICF.
void doICF(const std::vector<Chunk *> &Chunks) { ICF().run(Chunks); }

} // namespace coff
} // namespace lld
