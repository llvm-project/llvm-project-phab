//===- MicrosoftDemangle.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a demangler for MSVC-style mangled symbols.
//
// This file has no dependencies on the rest of LLVM so that it can be
// easily reused in other programs such as libcxxabi.
//
//===----------------------------------------------------------------------===//

#include "llvm/Demangle/Demangle.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// A string class that does not own its contents.
// This class provides a few utility functions for string manipulations.
class String {
public:
  String() = default;
  String(const String &other) = default;
  String(const std::string &s) : p(s.data()), len(s.size()) {}
  String(const char *p) : p(p), len(strlen(p)) {}
  String(const char *p, size_t len) : p(p), len(len) {}
  template <size_t N> String(const char (&p)[N]) : p(p), len(N - 1) {}

  std::string str() const { return {p, p + len}; }

  bool empty() const { return len == 0; }

  bool startswith(const std::string &s) {
    return s.size() <= len && strncmp(p, s.data(), s.size()) == 0;
  }

  ssize_t find(const std::string &s) const {
    if (s.size() > len)
      return -1;

    for (size_t i = 0; i < len - s.size(); ++i)
      if (strncmp(p + i, s.data(), s.size()) == 0)
        return i;
    return -1;
  }

  void trim(size_t n) {
    assert(n <= len);
    p += n;
    len -= n;
  }

  String substr(size_t start) const { return {p + start, len}; }
  String substr(size_t start, size_t end) const { return {p + start, end}; }

  const char *p = nullptr;
  size_t len = 0;
};

std::ostream &operator<<(std::ostream &os, const String s) {
  os.write(s.p, s.len);
  return os;
}

// Storage classes
enum {
  Const = 1 << 0,
  Volatile = 1 << 1,
  Far = 1 << 2,
  Huge = 1 << 3,
  Unaligned = 1 << 4,
  Restrict = 1 << 5,
};

// Calling conventions
enum CallingConv : uint8_t {
  Cdecl,
  Pascal,
  Thiscall,
  Stdcall,
  Fastcall,
  Regcall,
};

// Types
enum PrimTy : uint8_t {
  Unknown,
  None,
  Function,
  Ptr,
  Array,

  Struct,
  Union,
  Class,
  Enum,

  Void,
  Bool,
  Char,
  Schar,
  Uchar,
  Short,
  Ushort,
  Int,
  Uint,
  Long,
  Ulong,
  Llong,
  Ullong,
  Wchar,
  Float,
  Double,
  Ldouble,
  M64,
  M128,
  M128d,
  M128i,
  M256,
  M256d,
  M256i,
  M512,
  M512d,
  M512i,
  Varargs,
};

// Function classes
enum FuncClass : uint8_t {
  Public = 1 << 0,
  Protected = 1 << 1,
  Private = 1 << 2,
  Global = 1 << 3,
  Static = 1 << 4,
  Virtual = 1 << 5,
  FFar = 1 << 6,
};

// The type class. Mangled symbols are first parsed and converted to
// this type and then converted to string.
struct Type {
  PrimTy prim;
  Type *ptr = nullptr;
  uint8_t sclass = 0;

  CallingConv calling_conv;
  FuncClass func_class;

  int32_t len; // valid if prim == Array

  // Valid if prim is one of (Struct, Union, Class, Enum).
  String name;

  // Function or template parameters.
  std::vector<struct Type *> params;
};

// Demangler class takes the main role in demangling symbols.
// It has a set of functions to parse mangled symbols into Type instnaces.
// It also have a set of functions to cnovert Type instances to strings.
namespace {
class Demangler {
public:
  Demangler(String s) : input(s) {}

  // You are supposed to call parse() first and then check if Error is
  // still empty. After that, call str() to get a result.
  void parse();
  std::string str();

  // Error string. Empty if there's no error.
  std::string error;

private:
  // Parser functions. This is a recursive-descendent parser.
  void read_var_type(Type &ty);
  void read_member_func_type(Type &ty);

