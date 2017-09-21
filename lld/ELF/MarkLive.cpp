//===- MarkLive.cpp -------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements --gc-sections, which is a feature to remove unused
// sections from output. Unused sections are sections that are not reachable
// from known GC-root symbols or sections. Naturally the feature is
// implemented as a mark-sweep garbage collector.
//
// Here's how it works. Each InputSectionBase has a "Live" bit. The bit is off
// by default. Starting with GC-root symbols or sections, markLive function
// defined in this file visits all reachable sections to set their Live
// bits. Writer will then ignore sections whose Live bits are off, so that
// such sections are not included into output.
//
//===----------------------------------------------------------------------===//

#include "EhFrame.h"
#include "InputSection.h"
#include "LinkerScript.h"
#include "Memory.h"
#include "OutputSections.h"
#include "Strings.h"
#include "SymbolTable.h"
#include "Symbols.h"
#include "Target.h"
#include "Writer.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Object/ELF.h"
#include <functional>
#include <vector>

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace llvm::support::endian;

using namespace lld;
using namespace lld::elf;

namespace {
class EhFrameSectionCies {
  std::vector<EhSectionPiece *> Cies;

public:
  void add(EhSectionPiece *Cie);
  EhSectionPiece *get(const EhSectionPiece &Fde, uint32_t ID) const;
};
}

template <class ELFT>
static typename ELFT::uint getAddend(InputSectionBase &Sec,
                                     const typename ELFT::Rel &Rel) {
  return Target->getImplicitAddend(Sec.Data.begin() + Rel.r_offset,
                                   Rel.getType(Config->IsMips64EL));
}

template <class ELFT>
static typename ELFT::uint getAddend(InputSectionBase &Sec,
                                     const typename ELFT::Rela &Rel) {
  return Rel.r_addend;
}

// There are normally few input sections whose names are valid C
// identifiers, so we just store a std::vector instead of a multimap.
static DenseMap<StringRef, std::vector<InputSectionBase *>> CNamedSections;

template <class ELFT, class RelT>
static void resolveReloc(InputSectionBase &Sec, RelT &Rel,
                         std::function<void(InputSectionBase *, uint64_t)> Fn) {
  SymbolBody &B = Sec.getFile<ELFT>()->getRelocTargetSym(Rel);

  if (auto *Sym = dyn_cast<DefinedCommon>(&B)) {
    Sym->Live = true;
    return;
  }

  if (auto *D = dyn_cast<DefinedRegular>(&B)) {
    if (!D->Section)
      return;
    uint64_t Offset = D->Value;
    if (D->isSection())
      Offset += getAddend<ELFT>(Sec, Rel);
    Fn(cast<InputSectionBase>(D->Section), Offset);
    return;
  }

  if (auto *U = dyn_cast<Undefined>(&B))
    for (InputSectionBase *Sec : CNamedSections.lookup(U->getName()))
      Fn(Sec, 0);
}

// Calls Fn for each section that Sec refers to via relocations.
template <class ELFT>
static void
forEachSuccessor(InputSection &Sec,
                 std::function<void(InputSectionBase *, uint64_t)> Fn) {
  if (Sec.AreRelocsRela) {
    for (const typename ELFT::Rela &Rel : Sec.template relas<ELFT>())
      resolveReloc<ELFT>(Sec, Rel, Fn);
  } else {
    for (const typename ELFT::Rel &Rel : Sec.template rels<ELFT>())
      resolveReloc<ELFT>(Sec, Rel, Fn);
  }

  for (InputSectionBase *IS : Sec.DependentSections)
    Fn(IS, 0);
}

void EhFrameSectionCies::add(EhSectionPiece *Cie) {
  assert(Cies.empty() || Cies.back()->InputOff < Cie->InputOff);
  Cies.push_back(Cie);
}

EhSectionPiece *EhFrameSectionCies::get(const EhSectionPiece &Fde,
                                        uint32_t ID) const {
  if (Cies.empty())
    return nullptr;

  const uint32_t CieOffset = Fde.InputOff + 4 - ID;

  // In most cases, a FDE refers to the most recent CIE.
  if (Cies.back()->InputOff == CieOffset)
    return Cies.back();

  auto Found = std::lower_bound(Cies.begin(), Cies.end(), CieOffset,
                                [](EhSectionPiece *Cie, uint32_t Offset) {
                                  return Cie->InputOff < Offset;
                                });
  if (Found != Cies.end() && (*Found)->InputOff == CieOffset)
    return *Found;

  return nullptr;
}

