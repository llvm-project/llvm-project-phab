//===--- JSON.cpp - JSON representation, parser, serializer -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/JSON.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/NativeFormatting.h"

namespace llvm {
namespace json {
using namespace llvm::json::detail;

ValueRep detail::deepCopy(const ValueRep &U, Arena &Alloc) {
  switch (U.getTag()) {
  case VT_Sentinel:
  case VT_SmallInt:
    return U;
  case VT_OwnedString:
    return ValueRep::create<VT_OwnedString>(
        PString::create(*U.cast<VT_OwnedString>(), Alloc));
  case VT_StaticString:
    return ValueRep::create<VT_StaticString>(
        new (Alloc) const char *(*U.cast<VT_StaticString>()));
  case VT_Double:
    return ValueRep::create<VT_Double>(
        new (Alloc) double{*U.cast<VT_Double>()});
  case VT_Array: {
    Array *Old = U.cast<VT_Array>();
    Array *A = new (Alloc) Array(Old->size(), Alloc);
    for (const Value &E : *Old)
      A->Elems.emplace_back(deepCopy(ValueRep(E), Alloc));
    return ValueRep::create<VT_Array>(A);
  }
  case VT_Object: {
    Object *Old = U.cast<VT_Object>();
    Object *O = new (Alloc) Object(Old->size(), Alloc);
    for (const auto &E : *Old)
      // TODO: remove friends with more naive code if it optimizes OK.
      O->Props.try_emplace(E.first, deepCopy(ValueRep(E.second), Alloc));
    return ValueRep::create<VT_Object>(O);
  }
  }
}

ValueWrapper Literal::construct(Arena &Alloc) const {
  switch (Type) {
  case LT_Twine: {
    return ValueWrapper(ValueRep::create<VT_OwnedString>(PString::create(
        R.Twine.isSingleStringRef() ? R.Twine.getSingleStringRef()
                                    : StringRef(R.Twine.str()),
        Alloc)));
  }
  case LT_String:
    return ValueWrapper(
        ValueRep::create<VT_OwnedString>(PString::create(R.String, Alloc)));
  case LT_StaticString:
    return ValueWrapper(ValueRep::create<VT_StaticString>(
        new (Alloc) const char *(R.StaticString)));
  case LT_Array: {
    Array *A = new (Alloc) Array(R.Array.size(), Alloc);
    for (const Literal &E : R.Array)
      A->push_back(E);
    return ValueWrapper(ValueRep::create<VT_Array>(A));
  }
  case LT_Object: {
    Object *O = new (Alloc) Object(R.Object.size(), Alloc);
    for (const auto &E : R.Object)
      O->set(E.first(), E.second);
    return ValueWrapper(ValueRep::create<VT_Object>(O));
  }
  case LT_Double:
    return ValueWrapper(
        ValueRep::create<VT_Double>(new (Alloc) double{R.Double}));
  case LT_Immediate:
    return ValueWrapper(R.Immediate);
  case LT_Value: // Deep copy.
    return ValueWrapper(deepCopy(ValueRep(*R.Value), Alloc));
  case LT_Document: // Deep copy.
    return ValueWrapper(deepCopy(ValueRep(R.Document->root()), Alloc));
  }
}

Value Value::True(TrueRep);
Value Value::False(FalseRep);
Value Value::Null(NullRep);

Value::~Value() {
  static_assert(alignof(Array) >= alignof(int64_t), "");
  static_assert(alignof(Object) >= alignof(int64_t), "");

  switch (U.getTag()) {
  case VT_Array:
    U.cast<VT_Array>()->~Array();
    break;
  case VT_Object:
    U.cast<VT_Object>()->~Object();
    break;
  case VT_StaticString:
  case VT_OwnedString:
  case VT_Double:
  case VT_SmallInt:
  case VT_Sentinel:
    // POD, nothing to do.
    break;
  }
}

bool operator==(const Value &L, const Value &R) {
  if (L.U == R.U)
    return true;
  Kind K = L.kind();
  if (K != R.kind())
    return false;
  switch (K) {
  case StringKind:
    return *L.string() == *R.string();
  case ArrayKind:
    return *L.U.cast<VT_Array>() == *R.U.cast<VT_Array>();
  case ObjectKind:
    return *L.U.cast<VT_Object>() == *R.U.cast<VT_Object>();
  case NumberKind:
    return *L.number() == *R.number();
  case BooleanKind:
    return false; // rep equality checked above.
  case NullKind:
    llvm_unreachable("NullKinds with distinct ValueRep");
  }
}

bool operator<(const Value &L, const Value &R) {
  Kind LK = L.kind(), RK = R.kind();
  if (LK != RK)
    return LK < RK;
  switch (LK) {
  case StringKind:
    return *L.string() < *R.string();
  case ArrayKind:
    return *L.array() < *R.array();
  case ObjectKind:
    return *L.object() < *R.object();
  case NumberKind:
    return *L.number() < *R.number();
  case BooleanKind:
    return *L.boolean() < *R.boolean();
  case NullKind:
    return false;
  }
}

hash_code hash_value(const Value &V) {
  switch (Kind K = V.kind()) {
  case StringKind:
    return hash_combine(K, *V.string());
  case ArrayKind:
    return hash_combine(K, *V.array());
  case ObjectKind:
    return hash_combine(K, *V.object());
  case NumberKind: {
    double D = *V.number(); // doubles are not directly hashable.
    return hash_combine(K, *reinterpret_cast<uint64_t *>(&D));
  }
  case BooleanKind:
    return hash_combine(K, *V.boolean());
  case NullKind:
    return hash_combine(K);
  }
}

bool operator==(const Array &L, const Array &R) {
  if (L.size() != R.size())
    return false;
  for (size_t I = 0; I < L.size(); ++I)
    if (L[I] != R[I])
      return false;
  return true;
}

bool operator==(const Object &L, const Object &R) {
  if (L.size() != R.size())
    return false;
  for (const auto &E : L) {
    const Value *Other = R.get(E.first);
    if (!Other || E.second != *Other)
      return false;
  }
  return true;
}

hash_code hash_value(const Object &O) {
  size_t Hash = 0;
  for (const auto &E : O)
    Hash += hash_combine(E.first, E.second);
  return Hash; // Independent of iteration order.
}

Literal::Literal(LiteralList V) : Type(LT_Array) {
  new (&R.Array) std::vector<Literal>(std::make_move_iterator(V.begin()),
                                      std::make_move_iterator(V.end()));
}

Literal::Literal(Document V) : Type(LT_Document) {
  new (&R.Document) std::unique_ptr<Document>(new Document(std::move(V)));
}

Literal::Literal(const Literal &&Move) {
  switch (Move.Type) {
  case LT_Twine:
    new (&R.Twine) Twine(std::move(Move.R.Twine));
    Move.R.Twine.~Twine();
    break;
  case LT_String:
    new (&R.String) std::string(std::move(Move.R.String));
    Move.R.String.~basic_string();
    break;
  case LT_Object:
    new (&R.Object) StringMap<Literal>(std::move(Move.R.Object));
    Move.R.Object.~StringMap();
    break;
  case LT_Array:
    new (&R.Array) std::vector<Literal>(std::move(Move.R.Array));
    Move.R.Array.~vector();
    break;
  case LT_Document:
    new (&R.Document) std::unique_ptr<Document>(std::move(Move.R.Document));
    Move.R.Document.~unique_ptr();
    break;
  case LT_Immediate:
  case LT_StaticString:
  case LT_Double:
  case LT_Value:
    memcpy(&R, &Move.R, sizeof(ValueRep)); // POD
  }
  Type = Move.Type;
  Move.Type = LT_Immediate;
  Move.R.Immediate = NullRep;
}

Literal::~Literal() {
  switch (Type) {
  case LT_Twine:
    R.Twine.~Twine();
    break;
  case LT_String:
    R.String.~basic_string();
    break;
  case LT_Object:
    R.Object.~StringMap();
    break;
  case LT_Array:
    R.Array.~vector();
    break;
  case LT_Document:
    R.Document.~unique_ptr();
    break;
  case LT_Immediate:
  case LT_StaticString:
  case LT_Double:
  case LT_Value:
    break; // POD, nothing to do.
  }
}

static void quote(StringRef S, raw_ostream &OS) {
  OS << "\"";
  for (unsigned char C : S) {
    if (C == 0x22 || C == 0x5C)
      OS << '\\';
    if (C >= 0x20) {
      OS << C;
      continue;
    }
    OS << '\\';
    switch (C) {
    // Some control characters have shorter escape sequences than \uXXXX.
    case '\b':
      OS << 'b';
      break;
    case '\t':
      OS << 't';
      break;
    case '\n':
      OS << 'n';
      break;
    case '\f':
      OS << 'f';
      break;
    case '\r':
      OS << 'r';
      break;
    default:
      OS << 'u';
      llvm::write_hex(OS, C, HexPrintStyle::Lower, 4);
      break;
    }
  }
  OS << "\"";
}

raw_ostream &operator<<(raw_ostream &OS, const Array &V) {
  const char *Sep = "";
  OS << "[";
  for (const auto &E : V) {
    OS << Sep << E;
    Sep = ",";
  }
  OS << "]";
  return OS;
}

raw_ostream &operator<<(raw_ostream &OS, const Object &V) {
  const char *Sep = "";
  OS << "{";
  for (const auto &P : V) {
    OS << Sep;
    quote(P.first, OS);
    OS << ":" << P.second;
    Sep = ",";
  }
  OS << "}";
  return OS;
}

raw_ostream &operator<<(raw_ostream &OS, const Value &V) {
  switch (V.U.getTag()) {
  case VT_Array:
    OS << *V.U.cast<VT_Array>();
    break;
  case VT_Object:
    OS << *V.U.cast<VT_Object>();
    break;
  case VT_OwnedString:
    quote(*V.U.cast<VT_OwnedString>(), OS);
    break;
  case VT_StaticString:
    quote(*V.U.cast<VT_StaticString>(), OS);
    break;
  case VT_Double:
    OS << format("%g", *V.U.cast<VT_Double>());
    break;
  case VT_SmallInt:
    OS << V.U.cast<VT_SmallInt>();
    break;
  case VT_Sentinel:
    switch (V.U.cast<VT_Sentinel>()) {
    case SV_Null:
      OS << "null";
      break;
    case SV_True:
      OS << "true";
      break;
    case SV_False:
      OS << "false";
      break;
    }
    break;
  }
  return OS;
}

class Document::Parser {
public:
  Parser(StringRef Text)
      : Start(Text.data()), P(Text.data()), End(Text.data() + Text.size()),
        Alloc(new Arena()) {}

