#include "llvm/Support/JSON.h"
#include "llvm/ADT/SmallString.h"
#include "gtest/gtest.h"

namespace llvm {
namespace json {
namespace {

TEST(JSONTest, Literal) {
  auto JSON = [](Literal L) { return Document(L).str(); };

  EXPECT_EQ(R"(null)", JSON(nullptr));
  EXPECT_EQ(R"(1)", JSON(1));
  EXPECT_EQ(R"(1.5)", JSON(1.5));
  EXPECT_EQ(R"(true)", JSON(true));
  EXPECT_EQ(R"("foo")", JSON("foo"));

  EXPECT_EQ(R"([])", JSON({}));
  EXPECT_EQ(R"([1])", JSON({1}));
  EXPECT_EQ(R"([1,2])", JSON({1,2}));
  EXPECT_EQ(R"([[1]])", JSON({{1}}));

  EXPECT_EQ(R"({})", JSON(obj{}));
  EXPECT_EQ(R"({"foo":1})", JSON(obj{{"foo",1}}));
  EXPECT_EQ(R"({"foo":1,"bar":2})", JSON(obj{{"foo", 1}, {"bar", 2}}));
  EXPECT_EQ(R"({"foo":1,"bar":{"qux":42}})", JSON(obj{
                                                {"foo", 1},
                                                {"bar", obj{{"qux", 42}}},
                                            }));
}

TEST(JSONTest, Parse) {
  auto Compare = [](StringRef S, Literal Expected) {
    auto D = Document::parse(S);
    if (D)
      EXPECT_EQ(*D, Document(Expected));
    else
      handleAllErrors(D.takeError(), [S](const ErrorInfoBase &E) {
        FAIL() << "Failed to parse JSON >>> " << S << " <<<: " << E.message();
      });
  };

  Compare(R"(true)", true);
  Compare(R"(false)", false);
  Compare(R"(null)", nullptr);

  Compare(R"(42)", 42);
  Compare(R"(2.5)", 2.5);
  Compare(R"(2e50)", 2e50);
  Compare(R"(1.2e3456789)", 1.0/0.0);

  Compare(R"("foo")", "foo");
  Compare(R"("\"\\\b\f\n\r\t")", "\"\\\b\f\n\r\t");
  Compare(R"("\u0000")", StringRef("\0", 1));
  Compare("\"\x7f\"", "\x7f");
  Compare(R"("\ud801\udc37")", u8"\U00010437");  // UTF16 surrogate pair escape.
  Compare("\"\xE2\x82\xAC\xF0\x9D\x84\x9E\"", u8"\u20ac\U0001d11e"); // UTF8
  Compare(R"("\ud801")", u8"\ufffd"); // Invalid codepoint.

  Compare(R"({"":0,"":0})", obj{{"",0}});
  Compare(R"({"obj":{},"arr":[]})", obj{{"obj", obj{}}, {"arr", {}}});
}

// These are not really even worth looking at.
// TODO: write proper tests. And a benchmark.
TEST(JSONTest, All) {
  EXPECT_EQ(R"("a\\b\"c\nd\u0007e")", Document("a\\b\"c\nd\ae").str());
  EXPECT_EQ(R"("foo")", Document("foo").str());
  EXPECT_EQ(R"("foo")", Document(Twine("foo")).str());
  EXPECT_EQ(R"("foo")", Document(StringRef("foo")).str());
  EXPECT_EQ(R"("foo")", Document(SmallString<12>("foo")).str());
  EXPECT_EQ(R"("foo")", Document(std::string("foo")).str());
  EXPECT_EQ(R"(["foo"])", Document({"foo"}).str());
  EXPECT_EQ(R"(["foo"])", Document(Literal{"foo"}).str());
  EXPECT_EQ(R"([["foo"],"bar"])", Document({{"foo"}, "bar"}).str());
  EXPECT_EQ(R"(["one",true,3.5])", Document({"one", true, 3.5}).str());
  EXPECT_EQ(R"([])", Document({}).str());
  EXPECT_EQ(R"({"foo":42})", Document(obj{{"foo", 42}}).str());
  EXPECT_EQ(R"([{"foo":42}])", Document(Literal{obj{{"foo", 42}}}).str());
  EXPECT_EQ(R"([["foo",42]])", Document(ary{{"foo", 42}}).str());

  Document X = {"foo", json::obj{{"bar", 42}}};
  Value &Root = X;
  EXPECT_EQ(42, *Root.array()->object(1)->number("bar"));

  X = "bar";
  EXPECT_EQ(R"("bar")", X.str());

  X = {"foo", 42, "bar", false};
  EXPECT_EQ(sizeof(Array) + 2 * sizeof(char *) + 4 * sizeof(detail::ValueRep),
            X.usedBytes());
  EXPECT_EQ(detail::FirstSlabSize, X.allocatedBytes());

  Expected<Document> D = Document::parse(R"([["foo"], 42])");
  ASSERT_FALSE(!D);
  EXPECT_EQ(Document({{"foo"}, 42}), D.get());

  D = Document::parse(R"([["foo",], 42])");
  ASSERT_TRUE(!D);
  handleAllErrors(D.takeError(), [](const ErrorInfoBase &EI) {
    EXPECT_EQ("[1:9, byte=9]: Expected JSON value", EI.message());
  });

  EXPECT_TRUE(Document{} < Document{42});
  EXPECT_FALSE(Document{42} < Document{42});
  EXPECT_TRUE(Document{42} < Document{43});
  EXPECT_TRUE(Document({42, 43}) < Document({42, 44}));

  Document A = {{"foo", 42}}, B = {{"bar", 44}};
  EXPECT_TRUE((A < B) ^ (B < A));

  json::ary Ary{1};
  for (int I = 2; I < 5; ++I)
    Ary.push_back(I);
  Ary.push_back({100});
  EXPECT_EQ(R"([1,2,3,4,[100]])", Document(std::move(Ary)).str());

  // XXX Document DD = (json::ary{{"foo","bar"}});
  // XXX EXPECT_EQ(R"([["foo","bar"]])", DD.str());
  // XXX Document EE = json::obj{{"foo","bar"}};
  // XXX EXPECT_EQ(R"({"foo","bar"})", EE.str());

  json::obj::KV TL = {"foo",{"bar"}};
  json::obj Obj{{"foo",{"bar"}}};
  Obj.set("one", 1);
  Obj.set(Twine(42), {100});
  EXPECT_EQ(R"({"one":1,"foo":["bar"],"42":[100]})",
            Document(std::move(Obj)).str());

  Document Y = {4};
  EXPECT_EQ("[4]", Y.str());

  Document V = {{{{4}}}};
  EXPECT_EQ("[[[[4]]]]", V.str());

  Document V2= {{{{"x",4}}}};
  EXPECT_EQ("[[[[\"x\",4]]]]", V2.str());

  Document DefaultDoc;
  EXPECT_EQ("null", DefaultDoc.str());
}

} // namespace
} // namespace json
} // namespace llvm
