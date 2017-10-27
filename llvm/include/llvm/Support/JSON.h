//===--- JSON.h - JSON representation, parser, serializer -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file allows creating, mutating, and (de-)serializing JSON.
//
// A Document is an owned JSON tree, and is the arena owning all values.
// Values are JSON expressions owned by a Document. This is a union of:
//   StringRef, bool, double, nullptr_t: for primitive types
//   Array: similar to vector<Value>
//   Object: similar to unordered_map<string, Value>
// Arrays and Objects are mutable, inserted values are copied into the document.
//
// Literal allows you to use natural C++ expressions to construct JSON values.
//
// An example:
//   // Parse a JSON file and do "error checking".
//   Expected<Document> Doc = Document::parse("[1,true,3]");
//   assert(Doc && Doc->array());
//
//   Array& A = *Doc->array();
//   for (int I = 0; I < A.size(); ++I) {
//     if (auto N = A[I].number()) {
//       A.set(I, {{"number", *N}});
//     }
//   }
//
//   llvm::outs() << Doc;
//   // [{"number":1},true,{"number":3}]
//
//===----------------------------------------------------------------------===//
//
// Caveats (fix me!):
//  - parser is recursive-descent, so can overflow on malicious input
//  - parser can reuse strings. Not for object keys - can we fix this?
//  - StringMap may be suboptimal for tiny objects (vector too?)
//  - it's probably safe to skip ~Value() and abandon arena-allocated values
//  - nested literals are copied at every level (as initializer_list is const)
//  - no support for prettyprinting/canonicalizing output
//  - UTF-8 in strings is not validated
//  - UTF-16 and UTF-32 streams are not supported
//  - encapsulation is weak - too many friend declarations
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_JSON_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_JSON_H

#include "llvm/ADT/iterator.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSONDetail.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace json {
class Object;
class Array;

// The different kinds of possible JSON values.
enum Kind : char {
  NullKind,
  BooleanKind,
  NumberKind,
  ObjectKind,
  ArrayKind,
  StringKind,
};

// A Literal is a temporary JSON structure used to construct Values.
// You can implicitly construct Literals holding:
//   - strings: std::string, SmallString, StringRef, Twine, char*, formatv
//              (Twines, StringRefs are not deep copies. Convert to std::string)
//   - numbers
//   - booleans
//   - null: nullptr
//   - arrays: {"foo", 42.0, false}
//         or: json::ary{"foo", 42.0, false}
//   - objects: json::obj{{"key1", val1}, {"key2", val2}}
//   - json::Value, json::Document
//              (Values are by-reference - convert to a Document for a copy.)
// Literals can be nested: Objects and Arrays contain arbitrary literals.
//
// Normally you don't need to name the type, just writing directly:
//   Document D = {"one", 2, "three"};  // Calls Document(const Literal&)
// If you do create Literals, be aware they don't own referenced strings/Values:
//   Literal L = {AString, {42, AValue}};
// L is only valid as long as AString and AValue are alive.
// Think of a Literal as the JSON version of an llvm::Twine.
using Literal = detail::Literal;
// array(Literal...) returns an array convertible to Literal.
// It has a push_back() function that can be used to build an array in a loop:
//
// json::Literal priceList() {
//   json::ary A;
//   for (int X : prices())
//     A.push_back(X);
//   return std::move(A);
// }
class ary {
public:
  ary(std::initializer_list<Literal> Args) {
    Vec.reserve(Args.size());
    for (const auto &Arg : Args)
      Vec.emplace_back(std::move(Arg));
  }
  void push_back(Literal L) { Vec.push_back(std::move(L)); }
  operator Literal() && { return Literal{std::move(Vec), Literal::LT_Array}; }

private:
  // Avoid ambiguous parse of ary{{2}} by disallowing implicit copy/move.
  explicit ary(ary&&) = delete;
  explicit ary(const ary&) = delete;
  std::vector<Literal> Vec;
};
// obj{{Twine, Literal}...} is an object convertible to Literal.
// It has a set() function that can be used to build an object in a loop:
//
// json::Literal priceMap() {
//   json::obj O;
//   for (const std::pair<string, int> X : prices())
//     O.set(X.first, X.second);
//   return std::move(O);
// }
class obj {
public:
  struct KV {  // Not std::pair, we need an implicit constructor.
    KV(Twine K, Literal &&V) : Key(std::move(K)), Value(std::move(V)) {};
    const Twine Key;
    const Literal Value;
  };
  obj(std::initializer_list<KV> Args) {
    for (const auto &Arg : Args)
      set(Arg.Key, std::move(Arg.Value));
  }
  void set(const Twine &K, Literal V) {
    Map.try_emplace(K.isSingleStringRef() ? K.getSingleStringRef()
                                          : StringRef(K.str()),
                    std::move(V));
  }
  operator Literal() && { return Literal{std::move(Map), Literal::LT_Object}; }

private:
  // Avoid ambiguous parse of obj{{2}} by disallowing implicit copy/move.
  explicit obj(obj&&) = delete;
  explicit obj(const obj&) = delete;
  StringMap<Literal> Map;
};

