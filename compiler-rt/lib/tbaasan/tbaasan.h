//===-- tbaasan.h -------------------------------------------------*- C++ -*-===//
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
// Private TBAASan header.
//===----------------------------------------------------------------------===//

#ifndef TBAASAN_H
#define TBAASAN_H

#include "sanitizer_common/sanitizer_internal_defs.h"

using __sanitizer::uptr;
using __sanitizer::u16;

#include "tbaasan_platform.h"

enum {
  TBAASAN_MEMBER_TD = 1,
  TBAASAN_STRUCT_TD = 2
};

struct tbaasan_member_type_descriptor {
  struct tbaasan_type_descriptor *Base;
  struct tbaasan_type_descriptor *Access;
  uptr Offset;
};

struct tbaasan_struct_type_descriptor {
  uptr MemberCount;
  struct {
    struct tbaasan_type_descriptor *Type;
    uptr Offset;
  } Members[1]; // Tail allocated.
  // char Name[]; // Tail allocated.
};

struct tbaasan_type_descriptor {
  uptr Tag;
  union {
    tbaasan_member_type_descriptor Member;
    tbaasan_struct_type_descriptor Struct;
  };
};

extern "C" {
void tbaasan_set_type_unknown(void *addr, uptr size);
void tbaasan_copy_types(void *daddr, void *saddr, uptr size);
}

namespace __tbaasan {
extern bool tbaasan_init_is_running;

void InitializeInterceptors();

inline tbaasan_type_descriptor **shadow_for(void *ptr) {
  return (tbaasan_type_descriptor **)
    ((((uptr) ptr) & AppMask())*sizeof(ptr) + ShadowAddr());
}

struct Flags {
#define TBAASAN_FLAG(Type, Name, DefaultValue, Description) Type Name;
#include "tbaasan_flags.inc"
#undef TBAASAN_FLAG

  void SetDefaults();
};

extern Flags flags_data;
inline Flags &flags() {
  return flags_data;
}

}  // namespace __tbaasan

#endif  // TBAASAN_H