template <class ELFT, class RelTy>
static void scanFde(EhInputSection &EH, ArrayRef<RelTy> Rels,
                    std::function<void(InputSectionBase *, uint64_t)> Fn,
                    const EhSectionPiece &Fde) {
  const typename ELFT::uint FdeEnd = Fde.InputOff + Fde.Size;
  // The first relocation is known to point to the described function,
  // so it is safe to skip it when looking for LSDAs.
  for (unsigned I = Fde.FirstRelocation + 1, N = Rels.size(); I < N; ++I) {
    const RelTy &Rel = Rels[I];
    if (Rel.r_offset >= FdeEnd)
      break;
    // In a FDE, the relocations point to the described function or to a LSDA.
    // The described function is already marked and calling Fn for it is safe.
    // Although the main intent here is to mark LSDAs, we do not need to
    // recognize them especially and can process all references the same way.
    resolveReloc<ELFT>(EH, Rel, Fn);
  }
}

template <class ELFT, class RelTy>
static void scanCie(EhInputSection &EH, ArrayRef<RelTy> Rels,
                    std::function<void(InputSectionBase *, uint64_t)> Fn,
                    const EhSectionPiece &Cie) {
  const unsigned CieFirstRelI = Cie.FirstRelocation;
  if (CieFirstRelI == (unsigned)-1)
    return;

  // In a CIE, we only need to worry about the first relocation.
  // It is known to point to the personality function.
  resolveReloc<ELFT>(EH, Rels[CieFirstRelI], Fn);
}

// The .eh_frame section is an unfortunate special case.
// The section is divided in CIEs (Common Information Entries) and FDEs
// (Frame Description Entries). Each FDE references to a corresponding
// CIE via ID field, where a relative offset is stored.
// The relocations the section can have are:
// * CIEs can refer to a personality function.
// * FDEs can refer to a LSDA.
// * FDEs refer to the function they contain information about.
// The last kind of relocation is used to keep a FDE alive if the section
// it refers to is alive. In that case, everything which is referenced from
// that FDE should be marked, including a LSDA, a CIE and a personality
// function. These sections might reference to other sections, so they
// are added to the GC queue.
template <class ELFT, class RelTy>
static void
scanEhFrameSection(EhInputSection &EH, ArrayRef<RelTy> Rels,
                   std::function<void(InputSectionBase *, uint64_t)> Fn) {
  const endianness E = ELFT::TargetEndianness;

  EhFrameSectionCies Cies;
  for (EhSectionPiece &Piece : EH.Pieces) {
    // The empty record is the end marker.
    if (Piece.Size == 4)
      return;

    const uint32_t ID = read32<E>(Piece.data().data() + 4);
    if (ID == 0) {
      Cies.add(&Piece);
      continue;
    }

    if (Piece.Live // This FDE is already fully processed.
        || !elf::isFdeLive<ELFT>(Piece, Rels))
      continue;
    // This FDE should be kept.
    Piece.Live = true;

    scanFde<ELFT>(EH, Rels, Fn, Piece);

    // Find the corresponding CIE
    EhSectionPiece *Cie = Cies.get(Piece, ID);
    if (!Cie)
      fatal(toString(&EH) + ": invalid CIE reference");

    if (Cie->Live) // Already processed.
      continue;
    Cie->Live = true;

    scanCie<ELFT>(EH, Rels, Fn, *Cie);
  }
}

template <class ELFT>
static void
scanEhFrameSection(EhInputSection &EH,
                   std::function<void(InputSectionBase *, uint64_t)> Fn) {
  if (!EH.NumRelocations)
    return;

  // Unfortunately we need to split .eh_frame early since some relocations in
  // .eh_frame keep other section alive and some don't.
  EH.split<ELFT>();

  if (EH.AreRelocsRela)
    scanEhFrameSection<ELFT>(EH, EH.template relas<ELFT>(), Fn);
  else
    scanEhFrameSection<ELFT>(EH, EH.template rels<ELFT>(), Fn);
}

// We do not garbage-collect two types of sections:
// 1) Sections used by the loader (.init, .fini, .ctors, .dtors or .jcr)
// 2) Non-allocatable sections which typically contain debugging information
template <class ELFT> static bool isReserved(InputSectionBase *Sec) {
  switch (Sec->Type) {
  case SHT_FINI_ARRAY:
  case SHT_INIT_ARRAY:
  case SHT_NOTE:
  case SHT_PREINIT_ARRAY:
    return true;
  default:
    if (!(Sec->Flags & SHF_ALLOC))
      return true;

    StringRef S = Sec->Name;
    return S.startswith(".ctors") || S.startswith(".dtors") ||
           S.startswith(".init") || S.startswith(".fini") ||
           S.startswith(".jcr");
  }
}