  Expected<Document> parseDocument() {
    auto Root = parseObject();
    if (!Root)
      return std::move(*Err);
    eatWhitespace();
    if (P != End) {
      error("Text after end of document");
      return std::move(*Err);
    }
    if (Err)
      llvm_unreachable("Error should have been reported");
    return Document(ValueWrapper(*Root), std::move(Alloc));
  }

private:
  Optional<Error> Err;
  const char *Start, *P, *End;
  std::unique_ptr<Arena> Alloc;

  Optional<ValueRep> parseObject() {
    eatWhitespace();
    if (P == End)
      return error("Unexpected EOF");
    switch (char C = next()) {
    case 't': {
      if (!(consume('r') && consume('u') && consume('e')))
        return error("Invalid bareword");
      return TrueRep;
    }
    case 'f': {
      if (!(consume('a') && consume('l') && consume('s') && consume('e')))
        return error("Invalid bareword");
      return FalseRep;
    }
    case 'n': {
      if (!(consume('u') && consume('l') && consume('l')))
        return error("Invalid bareword");
      return NullRep;
    }
    case '"': {
      auto S = parseString();
      if (!S)
        return None;
      return ValueRep::create<VT_OwnedString>(PString::create(*S, *Alloc));
    }
    case '[': {
      Array *A = new (*Alloc) Array(*Alloc);
      eatWhitespace();
      if (peek() == ']') {
        ++P;
        return ValueRep::create<VT_Array>(A);
      }
      for (;;) {
        auto Elem = parseObject();
        if (!Elem)
          return None;
        A->Elems.emplace_back(*Elem);
        eatWhitespace();
        switch (next()) {
        case ',':
          eatWhitespace();
          continue;
        case ']':
          return ValueRep::create<VT_Array>(A);
        default:
          return error("Expected , or ] after array element");
        }
      }
    }
    case '{': {
      Object *O = new (*Alloc) Object(*Alloc);
      eatWhitespace();
      if (peek() == '}') {
        ++P;
        return ValueRep::create<VT_Object>(O);
      }
      for (;;) {
        if (next() != '"')
          return error("Expected object key");
        auto K = parseString();
        if (!K)
          return None;
        eatWhitespace();
        if (next() != ':')
          return error("Expected : after object key");
        eatWhitespace();
        auto V = parseObject();
        if (!V)
          return None;
        O->Props.try_emplace(*K, *V);
        eatWhitespace();
        switch (next()) {
        case ',':
          eatWhitespace();
          continue;
        case '}':
          return ValueRep::create<VT_Object>(O);
        default:
          return error("Expected , or } after object value");
        }
      }
    }
    default:
      if (isNumber(C)) {
        auto V = parseNumber(C);
        if (!V)
          return None;
        if (isSmallInt(*V))
          return ValueRep::create<VT_SmallInt>(*V);
        return ValueRep::create<VT_Double>(new (*Alloc) double{*V});
      }
      return error("Expected JSON value");
    }
  }

