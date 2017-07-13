//===- Format.h - Efficient printf-style formatting for streams -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the format() function, which can be used with other
// LLVM subsystems to provide printf-style formatting.  This gives all the power
// and risk of printf.  This can be used like this (with raw_ostreams as an
// example):
//
//    OS << "mynumber: " << format("%4.5f", 1234.412) << '\n';
//
// Or if you prefer:
//
//  OS << format("mynumber: %4.5f\n", 1234.412);
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_FORMAT_H
#define LLVM_SUPPORT_FORMAT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/DataTypes.h"
#include <cassert>
#include <cstdio>
#include <tuple>

namespace llvm {

/// This is a helper class used for handling formatted output.  It is the
/// abstract base class of a templated derived class.
class format_object_base {
protected:
  const char *Fmt;
  ~format_object_base() = default; // Disallow polymorphic deletion.
  format_object_base(const format_object_base &) = default;
  virtual void home(); // Out of line virtual method.

  /// Call snprintf() for this object, on the given buffer and size.
  virtual int snprint(char *Buffer, unsigned BufferSize) const = 0;

public:
  format_object_base(const char *fmt) : Fmt(fmt) {}

  /// Format the object into the specified buffer.  On success, this returns
  /// the length of the formatted string.  If the buffer is too small, this
  /// returns a length to retry with, which will be larger than BufferSize.
  unsigned print(char *Buffer, unsigned BufferSize) const {
    assert(BufferSize && "Invalid buffer size!");

    // Print the string, leaving room for the terminating null.
    int N = snprint(Buffer, BufferSize);

    // VC++ and old GlibC return negative on overflow, just double the size.
    if (N < 0)
      return BufferSize * 2;

    // Other implementations yield number of bytes needed, not including the
    // final '\0'.
    if (unsigned(N) >= BufferSize)
      return N + 1;

    // Otherwise N is the length of output (not including the final '\0').
    return N;
  }
};

/// These are templated helper classes used by the format function that
/// capture the object to be formatted and the format string. When actually
/// printed, this synthesizes the string into a temporary buffer provided and
/// returns whether or not it is big enough.

// Helper to validate that format() parameters are scalars or pointers.
template <typename... Args> struct validate_format_parameters;
template <typename Arg, typename... Args>
struct validate_format_parameters<Arg, Args...> {
  static_assert(std::is_scalar<Arg>::value,
                "format can't be used with non fundamental / non pointer type");
  validate_format_parameters() { validate_format_parameters<Args...>(); }
};
template <> struct validate_format_parameters<> {};

template <typename... Ts>
class format_object final : public format_object_base {
  std::tuple<Ts...> Vals;

  template <std::size_t... Is>
  int snprint_tuple(char *Buffer, unsigned BufferSize,
                    index_sequence<Is...>) const {
#ifdef _MSC_VER
    return _snprintf(Buffer, BufferSize, Fmt, std::get<Is>(Vals)...);
#else
    return snprintf(Buffer, BufferSize, Fmt, std::get<Is>(Vals)...);
#endif
  }

public:
  format_object(const char *fmt, const Ts &... vals)
      : format_object_base(fmt), Vals(vals...) {
    validate_format_parameters<Ts...>();
  }

