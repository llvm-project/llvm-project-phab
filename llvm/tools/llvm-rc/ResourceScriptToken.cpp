//===-- ResourceScriptToken.cpp ---------------------------------*- C++-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This file implements an interface defined in ResourceScriptToken.h.
// In particular, it defines an .rc script tokenizer.
//
//===---------------------------------------------------------------------===//

#include "ResourceScriptToken.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <utility>

using Kind = RCToken::Kind;

RCToken::RCToken(RCToken::Kind RCTokenKind, const llvm::StringRef &Value)
    : TokenKind(RCTokenKind), TokenValue(Value) {}

uint32_t RCToken::intValue() const {
  assert(TokenKind == Kind::Int);
  // We assume that the token already is a correct integer (checked by
  // IsCorrectInteger.) Thus, we don't care about std::stoull possibly not
  // reading the last character (lL); the return value will be correct.
  return std::stoull(TokenValue, nullptr, 0);
}

llvm::StringRef &RCToken::value() { return TokenValue; }

llvm::StringRef RCToken::value() const { return TokenValue; }

Kind RCToken::kind() const { return TokenKind; }

namespace {

// Checks if Representation is a correct description of an RC integer.
// It should be a 32-bit unsigned integer, either decimal, octal (0[0-7]+),
// or hexadecimal (0x[0-9a-f]+). It might be followed by a single 'L'
// character.
bool isCorrectInteger(llvm::StringRef Representation) {
  size_t Length = Representation.size();
  uint32_t Num;
  if (Length == 0)
    return false;
  // Strip the last 'L' if unnecessary.
  if (std::toupper(Representation.back()) == 'L')
    Representation = Representation.drop_back(1);

  return !Representation.getAsInteger<uint32_t>(0, Num);
}

class Tokenizer {
public:
  Tokenizer(llvm::StringRef Input) : Data(Input), DataLength(Input.size()) {}

  std::vector<RCToken> run(std::string &Error);

private:
  // All 'advancing' methods return boolean values; if they're equal to false,
  // the stream has ended or failed.
  bool advance(size_t Amount = 1);
  bool skipWhitespaces();
  bool consumeToken(const Kind TokenKind);

  // Check if tokenizer is about to read FollowingChars.
  bool willNowRead(const llvm::StringRef &FollowingChars) const;

  // Check if tokenizer can start reading an identifier at current position.
  // The original tool did non specify the rules to determine what is a correct
  // identifier. We assume they should follow the C convention:
  // [a-zA-z_][a-zA-Z0-9_]*.
  bool canStartIdentifier() const;
  // Check if tokenizer can continue reading an identifier.
  bool canContinueIdentifier() const;

  // Check if tokenizer can start reading an integer.
  // A correct integer always starts with a 0-9 digit,
  // can contain characters 0-9A-Fa-f (digits),
  // Ll (marking the integer is 32-bit), Xx (marking the representation
  // is hexadecimal). As some kind of separator should come after the
  // integer, we can consume the integer until a non-alphanumeric
  // character.
  bool canStartInt() const;
  bool canContinueInt() const;
  bool canStartString() const;

  size_t remainingChars() const;
  bool streamError() const;
  bool streamEof() const;
  bool streamDone() const;

  // Classify the token that is about to be read from the current position.
  Kind classifyCurrentToken() const;

  // Process the Kind::Identifier token - check if it is
  // an identifier describing a block start or end.
  void processIdentifier(RCToken &token) const;