  Optional<SmallString<16>> parseString() {
    SmallString<16> Result;
    for (char C = next(); C != '"'; C = next()) {
      switch (C) {
      default:
        Result.push_back(C);
        break;
      case 0:
        return error("Unterminated string");
      case '\\':
        switch (C = next()) {
        case '"':
        case '\\':
        case '/':
          Result.push_back(C);
          break;
        case 'b':
          Result.push_back('\b');
          break;
        case 'f':
          Result.push_back('\f');
          break;
        case 'n':
          Result.push_back('\n');
          break;
        case 'r':
          Result.push_back('\r');
          break;
        case 't':
          Result.push_back('\t');
          break;
        case 'u':
          if (!parseUnicode(Result))
            return None;
          break;
        default:
          return error("Invalid escape sequence");
        }
      }
    }
    return Result;
  }

  // Parse a \uNNNN escape sequence, the \u have already been consumed.
  // May parse multiple escapes in some cases (surrogate pairs).
  bool parseUnicode(SmallVectorImpl<char> &Out) {
    auto Invalid = [&] { Out.append({'\xef', '\xbf', '\xbd'}); };
    auto Parse2 = [this](uint16_t &V) {
      V = 0;
      char Bytes[] = {next(), next(), next(), next()};
      for (unsigned char C : Bytes) {
        if (!std::isxdigit(C)) {
          error("invalid \\u escape sequence");
          return false;
        }
        V <<= 4;
        V |= (C > '9') ? (C & ~0x20) - 'A' + 10 : (C - '0');
      }
      return true;
    };
    uint16_t First;
    if (!Parse2(First))
      return false;

    // We loop to allow proper surrogate-pair error handling.
    while (true) {
      if (First < 0xD800 || First >= 0xE000) // BMP.
        return encodeUtf8(First, Out);

      if (First >= 0xDC00) {
        Invalid(); // Lone trailing surrogate.
        return true;
      }

      // We have a leading surrogate, and need a trailing one.
      // Don't advance P: a lone surrogate is valid JSON (but invalid unicode)
      if (P + 2 > End || *P != '\\' || *(P + 1) != 'u') {
        Invalid(); // Lone leading not followed by \u...
        return true;
      }
      P += 2;
      uint16_t Second;
      if (!Parse2(Second))
        return false;
      if (Second < 0xDC00 && Second >= 0xE000) {
        Invalid();  // Lone loading not followed by trailing.
        First = Second;       // Second escape still needs to be processed.
        continue;
      }

      // Valid surrogate pair.
      return encodeUtf8(0x10000 | ((First - 0xD800) << 10) | (Second - 0xDC00),
                        Out);
    }
  }