  int read_number();
  String read_string();
  String read_until(const std::string &s);
  PrimTy read_prim_type();
  int read_func_class();
  CallingConv read_calling_conv();
  void read_func_return_type(Type &ty);
  int8_t read_storage_class();

  Type *alloc() {
    if (type_index < sizeof(type_buffer) / sizeof(*type_buffer))
      return type_buffer + type_index++;
    Type *ty = new Type();
    type_buffer2.emplace_back(ty);
    return ty;
  }

  bool consume(const std::string &s) {
    if (!input.startswith(s))
      return false;
    input.trim(s.size());
    return true;
  }

  // Mangled symbol. read_* functions shortens this string as
  // they parse it.
  String input;

  // A parsed mangled symbol.
  Type type;

  // The main symbol name. (e.g. "x" in "int x".)
  String symbol;

  // We want to reduce number of memory allocations. To do so,
  // we allocate a fixed number of Type instnaces as part of Demangler.
  // If it needs more Type instances, we dynamically allocate instances
  // and manage them using type_buffer2.
  Type type_buffer[20];
  size_t type_index = 0;
  std::vector<std::unique_ptr<Type>> type_buffer2;

  // Functions to convert Type to String.
  void write_pre(Type &type);
  void write_post(Type &type);
  void write_params(Type &type);
  void write_name(String s);
  void write_space();

  // The result is written to this stream.
  std::stringstream os;
};
} // namespace

// Parser entry point.
void Demangler::parse() {
  // MSVC-style mangled symbols must start with '?'.
  if (!consume("?")) {
    symbol = input;
    type.prim = Unknown;
  }

  // What follows is a main symbol name. This may include
  // namespaces or class names.
  symbol = read_string();

  // Read a variable
  if (consume("3")) {
    read_var_type(type);
    return;
  }

  // Read a non-member function.
  if (consume("Y")) {
    type.prim = Function;
    type.calling_conv = read_calling_conv();

    int8_t sclass = read_storage_class();

    type.ptr = alloc();
    read_var_type(*type.ptr);
    type.ptr->sclass = sclass;

    while (error.empty() && !input.empty() && !input.startswith("@")) {
      Type *tp = alloc();
      read_var_type(*tp);
      type.params.push_back(tp);
    }
    return;
  }

  // Read a member function.
  type.prim = Function;
  type.func_class = (FuncClass)read_func_class();
  consume("E"); // if 64 bit

  int8_t sclass = read_storage_class();

  type.calling_conv = read_calling_conv();

  type.ptr = alloc();
  read_func_return_type(*type.ptr);
  type.ptr->sclass = sclass;

  while (error.empty() && !input.empty() && !input.startswith("Z")) {
    Type *tp = alloc();
    read_var_type(*tp);
    type.params.push_back(tp);
  }
}

// Sometimes numbers are encoded in mangled symbols. For example,
// "int (*x)[20]" is a valid C type (x is a pointer to an array of
// length 20), so we need some way to embed numbers as part of symbols.
// This function parses it.
//
// <number>               ::= [?] <non-negative integer>
//
// <non-negative integer> ::= <decimal digit> # when 1 <= Number <= 10
//                        ::= <hex digit>+ @  # when Numbrer == 0 or >= 10
//
// <hex-digit>            ::= [A-P]           # A = 0, B = 1, ...
int Demangler::read_number() {
  bool neg = consume("?");

  if (0 < input.len && '0' <= *input.p && *input.p <= '9') {
    int32_t ret = *input.p - '0' + 1;
    input.trim(1);
    return neg ? -ret : ret;
  }

  size_t i = 0;
  int32_t ret = 0;
  for (; i < input.len; ++i) {
    char c = input.p[i];
    if (c == '@') {
      input.trim(i + 1);
      return neg ? -ret : ret;
    }
    if ('A' <= c && c <= 'P') {
      ret = (ret << 4) + (c - 'A');
      continue;
    }
    break;
  }
  error = "bad number";
  return 0;
}