  llvm::StringRef Data;
  std::string CurError;
  size_t DataLength, Pos;
};

std::vector<RCToken> Tokenizer::run(std::string &Error) {
  Pos = 0;
  std::vector<RCToken> Result;

  // Consume an optional UTF-8 Byte Order Mark.
  if (willNowRead("\xef\xbb\xbf"))
    advance(3);

  while (!streamDone()) {
    if (!skipWhitespaces())
      break;

    Kind TokenKind = classifyCurrentToken();
    if (TokenKind == Kind::Invalid) {
      CurError = "Invalid token found at position " + std::to_string(Pos);
      break;
    }

    const size_t TokenStart = Pos;
    if (!consumeToken(TokenKind))
      break;

    RCToken Token(TokenKind, Data.take_front(Pos).drop_front(TokenStart));
    if (TokenKind == Kind::Identifier) {
      processIdentifier(Token);
    } else if (TokenKind == Kind::Int && !isCorrectInteger(Token.value())) {
      // The integer has incorrect format or cannot be represented in
      // a 32-bit integer.
      CurError = "Integer invalid or too large: " + Token.value().str();
      break;
    }

    Result.push_back(Token);
  }

  // Process the errors.
  if (streamError()) {
    Error = CurError;
    Result.clear();
  }

  return Result;
}

bool Tokenizer::advance(size_t Amount) {
  Pos += Amount;
  return !streamDone();
}

bool Tokenizer::skipWhitespaces() {
  while (!streamEof() && std::isspace(Data[Pos]))
    advance();
  return !streamDone();
}

bool Tokenizer::consumeToken(const Kind TokenKind) {
  switch (TokenKind) {
  // One-character token consumption.
#define TOKEN(Name)
#define SHORT_TOKEN(Name, Ch) case Kind::Name:
#include "ResourceScriptTokenList.h"
#undef TOKEN
#undef SHORT_TOKEN
    advance();
    return true;

  case Kind::Identifier:
    while (!streamEof() && canContinueIdentifier())
      advance();
    return true;

  case Kind::Int:
    while (!streamEof() && canContinueInt())
      advance();
    return true;

  case Kind::String:
    // Consume the preceding 'L', if there is any.
    if (std::toupper(Data[Pos]) == 'L')
      advance();
    // Consume the double-quote.
    advance();

    // Consume the characters until the end of the file, line or string.
    while (true) {
      if (streamEof()) {
        CurError = "Unterminated string literal.";
        return false;
      } else if (Data[Pos] == '"') {
        // Consume the ending double-quote.
        advance();
        return true;
      } else if (Data[Pos] == '\n') {
        CurError = "String literal not terminated in the line.";
        return false;
      }

      advance();
    }

  case Kind::Invalid:
    assert(false && "Cannot consume an invalid token.");
  }
}

bool Tokenizer::willNowRead(const llvm::StringRef &FollowingChars) const {
  return Data.drop_front(Pos).startswith(FollowingChars);
}

size_t Tokenizer::remainingChars() const { return DataLength - Pos; }

bool Tokenizer::canStartIdentifier() const {
  assert(!streamEof());

  const char CurChar = Data[Pos];
  return std::isalpha(CurChar) || CurChar == '_';
}

bool Tokenizer::canContinueIdentifier() const {
  assert(!streamEof());
  const char CurChar = Data[Pos];
  return std::isalnum(CurChar) || CurChar == '_';
}

bool Tokenizer::canStartInt() const {
  assert(!streamEof());
  return std::isdigit(Data[Pos]);
}

bool Tokenizer::canContinueInt() const {
  assert(!streamEof());
  return std::isalnum(Data[Pos]);
}

bool Tokenizer::canStartString() const {
  return willNowRead("\"") || willNowRead("L\"") || willNowRead("l\"");
}

bool Tokenizer::streamError() const { return CurError.size() != 0; }

bool Tokenizer::streamEof() const { return Pos == DataLength; }

bool Tokenizer::streamDone() const { return streamEof() || streamError(); }

Kind Tokenizer::classifyCurrentToken() const {
  if (canStartInt())
    return Kind::Int;
  if (canStartString())
    return Kind::String;
  // BEGIN and END are at this point of lexing recognized as identifiers.
  if (canStartIdentifier())
    return Kind::Identifier;

  const char CurChar = Data[Pos];

  switch (CurChar) {
  // One-character token classification.
#define TOKEN(Name)
#define SHORT_TOKEN(Name, Ch)                                                  \
  case Ch:                                                                     \
    return Kind::Name;
#include "ResourceScriptTokenList.h"
#undef TOKEN
#undef SHORT_TOKEN

  default:
    return Kind::Invalid;
  }
}

void Tokenizer::processIdentifier(RCToken &Token) const {
  assert(Token.kind() == Kind::Identifier);
  llvm::StringRef Name = Token.value();

  if (Name.equals_lower("begin"))
    Token = RCToken(Kind::BlockBegin, Name);
  else if (Name.equals_lower("end"))
    Token = RCToken(Kind::BlockEnd, Name);
}

} // anonymous namespace

std::vector<RCToken> tokenize(llvm::StringRef Input, std::string &Error) {
  return Tokenizer(Input).run(Error);
}
