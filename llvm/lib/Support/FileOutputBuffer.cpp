//===- FileOutputBuffer.cpp - File Output Buffer ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Utility for creating a in-memory buffer that will be written to a file.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/FileOutputBuffer.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Memory.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include <system_error>

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace llvm;
using namespace llvm::sys;

// A FileOutputBuffer which creates a temporary file in the same directory
// as the final output file. The final output file is atomically replaced
// with the temporary file on commit().
class OnDiskBuffer : public FileOutputBuffer {
public:
  OnDiskBuffer(StringRef Path, StringRef TempPath,
               std::unique_ptr<fs::mapped_file_region> Buf)
      : FileOutputBuffer(Path), Buffer(std::move(Buf)), TempPath(TempPath) {}

  uint8_t *getBufferStart() const override { return (uint8_t *)Buffer->data(); }

  uint8_t *getBufferEnd() const override {
    return (uint8_t *)Buffer->data() + Buffer->size();
  }

  size_t getBufferSize() const override { return Buffer->size(); }

  Error commit() override {
    // Unmap buffer, letting OS flush dirty pages to file on disk.
    Buffer.reset();

    // Atomically replace the existing file with the new one.
    auto EC = fs::rename(TempPath, FinalPath);
    sys::DontRemoveFileOnSignal(TempPath);
    return errorCodeToError(EC);
  }

  ~OnDiskBuffer() override {
    // Close the mapping before deleting the temp file, so that the removal
    // succeeds.
    Buffer.reset();
    fs::remove(TempPath);
  }

private:
  std::unique_ptr<fs::mapped_file_region> Buffer;
  std::string TempPath;
};

// A FileOutputBuffer which keeps data in memory and writes to the final
// output file on commit(). This is used only when we cannot use OnDiskBuffer.
class InMemoryBuffer : public FileOutputBuffer {
public:
  InMemoryBuffer(StringRef Path, MemoryBlock Buf, unsigned Mode)
      : FileOutputBuffer(Path), Buffer(Buf), Mode(Mode) {}

  uint8_t *getBufferStart() const override { return (uint8_t *)Buffer.base(); }

  uint8_t *getBufferEnd() const override {
    return (uint8_t *)Buffer.base() + Buffer.size();
  }

  size_t getBufferSize() const override { return Buffer.size(); }

  Error commit() override {
    int FD;
    std::error_code EC;
    if (auto EC = openFileForWrite(FinalPath, FD, fs::F_None, Mode))
      return errorCodeToError(EC);
    raw_fd_ostream OS(FD, /*shouldClose=*/true, /*unbuffered=*/true);
    OS << StringRef((const char *)Buffer.base(), Buffer.size());
    return Error::success();
  }

private:
  OwningMemoryBlock Buffer;
  unsigned Mode;
};

static Expected<std::unique_ptr<InMemoryBuffer>>
createInMemoryBuffer(StringRef Path, size_t Size, unsigned Mode) {
  std::error_code EC;
  MemoryBlock MB = Memory::allocateMappedMemory(
      Size, nullptr, sys::Memory::MF_READ | sys::Memory::MF_WRITE, EC);
  if (EC)
    return errorCodeToError(EC);
  return llvm::make_unique<InMemoryBuffer>(Path, MB, Mode);
}

static Expected<std::unique_ptr<FileOutputBuffer>>
createOnDiskBuffer(StringRef Path, size_t Size, unsigned Mode) {
  // Create new file in same directory but with random name.
  SmallString<128> TempPath;
  int FD;
  if (auto EC = fs::createUniqueFile(Path + ".tmp%%%%%%%", FD, TempPath, Mode))
    return errorCodeToError(EC);

  sys::RemoveFileOnSignal(TempPath);

#ifndef LLVM_ON_WIN32
  // A signal (usually a SIGBUS) is raised when a disk becomes full while
  // writing to a sparse file using mmap. There's no portable nor reliable
  // way to handle such error condition. In order to prevent that error, we
  // extend a file and then preallocate disk blocks now.
  //
  // Not all operating systems nor filesystems support block preallocation.
  // If it is not supported, we use an in-memory buffer instead so that the
  // disk full error can be caught on commit().
  //
  // On Windows, we don't need to do this because CreateFileMapping (the
  // mmap function on Windows) automatically extends an undeflying file.
  // Pre-extending a file is just a waste of time.
  if (auto EC = fs::allocate_file(FD, Size)) {
    fs::remove(TempPath);
    if (EC == errc::function_not_supported)
      return createInMemoryBuffer(Path, Size, Mode);
    return errorCodeToError(EC);
  }
#endif

  // Mmap it.
  std::error_code EC;
  auto MappedFile = llvm::make_unique<fs::mapped_file_region>(
      FD, fs::mapped_file_region::readwrite, Size, 0, EC);
  close(FD);
  if (EC)
    return errorCodeToError(EC);
  return llvm::make_unique<OnDiskBuffer>(Path, TempPath, std::move(MappedFile));
}

// Create an instance of FileOutputBuffer.
Expected<std::unique_ptr<FileOutputBuffer>>
FileOutputBuffer::create(StringRef Path, size_t Size, unsigned Flags) {
  unsigned Mode = fs::all_read | fs::all_write;
  if (Flags & F_executable)
    Mode |= fs::all_exe;

  fs::file_status Stat;
  fs::status(Path, Stat);

  // Usually, we want to create OnDiskBuffer to create a temporary file in
  // the same directory as the destination file and atomically replaces it
  // by rename(2).
  //
  // However, if the destination file is a special file, we don't want to
  // use rename (e.g. we don't want to replace /dev/null with a regular
  // file.) If that's the case, we create an in-memory buffer, open the
  // destination file and write to it on commit().
  switch (Stat.type()) {
  case fs::file_type::directory_file:
    return errorCodeToError(errc::is_a_directory);
  case fs::file_type::regular_file:
  case fs::file_type::file_not_found:
  case fs::file_type::status_error:
    return createOnDiskBuffer(Path, Size, Mode);
  default:
    return createInMemoryBuffer(Path, Size, Mode);
  }
}