  bool encodeUtf8(uint32_t Rune, SmallVectorImpl<char> &Out) {
    // UTF-8 encode rune
    if (Rune <= 0x7F) {
      Out.push_back(Rune & 0x7F);
      return true;
    } else if (Rune <= 0x7FF) {
      uint8_t FirstByte = 0xC0 | ((Rune & 0x7C0) >> 6);
      uint8_t SecondByte = 0x80 | (Rune & 0x3F);
      Out.push_back(FirstByte);
      Out.push_back(SecondByte);
      return true;
    } else if (Rune <= 0xFFFF) {
      uint8_t FirstByte = 0xE0 | ((Rune & 0xF000) >> 12);
      uint8_t SecondByte = 0x80 | ((Rune & 0xFC0) >> 6);
      uint8_t ThirdByte = 0x80 | (Rune & 0x3F);
      Out.push_back(FirstByte);
      Out.push_back(SecondByte);
      Out.push_back(ThirdByte);
      return true;
    } else if (Rune <= 0x10FFFF) {
      uint8_t FirstByte = 0xF0 | ((Rune & 0x1F0000) >> 18);
      uint8_t SecondByte = 0x80 | ((Rune & 0x3F000) >> 12);
      uint8_t ThirdByte = 0x80 | ((Rune & 0xFC0) >> 6);
      uint8_t FourthByte = 0x80 | (Rune & 0x3F);
      Out.push_back(FirstByte);
      Out.push_back(SecondByte);
      Out.push_back(ThirdByte);
      Out.push_back(FourthByte);
      return true;
    } else {
      error("Invalid unicode codepoint");
      return false;
    }
  }

