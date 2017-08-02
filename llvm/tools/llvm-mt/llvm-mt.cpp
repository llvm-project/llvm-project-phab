//===- llvm-mt.cpp - Merge .manifest files ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// Merge .manifest files.  This is intended to be a platform-independent port
// of Microsoft's mt.exe.
//
//===---------------------------------------------------------------------===//

#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/WindowsManifest/WindowsManifestMerger.h"

#include <system_error>

using namespace llvm;

namespace {

enum ID {
  OPT_INVALID = 0, // This is not an option ID.
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  OPT_##ID,
#include "Opts.inc"
#undef OPTION
};

#define PREFIX(NAME, VALUE) const char *const NAME[] = VALUE;
#include "Opts.inc"
#undef PREFIX

static const opt::OptTable::Info InfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
{                                                                              \
      PREFIX,      NAME,      HELPTEXT,                                        \
      METAVAR,     OPT_##ID,  opt::Option::KIND##Class,                        \
      PARAM,       FLAGS,     OPT_##GROUP,                                     \
      OPT_##ALIAS, ALIASARGS, VALUES},
#include "Opts.inc"
#undef OPTION
};

class CvtResOptTable : public opt::OptTable {
public:
  CvtResOptTable() : OptTable(InfoTable, true) {}
};

static ExitOnError ExitOnErr;
} // namespace

LLVM_ATTRIBUTE_NORETURN void reportError(Twine Msg) {
  errs() << "llvm-mt error: " << Msg << "\n";
  exit(1);
}

static void reportError(StringRef Input, std::error_code EC) {
  reportError(Twine(Input) + ": " + EC.message());
}

void error(std::error_code EC) {
  if (EC)
    reportError(EC.message());
}

void error(Error EC) {
  if (EC)
    handleAllErrors(std::move(EC), [&](const ErrorInfoBase &EI) {
      reportError(EI.message());
    });
}

int main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);

  ExitOnErr.setBanner("llvm-mt: ");

  SmallVector<const char *, 256> argv_buf;
  SpecificBumpPtrAllocator<char> ArgAllocator;
  ExitOnErr(errorCodeToError(sys::Process::GetArgumentVector(
      argv_buf, makeArrayRef(argv, argc), ArgAllocator)));
  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  CvtResOptTable T;
  unsigned MAI, MAC;
  ArrayRef<const char *> ArgsArr = makeArrayRef(argv + 1, argc);
  opt::InputArgList InputArgs = T.ParseArgs(ArgsArr, MAI, MAC);

  if (MAC) {
    outs() << "invalid\n";
  }

  // for (auto &Arg : InputArgs) {
  //   outs() << "checking valid\n";
  //   outs() << Arg->getOption().getID() << "\n";
  //   if (!Arg->getOption().isValid()) {
  //     reportError(Twine("invalid option ") + Arg->getOption().getName());
  //   }
  // }

  for (auto &Arg : InputArgs) {
    if (!(Arg->getOption().matches(OPT_unsupported) ||
          Arg->getOption().matches(OPT_supported))) {
      reportError(Twine("invalid option ") + Arg->getSpelling());
    }
  }

  for (auto &Arg : InputArgs) {
    if (Arg->getOption().matches(OPT_unsupported)) {
      outs() << "llvm-mt: ignoring unsupported '" << Arg->getOption().getName()
             << "' option\n";
    }
  }

  if (InputArgs.hasArg(OPT_help)) {
    T.PrintHelp(outs(), "mt", "Manifest Tool", false);
    return 0;
  }

  std::vector<std::string> InputFiles = InputArgs.getAllArgValues(OPT_manifest);

  // for (int i = 0; i < 2; i++) {
  //   switch (i) {
  //   case 0:
  //     outs() << "If additional dominant\n";
  //     break;
  //   case 1:
  //     outs() << "Else\n";
  //     break;
  //   }
  //   for (int j = 0; j < 2; j++) {
  //     outs() << "\t";
  //     switch (j) {
  //     case 0:
  //       outs() << "If dominant defines default\n";
  //       break;
  //     case 1:
  //       outs() << "Else\n";
  //       break;
  //     }
  //     for (int k = 0; k < 2; k++) {
  //       outs() << "\t\t";
  //       switch (k) {
  //       case 0:
  //         outs() << "If dominant defines prefix\n";
  //         break;
  //       case 1:
  //         outs() << "Else\n";
  //         break;
  //       }
  //       for (int l = 0; l < 4; l++) {
  //         outs() << "\t\t\t";
  //         switch (l) {
  //         case 0:
  //           outs() << "If dominant namespace is inherited default\n";
  //           break;
  //         case 1:
  //           outs() << "If dominant namespace is inherited prefix\n";
  //           break;
  //         case 2:
  //           outs() << "If dominant namespace is defined default\n";
  //           break;
  //         case 3:
  //           outs() << "If dominant namespace is defined prefix\n";
  //           break;
  //         }
  //         for (int m = 0; m < 2; m++) {
  //           outs() << "\t\t\t\t";
  //           switch (m) {
  //           case 0:
  //             outs() << "If nondominant defines default\n";
  //             break;
  //           case 1:
  //             outs() << "Else\n";
  //             break;
  //           }
  //           for (int n = 0; n < 2; n++) {
  //             outs() << "\t\t\t\t\t";
  //             switch (n) {
  //             case 0:
  //               outs() << "If nondominant defines prefix\n";
  //               break;
  //             case 1:
  //               outs() << "Else\n";
  //               break;
  //             }
  //             for (int o = 0; o < 4; o++) {
  //               outs() << "\t\t\t\t\t\t";
  //               switch (o) {
  //               case 0:
  //                 outs() << "If nondominant namespace is inherited
  //                 default\n"; break;
  //               case 1:
  //                 outs() << "If nondominant namespace is inherited prefix\n";
  //                 break;
  //               case 2:
  //                 outs() << "If nondominant namespace is defined default\n";
  //                 break;
  //               case 3:
  //                 outs() << "If nondominant namespace is defined prefix\n";
  //                 break;
  //               }
  //             }
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  if (InputFiles.size() == 0) {
    reportError("no input file specified");
  }

  StringRef OutputFile;
  if (InputArgs.hasArg(OPT_out)) {
    OutputFile = InputArgs.getLastArgValue(OPT_out);
  } else if (InputFiles.size() == 1) {
    OutputFile = InputFiles[0];
  } else {
    reportError("no output file specified");
  }

  windows_manifest::WindowsManifestMerger Merger;

  for (const auto &File : InputFiles) {
    ErrorOr<std::unique_ptr<MemoryBuffer>> ManifestOrErr =
        MemoryBuffer::getFile(File);
    if (!ManifestOrErr)
      reportError(File, ManifestOrErr.getError());
    MemoryBuffer &Manifest = *ManifestOrErr.get();
    error(Merger.merge(Manifest));
  }

  std::unique_ptr<MemoryBuffer> OutputBuffer = Merger.getMergedManifest();
  if (!OutputBuffer)
    reportError("empty manifest not written");
  ErrorOr<std::unique_ptr<FileOutputBuffer>> FileOrErr =
      FileOutputBuffer::create(OutputFile, OutputBuffer->getBufferSize());
  if (!FileOrErr)
    reportError(OutputFile, FileOrErr.getError());
  std::unique_ptr<FileOutputBuffer> FileBuffer = std::move(*FileOrErr);
  std::copy(OutputBuffer->getBufferStart(), OutputBuffer->getBufferEnd(),
            FileBuffer->getBufferStart());
  error(FileBuffer->commit());
  return 0;
}