String Demangler::read_string() { return read_until("@@"); }

// Reads input until delim is found.
String Demangler::read_until(const std::string &delim) {
  ssize_t len = input.find(delim);
  if (len < 0) {
    error = "read_until";
    return "";
  }
  String ret = input.substr(0, len);
  input.trim(len + delim.size());
  return ret;
}

int Demangler::read_func_class() {
  if (consume("A"))
    return Private;
  if (consume("B"))
    return Private | FFar;
  if (consume("C"))
    return Private | Static;
  if (consume("D"))
    return Private | Static;
  if (consume("E"))
    return Private | Virtual;
  if (consume("F"))
    return Private | Virtual;
  if (consume("I"))
    return Protected;
  if (consume("J"))
    return Protected | FFar;
  if (consume("K"))
    return Protected | Static;
  if (consume("L"))
    return Protected | Static | FFar;
  if (consume("M"))
    return Protected | Virtual;
  if (consume("N"))
    return Protected | Virtual | FFar;
  if (consume("Q"))
    return Public;
  if (consume("R"))
    return Public | FFar;
  if (consume("S"))
    return Public | Static;
  if (consume("T"))
    return Public | Static | FFar;
  if (consume("U"))
    return Public | Virtual;
  if (consume("V"))
    return Public | Virtual | FFar;
  if (consume("Y"))
    return Global;
  if (consume("Z"))
    return Global | FFar;
  error = "unknown func class: " + input.str();
  return 0;
}

CallingConv Demangler::read_calling_conv() {
  if (consume("A"))
    return Cdecl;
  if (consume("C"))
    return Pascal;
  if (consume("E"))
    return Thiscall;
  if (consume("G"))
    return Stdcall;
  if (consume("I"))
    return Fastcall;
  if (consume("E"))
    return Regcall;
  error = "unknown calling convention: " + input.str();
  return Cdecl;
};

// <return-type> ::= <type>
//               ::= @ # structors (they have no declared return type)
void Demangler::read_func_return_type(Type &ty) {
  if (consume("@")) {
    ty.prim = None;
    return;
  }
  read_var_type(ty);
  consume("@"); // expect
}

int8_t Demangler::read_storage_class() {
  if (consume("A"))
    return 0;
  if (consume("B"))
    return Const;
  if (consume("C"))
    return Volatile;
  if (consume("D"))
    return Const | Volatile;
  if (consume("E"))
    return Far;
  if (consume("F"))
    return Const | Far;
  if (consume("G"))
    return Volatile | Far;
  if (consume("H"))
    return Const | Volatile | Far;
  if (consume("I"))
    return Huge;
  if (consume("F"))
    return Unaligned;
  if (consume("I"))
    return Restrict;
  return 0;
}

