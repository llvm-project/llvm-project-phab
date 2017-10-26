#include "llvm/Support/JSON.h"
#include "llvm/ADT/SmallString.h"
#include "gtest/gtest.h"

namespace llvm {
namespace json {
namespace {

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