  int snprint(char *Buffer, unsigned BufferSize) const override {
    return snprint_tuple(Buffer, BufferSize, index_sequence_for<Ts...>());
  }
};

/// These are helper functions used to produce formatted output.  They use
/// template type deduction to construct the appropriate instance of the
/// format_object class to simplify their construction.
///
/// This is typically used like:
/// \code
///   OS << format("%0.4f", myfloat) << '\n';
/// \endcode

template <typename... Ts>
inline format_object<Ts...> format(const char *Fmt, const Ts &... Vals) {
  return format_object<Ts...>(Fmt, Vals...);
}

/// This is a helper class for left_justify, right_justify, and center_justify.
class FormattedString {
public:
  enum Justification { JustifyNone, JustifyLeft, JustifyRight, JustifyCenter };
  FormattedString(StringRef S, unsigned W, Justification J)
      : Str(S), Width(W), Justify(J) {}

private:
  StringRef Str;
  unsigned Width;
  Justification Justify;
  friend class raw_ostream;
};

/// left_justify - append spaces after string so total output is
/// \p Width characters.  If \p Str is larger that \p Width, full string
/// is written with no padding.
inline FormattedString left_justify(StringRef Str, unsigned Width) {
  return FormattedString(Str, Width, FormattedString::JustifyLeft);
}

/// right_justify - add spaces before string so total output is
/// \p Width characters.  If \p Str is larger that \p Width, full string
/// is written with no padding.
inline FormattedString right_justify(StringRef Str, unsigned Width) {
  return FormattedString(Str, Width, FormattedString::JustifyRight);
}

/// center_justify - add spaces before and after string so total output is
/// \p Width characters.  If \p Str is larger that \p Width, full string
/// is written with no padding.
inline FormattedString center_justify(StringRef Str, unsigned Width) {
  return FormattedString(Str, Width, FormattedString::JustifyCenter);
}

/// This is a helper class used for format_hex() and format_decimal().
class FormattedNumber {
  uint64_t HexValue;
  int64_t DecValue;
  unsigned Width;
  bool Hex;
  bool Upper;
  bool HexPrefix;
  friend class raw_ostream;

public:
  FormattedNumber(uint64_t HV, int64_t DV, unsigned W, bool H, bool U,
                  bool Prefix)
      : HexValue(HV), DecValue(DV), Width(W), Hex(H), Upper(U),
        HexPrefix(Prefix) {}
};

/// format_hex - Output \p N as a fixed width hexadecimal. If number will not
/// fit in width, full number is still printed.  Examples:
///   OS << format_hex(255, 4)              => 0xff
///   OS << format_hex(255, 4, true)        => 0xFF
///   OS << format_hex(255, 6)              => 0x00ff
///   OS << format_hex(255, 2)              => 0xff
inline FormattedNumber format_hex(uint64_t N, unsigned Width,
                                  bool Upper = false) {
  assert(Width <= 18 && "hex width must be <= 18");
  return FormattedNumber(N, 0, Width, true, Upper, true);
}

/// format_hex_no_prefix - Output \p N as a fixed width hexadecimal. Does not
/// prepend '0x' to the outputted string.  If number will not fit in width,
/// full number is still printed.  Examples:
///   OS << format_hex_no_prefix(255, 2)              => ff
///   OS << format_hex_no_prefix(255, 2, true)        => FF
///   OS << format_hex_no_prefix(255, 4)              => 00ff
///   OS << format_hex_no_prefix(255, 1)              => ff
inline FormattedNumber format_hex_no_prefix(uint64_t N, unsigned Width,
                                            bool Upper = false) {
  assert(Width <= 16 && "hex width must be <= 16");
  return FormattedNumber(N, 0, Width, true, Upper, false);
}

/// format_decimal - Output \p N as a right justified, fixed-width decimal. If
/// number will not fit in width, full number is still printed.  Examples:
///   OS << format_decimal(0, 5)     => "    0"
///   OS << format_decimal(255, 5)   => "  255"
///   OS << format_decimal(-1, 3)    => " -1"
///   OS << format_decimal(12345, 3) => "12345"
inline FormattedNumber format_decimal(int64_t N, unsigned Width) {
  return FormattedNumber(0, N, Width, false, false, false);
}

class FormattedBytes {
  ArrayRef<uint8_t> Bytes;