// Value is a generic JSON value of unknown type, owned by a Document.
//
// A Value may be a number, string, boolean, object, array, or null.
// The typed accessors let you access the Value's details.
//
// Value is obtained by reference, and is not copyable or movable. But you can:
//   - clone it into new document: Document D = SomeValue;
//   - copy it into an Array or Object: SomeObject.set("foo", SomeValue);
// References to Values are invalidated when their immediate parent is mutated.
class Value {
public:
  static Value Null;
  static Value True;
  static Value False;

  Value(Value &&) = delete;
  Value(Value &) = delete;
  Value &operator=(Value &&) = delete;
  Value &operator=(const Value &) = delete;
  ~Value();

  Kind kind() const {
    switch (U.getTag()) {
    case detail::VT_Sentinel:
      return U == Null ? NullKind : BooleanKind;
    case detail::VT_SmallInt:
    case detail::VT_Double:
      return NumberKind;
    case detail::VT_Array:
      return ArrayKind;
    case detail::VT_Object:
      return ObjectKind;
    case detail::VT_OwnedString:
    case detail::VT_StaticString:
      return StringKind;
    }
  }

  // Typed accessors return None/nullptr of the Value is not of this type.
  Optional<double> number() const {
    if (detail::SmallInt I = U.get<detail::VT_SmallInt>())
      return double(I);
    if (double *D = U.get<detail::VT_Double>())
      return *D;
    return None;
  }
  Optional<StringRef> string() const {
    if (detail::PString *PS = U.get<detail::VT_OwnedString>())
      return StringRef(*PS);
    if (const char *S = *U.get<detail::VT_StaticString>())
      return StringRef(S);
    return None;
  }
  Optional<bool> boolean() const {
    if (U == detail::TrueRep)
      return true;
    if (U == detail::FalseRep)
      return false;
    return None;
  }
  Object *object() { return U.get<detail::VT_Object>(); }
  const Object *object() const { return U.get<detail::VT_Object>(); }
  Array *array() { return U.get<detail::VT_Array>(); }
  const Array *array() const { return U.get<detail::VT_Array>(); }
  bool null() const { return U == detail::NullRep; }

  // Returns the JSON serialization of this Value.
  std::string str() const {
    std::string S;
    raw_string_ostream OS(S);
    OS << *this;
    return OS.str();
  }

  // Not part of the public interface (but only internals can name it)
  explicit operator detail::ValueRep() const { return U; }

private:
  Value(detail::ValueRep R) : U(R) {}

  // Value owns the lifetime of the object pointed to by U, but not its memory.
  detail::ValueRep U;
  friend detail::ValueWrapper;
  friend bool operator==(const Value &, const Value &);
  friend raw_ostream &operator<<(raw_ostream &, const class Value &);
};

// ValueWrapper provides move semantics for Value so containers can hold it.
// Only moves within a document are valid!
// \private
class detail::ValueWrapper {
public:
  explicit ValueWrapper(ValueRep R) : V(R) {}
  ValueWrapper(const ValueWrapper &Copy) = delete;
  ValueWrapper(ValueWrapper &&Move) : V(Move.V.U) { Move.V.U = NullRep; }
  ValueWrapper &operator=(const ValueWrapper &Copy) = delete;
  ValueWrapper &operator=(ValueWrapper &&Move) {
    V.~Value();
    new (&V) Value(Move.V.U);
    Move.V.U = detail::NullRep;
    return *this;
  }

  operator Value &() { return V; }
  operator const Value &() const { return V; }

private:
  Value V;
};

// Document is a self-contained JSON value of unknown type.
// It owns the entire tree of values, which are stored in a BumpPtrAllocator.
//
// Documents can be passed cheaply by value or reference, clone() is expensive.
class Document {
public:
  // Parses a document from a UTF-8 JSON stream.
  // The only error reported is due to malformed input.
  static Expected<Document> parse(StringRef text);

