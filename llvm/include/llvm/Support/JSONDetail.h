//===--- JSONDetail.h - implementation details for JSON.h -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/PointerEmbeddedInt.h"
#include "llvm/ADT/PointerSumType.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/Support/TrailingObjects.h"

namespace llvm {
namespace json {
class Object;
class Array;
class Value;
namespace detail {

// Memory inside a JSON document is managed by an Arena (BumpPtrAllocator).
// Array uses std::vector, so we adapt LLVM's allocator concept to the STL's.
constexpr size_t FirstSlabSize = 1024;
using Arena = BumpPtrAllocatorImpl<MallocAllocator, FirstSlabSize>;
template <typename T> struct STLArena {
  using value_type = T;
  T *allocate(size_t N) {
    return static_cast<T *>(A.Allocate(N * sizeof(T), alignof(T)));
  }
  void deallocate(T *P, size_t N) { A.Deallocate(P, N * sizeof(T)); }
  operator Arena &() { return A; }
  Arena &A;
};

// The Object and Array types are used in a the Value::Rep PointerSumType.
// This requires them to be complete, but they have Values as indirect members!
// We get around this circularity by explicitly specifying that they are at
// least as aligned as an int64. (Both are large objects of at least 24 bytes).
} // namespace detail
} // namespace json
template <> struct PointerLikeTypeTraits<json::Object *> {
  static inline void *getAsVoidPointer(json::Object *P) { return P; }
  static inline json::Object *getFromVoidPointer(void *P) {
    return static_cast<json::Object *>(P);
  }
  enum {
    NumLowBitsAvailable = PointerLikeTypeTraits<int64_t *>::NumLowBitsAvailable
  };
};
template <> struct PointerLikeTypeTraits<json::Array *> {
  static inline void *getAsVoidPointer(json::Array *P) { return P; }
  static inline json::Array *getFromVoidPointer(void *P) {
    return static_cast<json::Array *>(P);
  }
  enum {
    NumLowBitsAvailable = PointerLikeTypeTraits<int64_t *>::NumLowBitsAvailable
  };
};
namespace json {
namespace detail {

// PString is the internal representation of a String value.
// These are immutable, so we just allocate the right number of bytes.
struct PString final : public TrailingObjects<PString, char> {
  size_t Size;
  const char *data() const { return getTrailingObjects<char>(); }

  operator StringRef() const { return StringRef(data(), Size); }
  bool operator==(const PString &PS) const {
    return Size == PS.Size && !memcmp(data(), PS.data(), Size);
  }
  static PString *create(StringRef S, Arena &Alloc) {
    PString *PS = static_cast<PString *>(
        Alloc.Allocate(totalSizeToAlloc<char>(S.size()), alignof(PString)));
    PS->Size = S.size();
    memcpy(const_cast<char *>(PS->data()), S.data(), S.size());
    return PS;
  }
};

// ValueRep is the lightweight representation of an arbitrary JSON value.
// It is a tagged pointer: [ pointer to representation | type ].
// The representation is stored in a BumpPtrAllocator in a type-specific way.
// true/false/null/integers have no extra data outside the ValueType.
//
// ValueRep has no ownership semantics, like a raw pointer.
// Value wraps ValueRep and owns the external data's lifetime (but not storage).
// ValueWrapper wraps Value and adds movability for use in internal containers.
enum ValueType {
  VT_Sentinel = 0,
  VT_SmallInt,
  VT_Double,
  VT_Array,
  VT_Object,
  VT_OwnedString,
  VT_StaticString,
};
enum SentinelValue : char {
  SV_Null = 0,
  SV_True = 1,
  SV_False = 2,
};
// We pack integers into the pointer, up to 53 bits. (More than that would
// result in inconsistent rounding as they're not representible as doubles).
// Of course on 32-bit platforms, we have fewer spare bits.
constexpr size_t SmallIntSize = 53 <= CHAR_BIT * sizeof(uintptr_t) - 3
                                    ? 53
                                    : CHAR_BIT * sizeof(uintptr_t) - 3;
using SmallInt = PointerEmbeddedInt<int64_t, SmallIntSize>;
template <typename T> bool isSmallInt(T V) {
  int64_t I = static_cast<int64_t>(V);
  return V == I && isInt<SmallIntSize>(I);
}
using ValueRep = PointerSumType<
    ValueType,
    PointerSumTypeMember<VT_Sentinel, PointerEmbeddedInt<SentinelValue>>,
    PointerSumTypeMember<VT_SmallInt, SmallInt>,
    PointerSumTypeMember<VT_Double, double *>,
    PointerSumTypeMember<VT_Array, Array *>,
    PointerSumTypeMember<VT_Object, Object *>,
    PointerSumTypeMember<VT_OwnedString, PString *>,
    PointerSumTypeMember<VT_StaticString, const char **>>;
const ValueRep TrueRep = ValueRep::create<VT_Sentinel>(SV_True);
const ValueRep FalseRep = ValueRep::create<VT_Sentinel>(SV_False);
const ValueRep NullRep = ValueRep::create<VT_Sentinel>(SV_Null);

class ValueWrapper;
ValueRep deepCopy(const ValueRep &U, Arena &Alloc);

class Literal {
public:
  Literal(const Literal &);
  Literal(Literal &&);
  ~Literal() { reset(); }

