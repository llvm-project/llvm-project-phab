//===-- ProcessElfCore.cpp --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/// Core files PT_NOTE segment descriptor types
enum {
  NT_PRSTATUS = 1,
  NT_PRPSINFO = 3
};

namespace FREEBSD {
  enum {
    NT_PRSTATUS = 1,
    NT_FPREGSET,
    NT_PRPSINFO,
    NT_THRMISC = 7,
    NT_PROCSTAT_AUXV = 16,
    NT_PPC_VMX = 0x100
  };
}

namespace NETBSD {
  enum {
    NT_PROCINFO = 1,
    NT_AUXV,
    NT_AMD64_REGS = 33,
    NT_AMD64_FPREGS = 35
  };
}

namespace OPENBSD {
  enum {
    NT_PROCINFO = 10,
    NT_AUXV = 11,
    NT_REGS = 20,
    NT_FPREGS = 21,
  };
}

namespace LINUX {
  enum {
    NT_PRSTATUS = 1,
    NT_FPREGSET,
    NT_PRPSINFO,
    NT_TASKSTRUCT,
    NT_PLATFORM,
    NT_AUXV,
    NT_FILE = 0x46494c45,
    NT_SIGINFO = 0x53494749,
    NT_PPC_VMX = 0x100,
    NT_PPC_VSX = 0x102,
    NT_PRXFPREG = 0x46e62b7f,
  };
}
