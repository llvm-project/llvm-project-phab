//===- llvm/ADT/SmallVector.h - 'Normally small' vectors --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// A chunked list is a unidirectional linked list where each node in the list
// contains an array of \c N values.
//
// Pros:
//
// - Constant (and low, depending on \c N) memory overhead: the amount of memory
//   consumed is a constant amount over the amount of memory needed.
// - Fast insertion: O(1) in worst case
//
// Cons:
//
// - O(n) random access.
// - Only forward iteration supported in LIFO order.
// - O(n) deletion.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_CHUNKED_LIST_H
#define LLVM_ADT_CHUNKED_LIST_H

#include "llvm/ADT/iterator.h"
#include "llvm/Support/Compiler.h"

#include <cassert>
#include <cstddef>
#include <iterator>

namespace llvm {

template <typename T, size_t N> class ChunkedList {
  struct Chunk;

public:
  using value_type = T;

  ChunkedList() = default;
  ChunkedList(const ChunkedList &Other) { copyFrom(Other); }
  ChunkedList(ChunkedList &&Other) : End(Other.End), Capacity(Other.Capacity) {
    Other.End = nullptr;
    Other.Capacity = nullptr;
  }

  ~ChunkedList() { clear(); }

  ChunkedList &operator=(const ChunkedList &Other) {
    clear();
    copyFrom(Other);
    return *this;
  }

  ChunkedList &operator=(ChunkedList &&Other) {
    clear();
    End = Other.End;
    Other.End = nullptr;
    Capacity = Other.Capacity;
    Other.Capacity = nullptr;
  }

  void push_back(const T &V) { new (getLastLocation()) T(V); }
  void push_back(T &&V) { new (getLastLocation()) T(std::move(V)); }

  template <bool IsConst>
  class Iterator : public iterator_facade_base<Iterator<IsConst>,
                                               std::forward_iterator_tag, T> {
    // Implementation Note:
    //
    // Iterators traverse the ChunkedList in LIFO order.  They maintain a
    // pointer to the current element, \c Current; and to the first element in
    // the chunk the current element belongs to, \c First.  Once we've hit \c
    // First, we use the known offset of \c First within \c Chunk to get to the
    // containing \c Chunk instance, and retreive the previous chunk.
    using ConstIterator = Iterator<true>;
    using MutableIterator = Iterator<false>;

    friend ConstIterator;
    friend MutableIterator;

  public:
    using value_type = typename std::conditional<IsConst, const T, T>::type;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;

    template <bool IsConstSrc,
              typename = typename std::enable_if<!IsConstSrc || IsConst>::type>
    Iterator(const Iterator<IsConstSrc> &Other)
        : Current(Other.Current), First(Other.First) {}

    reference operator*() const { return *Current; }

    bool operator==(const ConstIterator &RHS) const {
      assert(Current != RHS.First || First == RHS.First && "Broken invariant");
      return Current == RHS.Current;
    }

    Iterator &operator++() { // Preincrement
      if (LLVM_UNLIKELY(Current == First)) {
        Chunk *PrevChunk = getPrevChunk();
        if (!PrevChunk) {
          Current = First = nullptr;
          return *this;
        }
        Current = &PrevChunk->Values[N];
        First = &PrevChunk->Values[0];
      }
      --Current;
      return *this;
    }

  private:
    T *Current;
    T *First;

    Iterator(T *Current, T *First) : Current(Current), First(First) {}

    Chunk *getPrevChunk() const {
      assert(First && "getPrevChunk() on empty list!");
      char *CurChunkAddress =
          reinterpret_cast<char *>(First) - offsetof(Chunk, Values);
      Chunk *CurChunk = reinterpret_cast<Chunk *>(CurChunkAddress);
      return CurChunk->Prev;
    }

    friend class ChunkedList<T, N>;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  iterator begin() {
    if (End == nullptr)
      return iterator(nullptr, nullptr);

    // We never have this condition.  End either points to one past the last
    // element in the full chunk or one past the first element in a chunk with
    // only one element.
    assert(End != &getTailChunk()->Values[0] && "Broken invariant!");
    return iterator(End - 1, &getTailChunk()->Values[0]);
  }
  iterator end() { return iterator(nullptr, nullptr); }

  const_iterator begin() const {
    return const_cast<ChunkedList<T, N> *>(this)->begin();
  }
  const_iterator end() const {
    return const_cast<ChunkedList<T, N> *>(this)->end();
  }

  bool empty() const { return End == nullptr; }

private:
  // Implementation Note:
  //
  // \c End points to one past the last inserted element and \c Capacity points
  // to the last element in the current chunk.  When \c End meets \c Capacity
  // and we need to create a new Chunk, we use \c Capacity to find the pointer
  // to the current \c Chunk instance (to store as \c Prev in the new chunk).
  struct Chunk {
    Chunk *Prev;
    T Values[N];
  };

  T *End = nullptr;
  T *Capacity = nullptr;

  Chunk *getTailChunk() const {
    if (Capacity)
      return ChunkedList<T, N>::getTailChunkFromCapacityPtr(Capacity);
    return nullptr;
  }

  Chunk *mallocChunk() const {
    return static_cast<Chunk *>(malloc(sizeof(Chunk)));
  }

  T *getLastLocation() {
    if (LLVM_UNLIKELY(End == Capacity)) {
      auto *NewChunk = mallocChunk();
      NewChunk->Prev = getTailChunk();
      End = &NewChunk->Values[0];
      Capacity = &NewChunk->Values[N];
    }
    return End++;
  }

  void clear() {
    if (End == nullptr) {
      assert(Capacity == nullptr && "Broken invariant!");
      return;
    }

    Chunk *TailChunk = getTailChunk();
    Chunk *CurrentChunk = TailChunk->Prev;

    for (T *I = &TailChunk->Values[0], *E = End; I != E; ++I)
      I->~T();
    free(TailChunk);

    while (CurrentChunk) {
      Chunk *PrevChunk = CurrentChunk->Prev;
      for (size_t i = 0; i < N; i++)
        CurrentChunk->Values[i].~T();
      free(CurrentChunk);
      CurrentChunk = PrevChunk;
    }
    End = Capacity = nullptr;
  }

  void copyFrom(const ChunkedList &Other) {
    if (Other.End == nullptr) {
      assert(Other.Capacity == nullptr && "Broken invariant!");
      return;
    }

    assert(End == nullptr && Capacity == nullptr &&
           "Call clear() before calling copyFrom!");

    Chunk *OtherChunk = Other.getTailChunk();
    Chunk *MyChunk = mallocChunk();

    size_t TailCount = Other.End - &OtherChunk->Values[0];
    for (size_t i = 0; i < TailCount; i++) {
      new (&MyChunk->Values[i]) T(OtherChunk->Values[i]);
    }

    End = &MyChunk->Values[TailCount];
    Capacity = &MyChunk->Values[N];

    while (OtherChunk->Prev) {
      OtherChunk = OtherChunk->Prev;
      MyChunk->Prev = mallocChunk();
      MyChunk = MyChunk->Prev;

      for (size_t i = 0; i < N; i++)
        new (&MyChunk->Values[i]) T(OtherChunk->Values[i]);
    }

    MyChunk->Prev = nullptr;
  }

  static Chunk *getTailChunkFromCapacityPtr(T *CapacityPtr) {
    assert(CapacityPtr);
    size_t TotalOffset = offsetof(Chunk, Values) + N * sizeof(T);
    char *TailChunkAddr = reinterpret_cast<char *>(CapacityPtr) - TotalOffset;
    return reinterpret_cast<Chunk *>(TailChunkAddr);
  }

  static_assert(N > 0, "");
};
} // namespace llvm

#endif // LLVM_ADT_CHUNKED_LIST_H
