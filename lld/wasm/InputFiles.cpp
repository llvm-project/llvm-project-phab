//===- InputFiles.cpp -----------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"

#include "Config.h"
#include "Driver.h"
#include "Error.h"
#include "Memory.h"
#include "Strings.h"
#include "SymbolTable.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/Wasm.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "lld"

using namespace lld;
using namespace lld::wasm;

using namespace llvm;
using namespace llvm::object;
using namespace llvm::wasm;

namespace lld {
namespace wasm {

Optional<MemoryBufferRef> readFile(StringRef Path) {
  log("Loading: " + Path);

  auto MBOrErr = MemoryBuffer::getFile(Path);
  if (auto EC = MBOrErr.getError()) {
    error("cannot open " + Path + ": " + EC.message());
    return None;
  }
  std::unique_ptr<MemoryBuffer> &MB = *MBOrErr;
  MemoryBufferRef MBRef = MB->getMemBufferRef();
  make<std::unique_ptr<MemoryBuffer>>(std::move(MB)); // take MB ownership

  return MBRef;
}

void ObjectFile::dumpInfo() const {
  log("reloc info for: " + getName() + "\n" +
      "        FunctionIndexOffset : " + Twine(FunctionIndexOffset) + "\n" +
      "     FunctionImports.size() : " + Twine(FunctionImports.size()) + "\n" +
      "           TableIndexOffset : " + Twine(TableIndexOffset) + "\n" +
      "          GlobalIndexOffset : " + Twine(GlobalIndexOffset) + "\n" +
      "                 DataOffset : " + Twine(DataOffset) + "\n" +
      "       GlobalImports.size() : " + Twine(GlobalImports.size()) + "\n");
}

bool ObjectFile::isImportedFunction(uint32_t Index) const {
  return Index < FunctionImports.size();
}

bool ObjectFile::isImportedGlobal(uint32_t Index) const {
  return Index < GlobalImports.size();
}

bool ObjectFile::isResolvedFunctionImport(uint32_t Index) const {
  if (!isImportedFunction(Index))
    return false;
  StringRef Name = FunctionImports[Index];
  Symbol *Sym = Symtab->find(Name);
  assert(Sym);
  return Sym->isDefined();
}

bool ObjectFile::isResolvedGlobalImport(uint32_t Index) const {
  if (!isImportedGlobal(Index))
    return false;
  StringRef Name = GlobalImports[Index];
  Symbol *Sym = Symtab->find(Name);
  assert(Sym);
  return Sym->isDefined();
}

int32_t ObjectFile::getGlobalAddress(uint32_t Index) const {
  if (isImportedGlobal(Index))
    return 0;

  const WasmGlobal &Global = WasmObj->globals()[Index - GlobalImports.size()];
  assert(Global.Type == WASM_TYPE_I32);
  return Global.InitExpr.Value.Int32 + DataOffset;
}

uint32_t ObjectFile::relocateFunctionIndex(uint32_t Original) const {
  DEBUG(dbgs() << "relocateFunctionIndex: " << Original << "\n");
  if (isImportedFunction(Original)) {
    StringRef Name = FunctionImports[Original];
    Symbol *Sym = Symtab->find(Name);
    assert(Sym && "imported symbol not found in symbol table");
    return Sym->getOutputIndex();
  }

  DEBUG(dbgs() << " -> " << FunctionIndexOffset << " "
               << (Original + FunctionIndexOffset) << "\n");
  return Original + FunctionIndexOffset;
}

uint32_t ObjectFile::relocateTypeIndex(uint32_t Original) const {
  assert(TypeMap.count(Original) > 0);
  return TypeMap.find(Original)->second;
}

uint32_t ObjectFile::relocateTableIndex(uint32_t Original) const {
  return Original + TableIndexOffset;
}

uint32_t ObjectFile::relocateGlobalIndex(uint32_t Original) const {
  if (isImportedGlobal(Original)) {
    StringRef Name = GlobalImports[Original];
    Symbol *Sym = Symtab->find(Name);
    assert(Sym && "imported symbol not found in symbol table");
    return Sym->getOutputIndex();
  }

  return Original + GlobalIndexOffset;
}

void ObjectFile::parse() {
  // Parse a memory buffer as a wasm file.
  DEBUG(dbgs() << "Parsing object: " << toString(this) << "\n");
  std::unique_ptr<Binary> Bin = check(createBinary(MB), toString(this));

  auto *Obj = dyn_cast<WasmObjectFile>(Bin.get());
  if (!Obj)
    fatal(toString(this) + ": not a wasm file");
  if (!Obj->isRelocatableObject())
    fatal(toString(this) + ": not a relocatable wasm file");

  Bin.release();
  WasmObj.reset(Obj);

  // Find the code and data sections.  Wasm objects can have at most one code
  // and one data section.
  for (const SectionRef &Sec : WasmObj->sections()) {
    const WasmSection &Section = WasmObj->getWasmSection(Sec);
    if (Section.Type == WASM_SEC_CODE)
      CodeSection = &Section;
    else if (Section.Type == WASM_SEC_DATA)
      DataSection = &Section;
  }

  initializeSymbols();
}

void ObjectFile::initializeSymbols() {
  Symbols.reserve(WasmObj->getNumberOfSymbols());

  for (const WasmImport &Import : WasmObj->imports()) {
    switch (Import.Kind) {
    case WASM_EXTERNAL_FUNCTION:
      FunctionImports.emplace_back(Import.Field);
      break;
    case WASM_EXTERNAL_GLOBAL:
      GlobalImports.emplace_back(Import.Field);
      break;
    }
  }

  for (const SymbolRef &Sym : WasmObj->symbols()) {
    const WasmSymbol &WasmSym = WasmObj->getWasmSymbol(Sym.getRawDataRefImpl());
    switch (WasmSym.Type) {
    case WasmSymbol::SymbolType::FUNCTION_IMPORT:
    case WasmSymbol::SymbolType::GLOBAL_IMPORT:
      createUndefined(WasmSym);
      break;
    case WasmSymbol::SymbolType::FUNCTION_EXPORT:
    case WasmSymbol::SymbolType::GLOBAL_EXPORT:
      createDefined(WasmSym);
      break;
    case WasmSymbol::SymbolType::DEBUG_FUNCTION_NAME:
      // These are internal only, no need to create linker symbols for them
      break;
    }
  }
}

Symbol *ObjectFile::createUndefined(const WasmSymbol &Sym) {
  Symbol *S = Symtab->addUndefined(this, &Sym);
  Symbols.push_back(S);
  return S;
}

Symbol *ObjectFile::createDefined(const WasmSymbol &Sym) {
  Symbol *S = Symtab->addDefined(this, &Sym);
  Symbols.push_back(S);
  return S;
}

void ArchiveFile::parse() {
  // Parse a MemoryBufferRef as an archive file.
  DEBUG(dbgs() << "Parsing library: " << toString(this) << "\n");
  File = check(Archive::create(MB), toString(this));

  // Read the symbol table to construct Lazy symbols.
  for (const Archive::Symbol &Sym : File->symbols())
    Symtab->addLazy(this, &Sym);
}

void ArchiveFile::addMember(const Archive::Symbol *Sym) {
  const Archive::Child &C =
      check(Sym->getMember(),
            "could not get the member for symbol " + Sym->getName());

  // Don't try to load the same member twice (this can happen when members
  // mutually reference each other.
  if (!Seen.insert(C.getChildOffset()).second)
    return;

  DEBUG(dbgs() << "loading lazy: " << displayName(Sym->getName()) << "\n");
  DEBUG(dbgs() << "from archive: " << toString(this) << "\n");
  // DEBUG(dbgs() << "loading symbol from object symbol: " << C.getName() <<
  // "\n");
  MemoryBufferRef MB =
      check(C.getMemoryBufferRef(),
            "could not get the buffer for the member defining symbol " +
                Sym->getName());
  Driver->addArchiveBuffer(MB, Sym->getName(), ParentName);
}

} // namespace wasm
} // namespace lld

// Returns the last element of a path, which is supposed to be a filename.
static StringRef getBasename(StringRef Path) {
  size_t Pos = Path.find_last_of("\\/");
  if (Pos == StringRef::npos)
    return Path;
  return Path.substr(Pos + 1);
}

// Returns a string in the format of "foo.obj" or "foo.obj(bar.lib)".
std::string lld::toString(wasm::InputFile *File) {
  if (!File)
    return "(internal)";

  if (File->ParentName.empty())
    return File->getName();

  std::string Res =
      (getBasename(File->ParentName) + "(" + getBasename(File->getName()) + ")")
          .str();
  return StringRef(Res);
}