// This is the main function of the garbage collector.
// Starting from GC-root sections, this function visits all reachable
// sections to set their "Live" bits.
template <class ELFT> void elf::markLive() {
  SmallVector<InputSection *, 256> Q;
  CNamedSections.clear();
  // The flag is set if an executable section is added to the queue.
  // We scan .eh_frame sections only if the flag is set.
  bool ExecInQueue = false;

  auto Enqueue = [&](InputSectionBase *Sec, uint64_t Offset) {
    // Skip over discarded sections. This in theory shouldn't happen, because
    // the ELF spec doesn't allow a relocation to point to a deduplicated
    // COMDAT section directly. Unfortunately this happens in practice (e.g.
    // .eh_frame) so we need to add a check.
    if (Sec == &InputSection::Discarded)
      return;

    // We don't gc non alloc sections.
    if (!(Sec->Flags & SHF_ALLOC))
      return;

    // Usually, a whole section is marked as live or dead, but in mergeable
    // (splittable) sections, each piece of data has independent liveness bit.
    // So we explicitly tell it which offset is in use.
    if (auto *MS = dyn_cast<MergeInputSection>(Sec))
      MS->markLiveAt(Offset);

    if (Sec->Live)
      return;
    Sec->Live = true;

    // Add input section to the queue.
    if (InputSection *S = dyn_cast<InputSection>(Sec)) {
      Q.push_back(S);
      if (S->Flags & SHF_EXECINSTR)
        ExecInQueue = true;
    }
  };

  auto MarkSymbol = [&](SymbolBody *Sym) {
    if (auto *D = dyn_cast_or_null<DefinedRegular>(Sym)) {
      if (auto *IS = cast_or_null<InputSectionBase>(D->Section))
        Enqueue(IS, D->Value);
      return;
    }
    if (auto *S = dyn_cast_or_null<DefinedCommon>(Sym))
      S->Live = true;
  };

  // Add GC root symbols.
  MarkSymbol(Symtab->find(Config->Entry));
  MarkSymbol(Symtab->find(Config->Init));
  MarkSymbol(Symtab->find(Config->Fini));
  for (StringRef S : Config->Undefined)
    MarkSymbol(Symtab->find(S));
  for (StringRef S : Script->Opt.ReferencedSymbols)
    MarkSymbol(Symtab->find(S));

  // Preserve externally-visible symbols if the symbols defined by this
  // file can interrupt other ELF file's symbols at runtime.
  for (Symbol *S : Symtab->getSymbols())
    if (S->includeInDynsym())
      MarkSymbol(S->body());

  std::vector<EhInputSection *> EhInputSections;

  // Preserve special sections and those which are specified in linker
  // script KEEP command.
  for (InputSectionBase *Sec : InputSections) {
    // Postpone scanning .eh_frame until we find all the secions reachable
    // from the roots.
    if (auto *EH = dyn_cast_or_null<EhInputSection>(Sec))
      EhInputSections.push_back(EH);
    if (Sec->Flags & SHF_LINK_ORDER)
      continue;
    if (isReserved<ELFT>(Sec) || Script->shouldKeep(Sec))
      Enqueue(Sec, 0);
    else if (isValidCIdentifier(Sec->Name)) {
      CNamedSections[Saver.save("__start_" + Sec->Name)].push_back(Sec);
      CNamedSections[Saver.save("__stop_" + Sec->Name)].push_back(Sec);
    }
  }

  // Mark all reachable sections.
  // It should take only one or two iterations to converge.
  // If the output does not use exception handling, scanning .eh_frame sections
  // will not add new sections to the queue and the loop will stop.
  // Otherwise, sections for LSDAs and a personality function will be added.
  // The personality function might be located in a DSO or in a library.
  // In the latter case, it might require an additional iteration because
  // it may introduce new dependencies, which, in turn, will require
  // an additional scan of .eh_frame sections.
  while (true) {
    while (!Q.empty())
      forEachSuccessor<ELFT>(*Q.pop_back_val(), Enqueue);
    if (!ExecInQueue)
      break;
    ExecInQueue = false;
    for (EhInputSection *EH : EhInputSections)
      scanEhFrameSection<ELFT>(*EH, Enqueue);
  }
}

template void elf::markLive<ELF32LE>();
template void elf::markLive<ELF32BE>();
template void elf::markLive<ELF64LE>();
template void elf::markLive<ELF64BE>();