// Reads a variable type.
void Demangler::read_var_type(Type &ty) {
  if (consume("T")) {
    ty.prim = Union;
    ty.name = read_string();
    return;
  }

  if (consume("U")) {
    ty.prim = Struct;
    ty.name = read_string();
    return;
  }

  if (consume("V")) {
    ty.prim = Class;

    if (consume("?$")) {
      ty.name = read_until("@");
      while (error.empty() && !consume("@")) {
        Type *tp = alloc();
        read_var_type(*tp);
        ty.params.push_back(tp);
      }
      return;
    }

    ty.name = read_string();
    return;
  }

  if (consume("W4")) {
    ty.prim = Enum;
    ty.name = read_string();
    return;
  }

  if (consume("PEA")) {
    ty.prim = Ptr;
    ty.ptr = alloc();
    read_var_type(*ty.ptr);
    return;
  }

  if (consume("PEB")) {
    ty.prim = Ptr;
    ty.ptr = alloc();
    read_var_type(*ty.ptr);
    ty.ptr->sclass = Const;
    return;
  }

  if (consume("QEB")) {
    ty.prim = Ptr;
    ty.sclass = Const;
    ty.ptr = alloc();
    read_var_type(*ty.ptr);
    ty.ptr->sclass = Const;
    return;
  }

  if (consume("Y")) {
    int dimension = read_number();
    if (dimension <= 0) {
      error = "invalid array dimension: " + std::to_string(dimension);
      return;
    }

    Type *tp = &ty;
    for (int i = 0; i < dimension; ++i) {
      tp->prim = Array;
      tp->len = read_number();
      tp->ptr = alloc();
      tp = tp->ptr;
    }

    if (consume("$$C")) {
      if (consume("B"))
        ty.sclass = Const;
      else if (consume("C") || consume("D"))
        ty.sclass = Const | Volatile;
      else if (!consume("A"))
        error = "unkonwn storage class: " + input.str();
    }

    read_var_type(*tp);
    return;
  }

  if (consume("P6A")) {
    ty.prim = Ptr;
    ty.ptr = alloc();

    Type &fn = *ty.ptr;
    fn.prim = Function;
    fn.ptr = alloc();
    read_var_type(*fn.ptr);

    while (error.empty() && !consume("@Z") && !consume("Z")) {
      Type *tp = alloc();
      read_var_type(*tp);
      fn.params.push_back(tp);
    }
    return;
  }

  ty.prim = read_prim_type();
}

// Reads a primitive type.
PrimTy Demangler::read_prim_type() {
  if (consume("X"))
    return Void;
  if (consume("_N"))
    return Bool;
  if (consume("D"))
    return Char;
  if (consume("C"))
    return Schar;
  if (consume("E"))
    return Uchar;
  if (consume("F"))
    return Short;
  if (consume("int"))
    return Ushort;
  if (consume("H"))
    return Int;
  if (consume("I"))
    return Uint;
  if (consume("J"))
    return Long;
  if (consume("int"))
    return Ulong;
  if (consume("_J"))
    return Llong;
  if (consume("_K"))
    return Ullong;
  if (consume("_W"))
    return Wchar;
  if (consume("M"))
    return Float;
  if (consume("N"))
    return Double;
  if (consume("ldouble"))
    return Ldouble;
  if (consume("T__m64@@"))
    return M64;
  if (consume("T__m128@@"))
    return M128;
  if (consume("U__m128d@@"))
    return M128d;
  if (consume("T__m128i@@"))
    return M128i;
  if (consume("T__m256@@"))
    return M256;
  if (consume("U__m256d@@"))
    return M256d;
  if (consume("T__m256i@@"))
    return M256i;
  if (consume("T__m512@@"))
    return M512;
  if (consume("U__m512d@@"))
    return M512d;
  if (consume("T__m512i@@"))
    return M512i;
  if (consume("Z"))
    return Varargs;

  error = "unknown primitive type: " + input.str();
  return Unknown;
}

// Converts an AST to a string.
//
// Converting an AST representing a C++ type to a string is tricky due
// to the bad grammar of the C++ declaration inherited from C. You have
// to construct a string from inside to outside. For example, if a type
// X is a pointer to a function returning int, the order you create a
// string becomes something like this:
//
//   (1) X is a pointer: *X
//   (2) (1) is a function returning int: int (*X)()
//
// So you cannot construct a result just by appending strings to a result.
//
// To deal with this, we split the function into two. write_pre() writes
// the "first half" of type declaration, and write_post() writes the
// "second half". For example, write_pre() writes a return type for a
// function and write_post() writes an parameter list.
std::string Demangler::str() {
  write_pre(type);
  write_name(symbol);
  write_post(type);
  return os.str();
}