  // An empty document, with value null.
  explicit Document() // Use Document D = json::ary{} for an empty list doc!
      : Alloc(nullptr), Root(detail::NullRep) {}
  Document(Document &&) = default;
  // Create a document from a literal JSON expression (see Literal above).
  // Note that a json::Value may be passed here, it will be deep-copied.
  Document(const Literal &V)
      : Alloc(new detail::Arena()), Root(V.construct(*Alloc)) {}
  Document(Literal &&V)
      : Alloc(new detail::Arena()), Root(V.construct(*Alloc)) {}
  // This constructor is needed to disambiguate Document({{"foo"}}), which could
  // be Document(Document{Literal{"foo"}}) or Document(Literal{Literal{"foo"}}).
  Document(std::initializer_list<Literal> V) : Document(Literal(V)){};
  Document &operator=(Document &&Move) {
    // The defaulted operator= destroys Alloc first, then ~Value() writes to it!
    Root.~ValueWrapper();
    Alloc = std::move(Move.Alloc);
    new (&Root) detail::ValueWrapper(std::move(Move.Root));
    return *this;
  }
  Document &operator=(const Literal &V) { return *this = Document(V); }
  Document &operator=(std::initializer_list<Literal> V) {
    return *this = Literal(V);
  }

  // Explicitly obtain the Value of this document.
  Value &root() { return Root; }
  const Value &root() const { return Root; }

  // For most purposes, a Document can be treated as its root Value.
  Kind kind() const { return root().kind(); }
  Optional<double> number() const { return root().number(); }
  Optional<StringRef> string() const { return root().string(); }
  Optional<bool> boolean() const { return root().boolean(); }
  bool null() const { return root().null(); }
  Object *object() { return root().object(); }
  const Object *object() const { return root().object(); }
  Array *array() { return root().array(); }
  const Array *array() const { return root().array(); }
  std::string str() const { return root().str(); }
  operator Value &() { return Root; }
  operator const Value &() const { return Root; }

  // Clean up memory used by values orphaned by mutations.
  // This invalidates all Value references.
  void trim() { *this = clone(); }
  // Does a deep-copy (the copy-constructor is deleted as it is expensive).
  Document clone() const { return Document((const Value &)*this); }
  // Space actually used on the arena.
  size_t usedBytes() const { return Alloc ? Alloc->getBytesAllocated() : 0; }
  // Space consumed by the arena.
  size_t allocatedBytes() const { return Alloc ? Alloc->getTotalMemory() : 0; }

  class Parser;

private:
  Document(detail::ValueWrapper V, std::unique_ptr<detail::Arena> Alloc)
      : Alloc(std::move(Alloc)), Root(std::move(V)) {}

  std::unique_ptr<detail::Arena> Alloc;
  detail::ValueWrapper Root;
};

// An Array is a JSON array, conceptually similar to a std::vector<Value>.
// Elements written to the array are cloned to the document's backing arena.
// To assign at an index, call Ary.set(I, SomeVal), not Ary[I] = SomeVal.
// (The array knows how to clone onto the arena, but its elements do not).
class Array {
public:
  Array(detail::Arena &Alloc) : Elems(Vec::allocator_type{Alloc}) {}
  Array(size_t Size, detail::Arena &Alloc) : Array(Alloc) {
    Elems.reserve(Size);
  }
  Array(Array &&) = delete;
  Array(Array &) = delete;
  Array &operator=(Array &&) = delete;
  Array &operator=(const Array &) = delete;

