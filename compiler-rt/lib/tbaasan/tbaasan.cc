//===-- tbaasan.cc --------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of TBAASanitizer.
//
// TBAASanitizer runtime.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_flag_parser.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_report_decorator.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "sanitizer_common/sanitizer_symbolizer.h"

#include "tbaasan/tbaasan.h"

using namespace __sanitizer;
using namespace __tbaasan;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void tbaasan_set_type_unknown(void *addr, uptr size) {
  internal_memset(shadow_for(addr), 0, size*sizeof(uptr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void tbaasan_copy_types(void *daddr, void *saddr, uptr size) {
  internal_memcpy(shadow_for(daddr), shadow_for(saddr), size*sizeof(uptr));
}

static const char *getDisplayName(const char *Name) {
  if (Name[0] == '\0')
    return "<anonymous type>";

  // Clang generates tags for C++ types that demangle as typeinfo. Remove the
  // prefix from the generated string.
  const char TIPrefix[] = "typeinfo name for ";

  const char *DName = Symbolizer::GetOrInit()->Demangle(Name);
  if (!internal_strncmp(DName, TIPrefix, sizeof(TIPrefix) - 1))
    DName += sizeof(TIPrefix) - 1;

  return DName;
}

static void printTDName(tbaasan_type_descriptor *td) {
  if (!td) {
    Printf("<unknown type>");
    return;
  }

  switch (td->Tag) {
  default:
    DCHECK(0);
    break;
  case TBAASAN_MEMBER_TD:
    printTDName(td->Member.Access);
    if (td->Member.Access != td->Member.Base) {
      Printf(" (in ");
      printTDName(td->Member.Base);
      Printf(" at offset %u)", td->Member.Offset);
    }
    break;
  case TBAASAN_STRUCT_TD:
    Printf("%s", getDisplayName((char *) (td->Struct.Members +
                                          td->Struct.MemberCount)));
    break;
  } 
}

static tbaasan_type_descriptor *getRootTD(tbaasan_type_descriptor *TD) {
  tbaasan_type_descriptor *RootTD = TD;

  do {
    RootTD = TD;

    if (TD->Tag == TBAASAN_STRUCT_TD) {
      if (TD->Struct.MemberCount > 0)
        TD = TD->Struct.Members[0].Type;
      else
        TD = nullptr;
    } else if (TD->Tag == TBAASAN_MEMBER_TD) {
      TD = TD->Member.Access;
    } else {
      DCHECK(0);
      break;
    }
  } while (TD);

  return RootTD;
}

static bool isAliasingLegalUp(tbaasan_type_descriptor *TDA,
                              tbaasan_type_descriptor *TDB) {
  // Walk up the tree starting with TDA to see if we reach TDB.
  uptr OffsetA = 0, OffsetB = 0;
  if (TDB->Tag == TBAASAN_MEMBER_TD) {
    OffsetB = TDB->Member.Offset;
    TDB = TDB->Member.Base;
  }

  if (TDA->Tag == TBAASAN_MEMBER_TD) {
    OffsetA = TDA->Member.Offset;
    TDA = TDA->Member.Base;
  }

  do {
    if (TDA == TDB)
      return OffsetA == OffsetB;

    if (TDA->Tag == TBAASAN_STRUCT_TD) {
      if (!TDA->Struct.MemberCount) {
        DCHECK(0);
        break;
      }

      uptr Idx = 0;
      for (; Idx < TDA->Struct.MemberCount-1; ++Idx) {
        if (TDA->Struct.Members[Idx].Offset >= OffsetA)
          break;
      }

      OffsetA -= TDA->Struct.Members[Idx].Offset;
      TDA = TDA->Struct.Members[Idx].Type;
    } else {
      DCHECK(0);
      break;
    }
  } while (TDA);

  return false;
}

static bool isAliasingLegal(tbaasan_type_descriptor *TDA,
                            tbaasan_type_descriptor *TDB) {
  if (TDA == TDB || !TDB || !TDA)
    return true;

  // Aliasing is legal is the two types have different root nodes.
  if (getRootTD(TDA) != getRootTD(TDB))
    return true;

  return isAliasingLegalUp(TDA, TDB) || isAliasingLegalUp(TDB, TDA);
}

namespace __tbaasan {
class Decorator: public __sanitizer::SanitizerCommonDecorator {
 public:
  Decorator() : SanitizerCommonDecorator() { }
  const char *Warning()    { return Red(); }
  const char *Name()   { return Green(); }
  const char *End()    { return Default(); }
};
}

ALWAYS_INLINE
static void reportError(void *Addr, int Size, tbaasan_type_descriptor *TD,
                        tbaasan_type_descriptor *OldTD, const char *AccessStr,
                        const char *DescStr, int Offset, uptr pc, uptr bp,
                        uptr sp) {
  Decorator d;
  Printf("%s", d.Warning());
  Report("ERROR: TBAASanitizer: type-aliasing-violation on address %p"
         " (pc %p bp %p sp %p tid %d)\n",
         Addr, (void *) pc, (void *) bp, (void *) sp, GetTid());
  Printf("%s", d.End());
  Printf("%s of size %d at %p with type ", AccessStr, Size, Addr);

  Printf("%s", d.Name());
  printTDName(TD);
  Printf("%s", d.End());

  Printf(" %s of type ", DescStr);

  Printf("%s", d.Name());
  printTDName(OldTD);
  Printf("%s", d.End());

  if (Offset != 0)
    Printf(" that starts at offset %d\n", Offset);
  else
    Printf("\n");

  if (pc) {
    BufferedStackTrace ST;
    ST.Unwind(kStackTraceMax, pc, bp, 0, 0, 0, false);
    ST.Print();
  } else {
    Printf("\n");
  }
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __tbaasan_check(void *addr, int size, tbaasan_type_descriptor *td,
                     int flags) {
  GET_CALLER_PC_BP_SP;

  bool IsRead = flags & 1;
  bool IsWrite = flags & 2;
  const char *AccessStr;
  if (IsRead && !IsWrite)
    AccessStr = "READ";
  else if (!IsRead && IsWrite)
    AccessStr = "WRITE";
  else
    AccessStr = "ATOMIC UPDATE";

  tbaasan_type_descriptor **OldTDPtr = shadow_for(addr);
  tbaasan_type_descriptor *OldTD = *OldTDPtr;
  if (((uptr) OldTD) == ~((uptr)0)) {
    int i = 0;
    do {
      --i;
      --OldTDPtr;
      OldTD = *OldTDPtr;
    } while (((uptr) OldTD) == ~((uptr)0));

    if (!isAliasingLegal(td, OldTD))
      reportError(addr, size, td, OldTD, AccessStr,
                  "accesses part of an existing object", i, pc, bp, sp);

    return;
  }

  if (!isAliasingLegal(td, OldTD)) {
    reportError(addr, size, td, OldTD, AccessStr,
                "accesses an existing object", 0, pc, bp, sp);
    return;
  }

  // These types are allowed to alias (or the stored type is unknown), report
  // an error if we find an interior type.

  for (int i = 0; i < size; ++i) {
    OldTDPtr = shadow_for((void *)(((uptr)addr)+i));
    OldTD = *OldTDPtr;
    if (((uptr) OldTD) != ~((uptr)0) && !isAliasingLegal(td, OldTD))
      reportError(addr, size, td, OldTD, AccessStr,
                  "partially accesses an object", i, pc, bp, sp);
  }
}

Flags __tbaasan::flags_data;

SANITIZER_INTERFACE_ATTRIBUTE uptr __tbaasan_shadow_memory_address;
SANITIZER_INTERFACE_ATTRIBUTE uptr __tbaasan_app_memory_mask;

#ifdef TBAASAN_RUNTIME_VMA
// Runtime detected VMA size.
int __tbaasan::vmaSize;
#endif

void Flags::SetDefaults() {
#define TBAASAN_FLAG(Type, Name, DefaultValue, Description) Name = DefaultValue;
#include "tbaasan_flags.inc"
#undef TBAASAN_FLAG
}

static void RegisterTBAASanFlags(FlagParser *parser, Flags *f) {
#define TBAASAN_FLAG(Type, Name, DefaultValue, Description) \
  RegisterFlag(parser, #Name, Description, &f->Name);
#include "tbaasan_flags.inc"
#undef TBAASAN_FLAG
}

static void InitializeFlags() {
  SetCommonFlagsDefaults();
  {
    CommonFlags cf;
    cf.CopyFrom(*common_flags());
    cf.external_symbolizer_path = GetEnv("TBAASAN_SYMBOLIZER_PATH");
    OverrideCommonFlags(cf);
  }

  flags().SetDefaults();

  FlagParser parser;
  RegisterCommonFlags(&parser);
  RegisterTBAASanFlags(&parser, &flags());
  parser.ParseString(GetEnv("TBAASAN_OPTIONS"));
  InitializeCommonFlags();
  if (Verbosity()) ReportUnrecognizedFlags();
  if (common_flags()->help) parser.PrintFlagDescriptions();
}

static void InitializePlatformEarly() {
  AvoidCVE_2016_2143();
#ifdef TBAASAN_RUNTIME_VMA
  vmaSize =
    (MostSignificantSetBitIndex(GET_CURRENT_FRAME()) + 1);
#if defined(__aarch64__)
  if (vmaSize != 39 && vmaSize != 42 && vmaSize != 48) {
    Printf("FATAL: TBAASanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %d - Supported 39, 42 and 48\n", vmaSize);
    Die();
  }
#elif defined(__powerpc64__)
  if (vmaSize != 44 && vmaSize != 46) {
    Printf("FATAL: TBAASanitizer: unsupported VMA range\n");
    Printf("FATAL: Found %d - Supported 44 and 46\n", vmaSize);
    Die();
  }
#endif
#endif

  __tbaasan_shadow_memory_address = ShadowAddr();
  __tbaasan_app_memory_mask = AppMask();
}

namespace __tbaasan {
int tbaasan_inited = 0;
bool tbaasan_init_is_running;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __tbaasan_init() {
  CHECK(!tbaasan_init_is_running);
  if (tbaasan_inited) return;
  tbaasan_init_is_running = 1;

  InitializeFlags();
  InitializePlatformEarly();

  InitializeInterceptors();

  MmapFixedNoReserve(ShadowAddr(), AppAddr() - ShadowAddr());

  tbaasan_init_is_running = 0;
  tbaasan_inited = 1;
}

#if SANITIZER_CAN_USE_PREINIT_ARRAY
__attribute__((section(".preinit_array"), used))
static void (*tbaasan_init_ptr)() = __tbaasan_init;
#endif

