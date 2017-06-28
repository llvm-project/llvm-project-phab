//===- FileEdit.cpp - Split and transform file into multiple sub-files ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// FileEdit takes an input file and performs various operations on the file,
// such as replacing certain sequences or splitting the file into multiple
// sub-files.  This is useful for keeping tests self-contained, as it allows us
// to write a single file containing both test input and test check lines even
// when the test input itself must span multiple files (e.g. a test which tests
// some functionality involving compiling and linking 2 separate compilation
// units).
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
enum class EditResult { Discard, UseModified, UseUnmodified };
struct LineReplacement {
  StringRef Pattern;
  StringRef Repl;
};
} // namespace

static cl::opt<std::string>
    InputFilename(cl::Positional, cl::desc("input file (defaults to stdin)"),
                  cl::Required);

static cl::list<std::string> StripPrefixes(
    "strip-prefix", cl::CommaSeparated,
    cl::desc("Exclude lines beginning with the given prefix from the output"));

static cl::list<std::string> Replacements(
    "replace", cl::ZeroOrMore,
    cl::desc("Apply the specified replacement regex to each line"));

static cl::opt<std::string>
    DestFolder("dest", cl::desc("Write split files to the specified folder"));

static size_t findNextUnescapedSlash(StringRef Str) {
  size_t Pos = 0;
  while (Pos != StringRef::npos) {
    Pos = Str.find_first_of('/');
    if (Pos == StringRef::npos)
      return Pos;
    if (Pos == 0 || Str[Pos - 1] != '\\')
      return Pos;
  }
  return StringRef::npos;
}

static bool parseOneReplacementItem(StringRef Str, StringRef &Pattern,
                                    StringRef &Repl) {
  if (!Str.consume_front("s/"))
    return false;

  for (StringRef *Dest : {&Pattern, &Repl}) {
    size_t NextSlash = findNextUnescapedSlash(Str);
    if (NextSlash == StringRef::npos)
      return false;
    *Dest = Str.take_front(NextSlash);
    Str = Str.drop_front(NextSlash + 1);
  }

  return Str.empty();
}

static void buildReplacementVector(std::vector<LineReplacement> &Repls) {
  std::string UniversalRegexStr;
  std::vector<StringRef> Patterns;
  for (StringRef S : Replacements) {
    StringRef Pattern;
    StringRef Repl;
    if (!parseOneReplacementItem(S, Pattern, Repl)) {
      outs() << formatv("FileEdit: Invalid replacement string `{0}`", S);
      continue;
    }

    Patterns.push_back(Pattern);
    Repls.push_back({Pattern, Repl});
  }
}

static bool handleFileDirective(StringRef Line, StringRef &OutFile) {
  if (!Line.consume_front("{!--"))
    return false;
  OutFile = Line.trim();
  return true;
}

static std::error_code writeOutput(StringRef FileName, StringRef Text) {
  if (FileName.empty()) {
    llvm::outs() << Text;
    return std::error_code();
  }

  SmallString<128> Path;
  if (DestFolder.getNumOccurrences() == 0)
    llvm::sys::fs::current_path(Path);
  else
    Path = DestFolder;
  llvm::sys::path::append(Path, FileName);
  llvm::sys::path::native(Path);
  auto Directory = llvm::sys::path::parent_path(Path);
  if (auto EC = llvm::sys::fs::create_directories(Directory, true))
    return EC;

  int FD;
  if (auto EC =
          llvm::sys::fs::openFileForWrite(Path, FD, llvm::sys::fs::F_Text))
    return EC;
  llvm::raw_fd_ostream OS(FD, true);
  OS << Text;
  OS.flush();
  return std::error_code();
}

static EditResult editLine(StringRef Line,
                           ArrayRef<LineReplacement> Replacements,
                           SmallVectorImpl<char> &ModifiedLine) {
  if (Line.empty())
    return EditResult::UseUnmodified;

  for (StringRef Prefix : StripPrefixes) {
    if (Line.startswith(Prefix))
      return EditResult::Discard;
  }

  // Perform all replacements from left to right in the input string.
  ModifiedLine.clear();
  bool WasModified = false;
  while (!Line.empty()) {
    const LineReplacement *UseRepl = nullptr;
    size_t FirstPos = StringRef::npos;

    // Find the leftmost replacement in the input string, and map it back to
    // the text that should replace it.
    for (const LineReplacement &LR : Replacements) {
      size_t Pos = Line.find(LR.Pattern);
      if (Pos < FirstPos) {
        UseRepl = &LR;
        FirstPos = Pos;
      }
    }
    if (!UseRepl) {
      if (!WasModified) {
        // If we didn't find anything to replace on this or any previous
        // iteration we can just exit and use the original line.
        return EditResult::UseUnmodified;
      }

      ModifiedLine.append(Line.begin(), Line.end());
      return EditResult::UseModified;
    }

    // The line can be split into 3 groups.  Line = Left | Repl | Right.  Append
    // `Left` as is, use the replacement value for `Repl`, and keep `Right` as
    // the remainder of the string for further replacement.
    StringRef Left = Line.take_front(FirstPos);

    ModifiedLine.append(Left.begin(), Left.end());
    Line = Line.drop_front(FirstPos);

    ModifiedLine.append(UseRepl->Repl.begin(), UseRepl->Repl.end());
    WasModified = true;
    Line = Line.drop_front(UseRepl->Pattern.size());
  }
  return EditResult::UseModified;
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);

  // Read the expected strings from the check file.
  ErrorOr<std::unique_ptr<MemoryBuffer>> InFileOrErr =
      MemoryBuffer::getFileOrSTDIN(InputFilename);
  if (std::error_code EC = InFileOrErr.getError()) {
    errs() << "Could not open input file '" << InputFilename
           << "': " << EC.message() << '\n';
    return 1;
  }
  MemoryBuffer &InFile = *InFileOrErr.get();

  std::vector<LineReplacement> Replacements;

  buildReplacementVector(Replacements);

  SmallString<4096> InputFileBuffer;

  line_iterator Iter(InFile, false);
  line_iterator End;
  StringRef OutFile;
  std::string OutputText;
  SmallString<80> ModifiedLine;
  for (StringRef Line : make_range(Iter, End)) {
    StringRef NewOutFile;
    if (handleFileDirective(Line, NewOutFile)) {
      writeOutput(OutFile, OutputText);
      OutFile = NewOutFile;
      OutputText.clear();
      continue;
    }

    EditResult Result = editLine(Line, Replacements, ModifiedLine);
    if (Result == EditResult::Discard)
      continue;
    if (Result == EditResult::UseModified)
      Line = ModifiedLine;
    if (!OutputText.empty())
      OutputText += "\n";
    OutputText += Line;
  }

  writeOutput(OutFile, OutputText);
  return 0;
}