// Write the "first half" of a given type.
void Demangler::write_pre(Type &type) {
  switch (type.prim) {
  case Unknown:
  case None:
    break;
  case Function:
    write_pre(*type.ptr);
    return;
  case Ptr:
    write_pre(*type.ptr);
    if (type.ptr->prim == Function || type.ptr->prim == Array)
      os << "(";
    os << "*";
    break;
  case Array:
    write_pre(*type.ptr);
    break;
  case Struct:
    os << "struct " << type.name;
    break;
  case Union:
    os << "union " << type.name;
    break;
  case Class:
    os << "class " << type.name;
    if (!type.params.empty()) {
      os << "<";
      write_params(type);
      os << ">";
    }
    break;
  case Enum:
    os << "enum ";
    write_name(type.name);
    break;
  case Void:
    os << "void";
    break;
  case Bool:
    os << "bool";
    break;
  case Char:
    os << "char";
    break;
  case Schar:
    os << "signed char";
    break;
  case Uchar:
    os << "unsigned char";
    break;
  case Short:
    os << "short";
    break;
  case Ushort:
    os << "unsigned short";
    break;
  case Int:
    os << "int";
    break;
  case Uint:
    os << "unsigned int";
    break;
  case Long:
    os << "long";
    break;
  case Ulong:
    os << "unsigned long";
    break;
  case Llong:
    os << "long long";
    break;
  case Ullong:
    os << "unsigned long long";
    break;
  case Wchar:
    os << "wchar_t";
    break;
  case Float:
    os << "float";
    break;
  case Double:
    os << "double";
    break;
  case Ldouble:
    os << "long double";
    break;
  case M64:
    os << "__m64";
    break;
  case M128:
    os << "__m128";
    break;
  case M128d:
    os << "__m128d";
    break;
  case M128i:
    os << "__m128i";
    break;
  case M256:
    os << "__m256";
    break;
  case M256d:
    os << "__m256d";
    break;
  case M256i:
    os << "__m256i";
    break;
  case M512:
    os << "__m512";
    break;
  case M512d:
    os << "__m512d";
    break;
  case M512i:
    os << "__m512i";
    break;
  case Varargs:
    os << "...";
    break;
  }

  if (type.sclass & Const) {
    write_space();
    os << "const";
  }
}

// Write the "second half" of a given type.
void Demangler::write_post(Type &type) {
  if (type.prim == Function) {
    os << "(";
    write_params(type);
    os << ")";
    return;
  }

  if (type.prim == Ptr) {
    if (type.ptr->prim == Function || type.ptr->prim == Array)
      os << ")";
    write_post(*type.ptr);
    return;
  }

  if (type.prim == Array) {
    os << "[" << type.len << "]";
    write_post(*type.ptr);
  }
}

// Write a function or template parameter list.
void Demangler::write_params(Type &type) {
  for (size_t i = 0; i < type.params.size(); ++i) {
    if (i != 0)
      os << ",";
    write_pre(*type.params[i]);
    write_post(*type.params[i]);
  }
}

// Write a symbo name. C@B@A is converted to A::B::C.
void Demangler::write_name(String s) {
  write_space();

  size_t pos = s.len;
  bool sep = false;

  for (ssize_t i = pos - 1; i >= 0; --i) {
    if (s.p[i] != '@')
      continue;

    if (sep)
      os << "::";
    sep = true;
    os.write(s.p + i + 1, pos - i - 1);
    pos = i;
  }

  if (sep)
    os << "::";

  String tok = s.substr(0, pos);

  // A special name for the ctor.
  if (tok.startswith("?0")) {
    tok.trim(2);
    os << tok << "::" << tok;
    return;
  }

  // A special name for the dtor.
  if (tok.startswith("?1")) {
    tok.trim(2);
    os << tok << "::~" << tok;
    return;
  }

  os << tok;
}

// Writes a space if the last token does not end with a punctuation.
void Demangler::write_space() {
  std::string s = os.str(); // this is probably very slow, but OK for now
  if (!s.empty() && isalpha(s.back()))
    os << " ";
}

std::string llvm::microsoftDemangle(const std::string &mangled_name,
                                    std::string error) {
  Demangler d(mangled_name);
  d.parse();
  if (!d.error.empty()) {
    error = d.error;
    return "";
  }
  return d.str();
}