  // If not None, display offsets for each line relative to starting value.
  Optional<uint64_t> FirstByteOffset;
  uint32_t IndentLevel;  // Number of characters to indent each line.
  uint32_t NumPerLine;   // Number of bytes to show per line.
  uint8_t ByteGroupSize; // How many hex bytes are grouped without spaces
  bool Upper;            // Show offset and hex bytes as upper case.
  bool ASCII;            // Show the ASCII bytes for the hex bytes to the right.
  friend class raw_ostream;

public:
  FormattedBytes(ArrayRef<uint8_t> B, uint32_t IL, Optional<uint64_t> O,
                 uint32_t NPL, uint8_t BGS, bool U, bool A)
      : Bytes(B), FirstByteOffset(O), IndentLevel(IL), NumPerLine(NPL),
        ByteGroupSize(BGS), Upper(U), ASCII(A) {

    if (ByteGroupSize > NumPerLine)
      ByteGroupSize = NumPerLine;
  }
};

inline FormattedBytes
format_bytes(ArrayRef<uint8_t> Bytes, Optional<uint64_t> FirstByteOffset = None,
             uint32_t NumPerLine = 16, uint8_t ByteGroupSize = 4,
             uint32_t IndentLevel = 0, bool Upper = false) {
  return FormattedBytes(Bytes, IndentLevel, FirstByteOffset, NumPerLine,
                        ByteGroupSize, Upper, false);
}

inline FormattedBytes
format_bytes_with_ascii(ArrayRef<uint8_t> Bytes,
                        Optional<uint64_t> FirstByteOffset = None,
                        uint32_t NumPerLine = 16, uint8_t ByteGroupSize = 4,
                        uint32_t IndentLevel = 0, bool Upper = false) {
  return FormattedBytes(Bytes, IndentLevel, FirstByteOffset, NumPerLine,
                        ByteGroupSize, Upper, true);
}

/// This is a helper class used for format_memory.
class FormattedMemorySize {
public:
  enum Format { Binary, Decimal, Customary = -1 };
  FormattedMemorySize(double NB, unsigned W = 2, Format S = Binary)
      : NumBytes(NB), Width(W), Suffix(S) {}

private:
  friend class raw_ostream;
  const double NumBytes;
  const unsigned Width;
  const Format Suffix;
};

/// format_memory_size - Output \p NB as human readable amount of bytes with
/// IEC suffixes. When the amount is greater than half the amount for the next
/// suffix that next suffix is used.  If no suffix exists for the amount of
/// bytes: the number is printed as bytes.
///
///   OS << format_memory_size(1024, 2)         => "1.00 KiB"
///   OS << format_memory_size(524288, 2)       => "0.50 MiB"
///   OS << format_memory_size(1048576, 4)      => "1.0000 MiB"
///   OS << format_memory_size(pow(1024, 5), 3) => "1.000 PiB"
inline FormattedMemorySize format_memory_size(double NB, unsigned W = 2) {
  return FormattedMemorySize(NB, W);
}

/// format_memory_size_si - Output \p NB as human readable amount of bytes with
/// SI suffixes. When the amount is greater than half the amount for the next
/// suffix that next suffix is used.  If no suffix exists for the amount of
/// bytes: the number is printed as bytes.
///
///   OS << format_memory_size(1000, 2)         => "1.00 kB"
///   OS << format_memory_size(500000, 2)       => "0.50 MB"
///   OS << format_memory_size(1000000, 4)      => "1.0000 MB"
///   OS << format_memory_size(pow(1000, 5), 3) => "1.000 PB"
inline FormattedMemorySize format_memory_size_si(double NB, unsigned W = 2) {
  return FormattedMemorySize(NB, W, FormattedMemorySize::Decimal);
}

/// format_memory_size_customary - Output \p NB as human readable amount of
/// bytes with SI suffixes, but still as power of 2. Techincally incorrect but
/// still often used for RAM, older unix tools, and operating systems.
///
///   OS << format_memory_size_customary(1024, 2)   => "1.00 kB"
///   OS << format_memory_size_customary(524288, 2) => "0.50 MB"
inline FormattedMemorySize format_memory_size_customary(double NB,
                                                        unsigned W = 2) {
  return FormattedMemorySize(NB, W, FormattedMemorySize::Customary);
}

} // end namespace llvm

#endif
