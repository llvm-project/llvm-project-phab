//===-- scudo_allocator_secondary.h -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Scudo Secondary Allocator.
/// This services allocation that are too large to be serviced by the Primary
/// Allocator. It is directly backed by the memory mapping functions of the
/// operating system.
///
//===----------------------------------------------------------------------===//

#ifndef SCUDO_ALLOCATOR_SECONDARY_H_
#define SCUDO_ALLOCATOR_SECONDARY_H_

#ifndef SCUDO_ALLOCATOR_H_
# error "This file must be included inside scudo_allocator.h."
#endif

class ScudoLargeMmapAllocator {
 public:

  void Init() {
    PageSize = GetPageSizeCached();
  }

  void *Allocate(AllocatorStats *Stats, uptr Size, uptr Alignment) {
    ReservedAddressRange address_range;
    uptr UserSize = Size - AlignedChunkHeaderSize;
    // The Scudo frontend prevents us from allocating more than
    // MaxAllowedMallocSize, so integer overflow checks would be superfluous.
    uptr MapSize = Size + ReservedAddressRangeSize;
    if (Alignment > MinAlignment)
      MapSize += Alignment;
    MapSize = RoundUpTo(MapSize, PageSize);
    // Account for 2 guard pages, one before and one after the chunk.
    MapSize += 2 * PageSize;

    uptr MapBeg = address_range.Init(MapSize);
    CHECK_EQ(MapBeg, address_range.base());
    CHECK_EQ(MapSize, address_range.size());
    if (MapBeg == ~static_cast<uptr>(0))
      return ReturnNullOrDieOnFailure::OnOOM();
    // A page-aligned pointer is assumed after that, so check it now.
    CHECK(IsAligned(MapBeg, PageSize));
    uptr MapEnd = MapBeg + MapSize;
    // The beginning of the user area for that allocation comes after the
    // initial guard page, and both headers. This is the pointer that has to
    // abide by alignment requirements.
    uptr UserBeg = MapBeg + PageSize + HeadersSize;
    uptr UserEnd = UserBeg + UserSize;

    // In the rare event of larger alignments, we will attempt to fit the mmap
    // area better and unmap extraneous memory. This will also ensure that the
    // offset and unused bytes field of the header stay small.
    if (Alignment > MinAlignment) {
      if (!IsAligned(UserBeg, Alignment)) {
        UserBeg = RoundUpTo(UserBeg, Alignment);
        CHECK_GE(UserBeg, MapBeg);
        uptr NewMapBeg = RoundDownTo(UserBeg - HeadersSize, PageSize) -
            PageSize;
        CHECK_GE(NewMapBeg, MapBeg);
        if (NewMapBeg != MapBeg) {
          address_range.Unmap(reinterpret_cast<void *>(MapBeg), NewMapBeg - MapBeg);
          MapBeg = NewMapBeg;
          CHECK_EQ(MapBeg, address_range.base());
        }
        UserEnd = UserBeg + UserSize;
      }
      uptr NewMapEnd = RoundUpTo(UserEnd, PageSize) + PageSize;
      if (NewMapEnd != MapEnd) {
        address_range.Unmap(reinterpret_cast<void *>(NewMapEnd), MapEnd - NewMapEnd);
        MapEnd = NewMapEnd;
      }
      MapSize = MapEnd - MapBeg;
      CHECK_EQ(MapSize, address_range.size());
    }

    CHECK_LE(UserEnd, MapEnd - PageSize);
    // Actually mmap the memory, preserving the guard pages on either side.
    CHECK_EQ(MapBeg + PageSize, address_range.Map(MapBeg + PageSize, MapSize - 2 * PageSize));
    uptr Ptr = UserBeg - AlignedChunkHeaderSize;
    ReservedAddressRange *stored_range = new(reinterpret_cast<void*>(Ptr)) ReservedAddressRange();
    // TODO should probably be a move
    Swap(address_range, *stored_range);
    CHECK_EQ(MapBeg, stored_range->base());
    CHECK_EQ(MapSize, stored_range->size());

    // The primary adds the whole class size to the stats when allocating a
    // chunk, so we will do something similar here. But we will not account for
    // the guard pages.
    {
      SpinMutexLock l(&StatsMutex);
      Stats->Add(AllocatorStatAllocated, MapSize - 2 * PageSize);
      Stats->Add(AllocatorStatMapped, MapSize - 2 * PageSize);
    }

    return reinterpret_cast<void *>(Ptr);
  }

  void Deallocate(AllocatorStats *Stats, void *Ptr) {
    ReservedAddressRange *stored_range = getHeader(Ptr);
    {
      SpinMutexLock l(&StatsMutex);
      Stats->Sub(AllocatorStatAllocated, stored_range->size() - 2 * PageSize);
      Stats->Sub(AllocatorStatMapped, stored_range->size() - 2 * PageSize);
    }
    UnmapOrDie(reinterpret_cast<void *>(stored_range->base()), stored_range->size());
  }

  uptr GetActuallyAllocatedSize(void *Ptr) {
    ReservedAddressRange *stored_range = getHeader(Ptr);
    // Deduct PageSize as MapSize includes the trailing guard page.
    uptr MapEnd = reinterpret_cast<uptr>(stored_range->base()) + stored_range->size() - PageSize;
    return MapEnd - reinterpret_cast<uptr>(Ptr);
  }

 private:
  // Check that sizeof(ReservedAddressRange) is a multiple of MinAlignment.
  COMPILER_CHECK((sizeof(ReservedAddressRange) & (MinAlignment - 1)) == 0);

  ReservedAddressRange *getHeader(uptr Ptr) {
    return reinterpret_cast<ReservedAddressRange*>(Ptr - sizeof(ReservedAddressRange));
  }
  ReservedAddressRange *getHeader(const void *Ptr) {
    return getHeader(reinterpret_cast<uptr>(Ptr));
  }

  const uptr ReservedAddressRangeSize = sizeof(ReservedAddressRange);
  const uptr HeadersSize = ReservedAddressRangeSize + AlignedChunkHeaderSize;
  uptr PageSize;
  SpinMutex StatsMutex;
};

#endif  // SCUDO_ALLOCATOR_SECONDARY_H_