  // Strings: std::string, SmallString, StringRef, Twine, char*.
  // Support stringy types that Twine accepts, but not char, int etc.
  Literal(const Twine &V) : Type(LT_Twine) { new (&R.Twine) Twine(V); }
  Literal(const StringRef &V) : Type(LT_Twine) { new (&R.Twine) Twine(V); }
  Literal(const SmallVectorImpl<char> &V) : Type(LT_Twine) {
    new (&R.Twine) Twine(V);
  }
  Literal(const std::string &V) : Type(LT_Twine) { new (&R.Twine) Twine(V); }
  Literal(const formatv_object_base &V) : Type(LT_Twine) {
    new (&R.Twine) Twine(V);
  }
  // String literals are tracked separately to avoid copying them later.
  template <int N> Literal(const char (&V)[N]) : Type(LT_StaticString) {
    assert(strlen(V) == N - 1 &&
           "Detected string literal but not null-terminated!");
    R.StaticString = V;
  }
  // Don't treat const char* as a string literal unless it matched above.
  // Use a template to reduce overload match strength of string literals.
  template <typename T,
            typename = typename std::enable_if<
                std::is_same<T, const char *>::value>::type,
            int = 0>
  Literal(T V) : Type(LT_Twine) {
    new (&R.Twine) Twine(V);
  }
  // Mutable char arrays are also not string literals.
  template <int N> Literal(char (&V)[N]) : Literal((const char *)V) {}

  // Booleans: bool.
  // The default conversion rules will convert things to bool way too easily.
  template <
      typename T,
      typename = typename std::enable_if<std::is_same<T, bool>::value>::type,
      char Dummy = 0>
  Literal(T V) : Type(LT_Immediate) {
    R.Immediate = V ? TrueRep : FalseRep;
  }

  // Numbers: built-in numeric types.
  // is_arithmetic<T> is almost the right criterion, but exclude bool.
  template <
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type,
      typename = typename std::enable_if<std::integral_constant<
          bool, !std::is_same<T, bool>::value>::value>::type>
  Literal(T V) {
    if (isSmallInt(V)) {
      Type = LT_Immediate;
      R.Immediate = ValueRep::create<VT_SmallInt>(V);
    } else {
      Type = LT_Double;
      R.Double = V;
    }
  }

  // Null: nullptr;
  Literal(std::nullptr_t) : Type(LT_Immediate) { R.Immediate = NullRep; }

  // Objects: initializer_list<Literal> where each element is a two-element
  //          array and the first element is a string.
  // Arrays: initializer_list<Literal>.
  //
  // Where ambiguous, we choose Object unless ForceArray is passed.
  Literal(std::initializer_list<Literal> V, bool ForceArray = false);

  // References to JSON values: Value&.
  // The value must outlive this Literal.
  Literal(const Value &V) : Type(LT_Value) { R.Value = &V; }

  // Materializes this JSON value, allocating into the arena.
  ValueWrapper construct(Arena &) const;

private:
  static bool IsObjectCompatible(std::initializer_list<Literal> V);
  void reset();

  enum LiteralType {
    LT_Twine,
    LT_StaticString,
    LT_Array,
    LT_Object,
    LT_Double,
    LT_Immediate,
    LT_Value,
  } Type;
  union Rep {
    Rep() {}
    ~Rep() {}
    Twine Twine;
    const char *StaticString;
    std::vector<Literal> Array;
    StringMap<Literal> Object;
    double Double;
    ValueRep Immediate;
    const Value *Value;
  };
  Rep R;
};

} // namespace detail
} // namespace json
} // namespace llvm