  // Iterator API.
  using value_type = Value;
  class const_iterator;
  class iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  iterator begin() { return iterator(Elems.begin()); }
  const_iterator begin() const { return const_iterator(Elems.begin()); }
  iterator end() { return iterator(Elems.end()); }
  const_iterator end() const { return const_iterator(Elems.end()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  // Basic vector methods.
  size_t size() const { return Elems.size(); }
  void clear() { Elems.clear(); }
  void reserve(size_t N) { Elems.reserve(N); }
  void push_back(const Literal &V) {
    Elems.push_back(V.construct(Elems.get_allocator()));
  }
  void pop_back() { Elems.pop_back(); }
  iterator erase(const_iterator I) {
    return iterator(Elems.erase(I.wrapped()));
  }
  iterator erase(const_iterator B, const_iterator E) {
    return iterator(Elems.erase(B.wrapped(), E.wrapped()));
  }
  void insert(iterator I, const Literal &V) {
    Elems.insert(I.wrapped(), V.construct(Elems.get_allocator()));
  }
  Value &operator[](size_t I) { return Elems[I]; }
  const Value &operator[](size_t I) const { return Elems[I]; }
  void set(size_t I, const Literal &V) {
    Elems[I] = V.construct(Elems.get_allocator());
  }

  // Typed accessors.
  Optional<double> number(size_t I) const { return (*this)[I].number(); }
  Optional<StringRef> string(size_t I) const { return (*this)[I].string(); }
  Optional<bool> boolean(size_t I) const { return (*this)[I].boolean(); }
  bool null(size_t I) const { return (*this)[I].null(); }
  Array *array(size_t I) { return (*this)[I].array(); }
  const Array *array(size_t I) const { return (*this)[I].array(); }
  Object *object(size_t I) { return (*this)[I].object(); }
  const Object *object(size_t I) const { return (*this)[I].object(); }

private:
  // SmallVector is not allocator-aware. std::vector on an Arena behaves nicely!
  using Vec =
      std::vector<detail::ValueWrapper, detail::STLArena<detail::ValueWrapper>>;
  Vec Elems;
  friend class Document::Parser;
  friend detail::ValueRep detail::deepCopy(const detail::ValueRep &U,
                                           detail::Arena &Alloc);

public:
  class iterator : public iterator_adaptor_base<iterator, Vec::iterator,
                                                std::random_access_iterator_tag,
                                                value_type> {
    friend Array;

  public:
    iterator() = default;
    explicit iterator(Vec::iterator I) : iterator_adaptor_base(std::move(I)) {}
  };

  class const_iterator
      : public iterator_adaptor_base<const_iterator, Vec::const_iterator,
                                     std::random_access_iterator_tag,
                                     const value_type> {
    friend Array;

  public:
    const_iterator() = default;
    explicit const_iterator(Vec::const_iterator I)
        : iterator_adaptor_base(std::move(I)) {}
  };
};

// An Object is a JSON object, similar to a std::unordered_map<string, Value>.
// Elements written to the object are cloned to the document's backing arena.
// To assign at a key, call Obj.set(K, SomeVal), not Obj[K] = SomeVal.
// (The object knows how to clone onto the arena, but its elements do not).
class Object {
public:
  Object(detail::Arena &Alloc) : Props(Alloc) {}
  Object(size_t Size, detail::Arena &Alloc) : Props(Size, Alloc) {}
  Object(Object &&) = delete;
  Object(Object &) = delete;
  Object &operator=(Object &&) = delete;
  Object &operator=(const Object &) = delete;

  // Iterator API.
  using key_type = StringRef;
  using mapped_type = Value;
  struct value_type {
    value_type(StringRef first, Value &second) : first(first), second(second) {}
    const StringRef first;
    Value &second;
  };
  class const_iterator;
  class iterator;
  iterator begin() { return iterator(Props.begin()); }
  const_iterator begin() const { return const_iterator(Props.begin()); }
  iterator end() { return iterator(Props.end()); }
  const_iterator end() const { return const_iterator(Props.end()); }

  // Basic map methods.
  size_t size() const { return Props.size(); }
  void clear() { Props.clear(); }
  Value &operator[](StringRef K) {
    auto I = Props.find(K);
    return (I == Props.end()) ? Value::Null : (Value &)(I->second);
  }
  const Value &operator[](StringRef K) const {
    auto I = Props.find(K);
    return (I == Props.end()) ? Value::Null : (Value &)(I->second);
  }
  iterator find(StringRef K) { return iterator(Props.find(K)); }
  const_iterator find(StringRef K) const {
    return const_iterator(Props.find(K));
  }
  Value *get(StringRef K) {
    auto I = Props.find(K);
    return (I == Props.end()) ? nullptr : &(Value &)(I->second);
  }
  const Value *get(StringRef K) const {
    auto I = Props.find(K);
    return (I == Props.end()) ? nullptr : &(const Value &)(I->second);
  }
  size_t count(StringRef K) { return Props.count(K); }
  void set(StringRef K, const Literal &V) {
    auto VW = V.construct(Props.getAllocator());
    auto R = Props.try_emplace(K, std::move(VW));
    if (!R.second)
      R.first->second = std::move(VW);
  }
  std::pair<iterator, bool> insert(const value_type &V) {
    return emplace(V.first, V.second);
  }
  std::pair<iterator, bool> emplace(StringRef K, const Literal &V) {
    auto R = Props.try_emplace(K, V.construct(Props.getAllocator()));
    return {iterator(R.first), R.second};
  }
  void erase(iterator I) { Props.erase(I.wrapped()); }
  bool erase(StringRef K) { return Props.erase(K); }

  // Typed accessors.
  Optional<double> number(StringRef I) const {
    if (const Value *V = get(I))
      return V->number();
    return None;
  }
  Optional<StringRef> string(StringRef I) const {
    if (const Value *V = get(I))
      return V->string();
    return None;
  }
  Optional<bool> boolean(StringRef I) const {
    if (const Value *V = get(I))
      return V->boolean();
    return None;
  }
  bool null(StringRef I) const {
    if (const Value *V = get(I))
      return V->null();
    return false;
  }
  Array *array(StringRef I) {
    if (Value *V = get(I))
      return V->array();
    return nullptr;
  }
  const Array *array(StringRef I) const {
    if (const Value *V = get(I))
      return V->array();
    return nullptr;
  }
  Object *object(StringRef I) {
    if (Value *V = get(I))
      return V->object();
    return nullptr;
  }
  const Object *object(StringRef I) const {
    if (const Value *V = get(I))
      return V->object();
    return nullptr;
  }

private:
  using Map = StringMap<detail::ValueWrapper, detail::Arena &>;
  Map Props;
  friend Document::Parser;
  friend detail::ValueRep detail::deepCopy(const detail::ValueRep &U,
                                           detail::Arena &Alloc);

public:
  class iterator
      : public iterator_adaptor_base<iterator, Map::iterator,
                                     std::forward_iterator_tag, value_type> {
    Optional<value_type> Current;
    friend Object;

  public:
    iterator() = default;
    explicit iterator(Map::iterator I) : iterator_adaptor_base(std::move(I)) {}
    value_type &operator*() {
      Current.emplace(wrapped()->getKey(), wrapped()->second);
      return *Current;
    }
  };

  class const_iterator
      : public iterator_adaptor_base<const_iterator, Map::const_iterator,
                                     std::forward_iterator_tag,
                                     const value_type> {
    Optional<const value_type> Current;
    friend Object;

  public:
    const_iterator() = default;
    explicit const_iterator(Map::const_iterator I)
        : iterator_adaptor_base(std::move(I)) {}

    const value_type &operator*() {
      Current.emplace(wrapped()->getKey(),
                      const_cast<Value &>((const Value &)(wrapped()->second)));
      return *Current;
    }
  };
};

bool operator==(const Array &, const Array &);
bool operator==(const Object &, const Object &);
bool operator==(const Value &, const Value &);
inline bool operator!=(const Array &L, const Array &R) { return !(L == R); }
inline bool operator!=(const Object &L, const Object &R) { return !(L == R); }
inline bool operator!=(const Value &L, const Value &R) { return !(L == R); }
inline hash_code hash_value(const Array &A) {
  return hash_combine_range(A.begin(), A.end());
}
hash_code hash_value(const Object &O);
hash_code hash_value(const Value &O);
// Values are ordered Null < Boolean < Number < Object < Array < String.
// Primitive values of the same type have their natural ordering.
// Arrays are lexicographically ordered, Objects in some arbitrary order.
inline bool operator<(const Object &L, const Object &R) {
  // FIXME: define a meaningful ordering on Objects.
  return size_t(hash_value(L)) < size_t(hash_value(R));
}
inline bool operator<(const Array &L, const Array &R) {
  return std::lexicographical_compare(L.begin(), L.end(), R.begin(), R.end());
}
bool operator<(const Value &L, const Value &R);
// Arrays, Objects, and Values print their JSON representations.
raw_ostream &operator<<(raw_ostream &OS, const Array &V);
raw_ostream &operator<<(raw_ostream &OS, const Object &V);
raw_ostream &operator<<(raw_ostream &OS, const Value &V);

// For gtest. The default operator<< adapter doesn't work, as gtest needs ADL.
inline void PrintTo(const Array &V, std::ostream *S) {
  raw_os_ostream OS(*S);
  OS << V;
}
inline void PrintTo(const Object &V, std::ostream *S) {
  raw_os_ostream OS(*S);
  OS << V;
}
inline void PrintTo(const Value &V, std::ostream *S) {
  raw_os_ostream OS(*S);
  OS << V;
}
inline void PrintTo(const Document &V, std::ostream *S) {
  raw_os_ostream OS(*S);
  OS << V;
}

} // namespace json
} // namespace llvm

#endif