  bool isNumber(char C) {
    return C == '-' || C == '+' || C == '.' || C == 'e' || C == 'E' ||
           (C >= '0' && C <= '9');
  }

  llvm::Optional<double> parseNumber(char First) {
    SmallString<24> S;
    S.push_back(First);
    while (isNumber(peek()))
      S.push_back(next());
    char *End;
    double D = std::strtod(S.c_str(), &End);
    if (End != S.end())
      return error("Invalid number");
    return D;
  }

  void eatWhitespace() {
    while (P != End && (*P == ' ' || *P == '\r' || *P == '\n' || *P == '\t'))
      ++P;
  }
  bool consume(char C) { return P != End && *P++ == C; }
  char next() { return P == End ? 0 : *P++; }
  char peek() { return P == End ? 0 : *P; }

  NoneType error(StringRef Msg) {
    int Line = 1;
    const char *StartOfLine = Start;
    for (const char *X = Start; X < P; ++X) {
      if (*X == 0x0A) {
        ++Line;
        StartOfLine = X;
      }
    }
    // TODO: actual error type.
    Err = make_error<StringError>(formatv("[{0}:{1}, byte={2}]: {3}", Line,
                                          P - StartOfLine, P - Start, Msg),
                                  inconvertibleErrorCode());
    return None;
  }
};

Expected<Document> Document::parse(StringRef Text) {
  return Parser(Text).parseDocument();
}

} // namespace json
} // namespace llvm
