#include "ClangTidyTest.h"
#include "misc/ArgumentCommentCheck.h"
#include "misc/AssertionCountCheck.h"
#include "gtest/gtest.h"

namespace clang {
namespace tidy {
namespace test {
namespace {

template <typename Check>
std::unique_ptr<Check>
createCheck(const ClangTidyOptions &ExtraOptions = ClangTidyOptions()) {
  ClangTidyOptions Options = ExtraOptions;
  Options.Checks = "*";

  ClangTidyContext Context(llvm::make_unique<DefaultOptionsProvider>(
      ClangTidyGlobalOptions(), Options));

  return llvm::make_unique<Check>("misc-assertion-count", &Context);
}

// names of the config options used by the check
static const std::array<const char *, 4> ParamNames = {{
    "misc-assertion-count.LineThreshold",
    "misc-assertion-count.AssertsThreshold", "misc-assertion-count.LinesStep",
    "misc-assertion-count.AssertsStep",
}};
//  \/ and /\ are the exact same things.
using AssertionCountCheckParams =
    std::tuple<unsigned, unsigned, unsigned, unsigned>;

class AssertionCountCheckTest
    : public ::testing::TestWithParam<AssertionCountCheckParams> {
protected:
  AssertionCountCheckTest() = default;

  virtual void SetUp() {
    auto P = GetParam();

    static_assert(ParamNames.size() == std::tuple_size<ParamType>::value,
                  "must have the sample number of config options and values "
                  "for these options");

    ClangTidyOptions Opts;

    Opts.CheckOptions[ParamNames[0]] = std::to_string(std::get<0>(P));
    Opts.CheckOptions[ParamNames[1]] = std::to_string(std::get<1>(P));
    Opts.CheckOptions[ParamNames[2]] = std::to_string(std::get<2>(P));
    Opts.CheckOptions[ParamNames[3]] = std::to_string(std::get<3>(P));

    C = createCheck<misc::AssertionCountCheck>(Opts);
  }

  std::unique_ptr<misc::AssertionCountCheck> C;
};

// generates all possible combination of config options.
// the actual test data is stored in map, with key being this tuple.
INSTANTIATE_TEST_CASE_P(AssertionCountCheckTest, AssertionCountCheckTest,
                        ::testing::Combine(::testing::Values(0U, 10U),
                                           ::testing::Values(0U, 1U),
                                           ::testing::Values(0U, 20U),
                                           ::testing::Values(0U, 2U)));

// (Lines, expected result)
using AssertionCountCheckPoint = std::pair<unsigned, unsigned>;

// keeping the data in map allows to simultaneously actually store the data, and
// to sanity-check that all the data (as in, for all-NOP parameters) is present.
static std::map<AssertionCountCheckParams,
                std::array<AssertionCountCheckPoint, 28>>
    CheckMap{
        {
            // *always* at least one assertion
            std::make_tuple(0U, 1U, 0U, 0U),
            {{
                std::make_pair(0U, 1U),     std::make_pair(9U, 1U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },
        {
            // same as the last one
            // *always* at least one assertion
            std::make_tuple(0U, 1U, 0U, 2U),
            {{
                std::make_pair(0U, 1U),     std::make_pair(9U, 1U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },
        {
            // same as the last one
            // *always* at least one assertion
            std::make_tuple(0U, 1U, 20U, 0U),
            {{
                std::make_pair(0U, 1U),     std::make_pair(9U, 1U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },

        {
            // if no less than 10 lines, at least one assertion
            std::make_tuple(10U, 1U, 0U, 0U),
            {{
                std::make_pair(0U, 0U),     std::make_pair(9U, 0U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },
        {
            // same as the last one
            // if no less than 10 lines, at least one assertion
            std::make_tuple(10U, 1U, 0U, 2U),
            {{
                std::make_pair(0U, 0U),     std::make_pair(9U, 0U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },
        {
            // same as the last one
            // if no less than 10 lines, at least one assertion
            std::make_tuple(10U, 1U, 20U, 0U),
            {{
                std::make_pair(0U, 0U),     std::make_pair(9U, 0U),
                std::make_pair(10U, 1U),    std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),    std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),    std::make_pair(29U, 1U),
                std::make_pair(30U, 1U),    std::make_pair(31U, 1U),
                std::make_pair(39U, 1U),    std::make_pair(40U, 1U),
                std::make_pair(41U, 1U),    std::make_pair(49U, 1U),
                std::make_pair(50U, 1U),    std::make_pair(51U, 1U),
                std::make_pair(59U, 1U),    std::make_pair(60U, 1U),
                std::make_pair(61U, 1U),    std::make_pair(99U, 1U),
                std::make_pair(100U, 1U),   std::make_pair(101U, 1U),
                std::make_pair(999U, 1U),   std::make_pair(1000U, 1U),
                std::make_pair(1001U, 1U),  std::make_pair(9999U, 1U),
                std::make_pair(10000U, 1U), std::make_pair(10001U, 1U),
            }},
        },

        {
            // if more than 10 lines, 2 assertions per each 20 extra lines
            std::make_tuple(10U, 0U, 20U, 2U),
            {{
                std::make_pair(0U, 0U),       std::make_pair(9U, 0U),
                std::make_pair(10U, 0U),      std::make_pair(11U, 0U),
                std::make_pair(19U, 0U),      std::make_pair(20U, 0U),
                std::make_pair(21U, 0U),      std::make_pair(29U, 0U),
                std::make_pair(30U, 2U),      std::make_pair(31U, 2U),
                std::make_pair(39U, 2U),      std::make_pair(40U, 2U),
                std::make_pair(41U, 2U),      std::make_pair(49U, 2U),
                std::make_pair(50U, 4U),      std::make_pair(51U, 4U),
                std::make_pair(59U, 4U),      std::make_pair(60U, 4U),
                std::make_pair(61U, 4U),      std::make_pair(99U, 8U),
                std::make_pair(100U, 8U),     std::make_pair(101U, 8U),
                std::make_pair(999U, 98U),    std::make_pair(1000U, 98U),
                std::make_pair(1001U, 98U),   std::make_pair(9999U, 998U),
                std::make_pair(10000U, 998U), std::make_pair(10001U, 998U),
            }},
        },

        {
            // if no less than 10 lines, at least one assertion
            // plus, 2 assertions per each 20 extra lines
            std::make_tuple(10U, 1U, 20U, 2U),
            {{
                std::make_pair(0U, 0U),       std::make_pair(9U, 0U),
                std::make_pair(10U, 1U),      std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),      std::make_pair(20U, 1U),
                std::make_pair(21U, 1U),      std::make_pair(29U, 1U),
                std::make_pair(30U, 3U),      std::make_pair(31U, 3U),
                std::make_pair(39U, 3U),      std::make_pair(40U, 3U),
                std::make_pair(41U, 3U),      std::make_pair(49U, 3U),
                std::make_pair(50U, 5U),      std::make_pair(51U, 5U),
                std::make_pair(59U, 5U),      std::make_pair(60U, 5U),
                std::make_pair(61U, 5U),      std::make_pair(99U, 9U),
                std::make_pair(100U, 9U),     std::make_pair(101U, 9U),
                std::make_pair(999U, 99U),    std::make_pair(1000U, 99U),
                std::make_pair(1001U, 99U),   std::make_pair(9999U, 999U),
                std::make_pair(10000U, 999U), std::make_pair(10001U, 999U),
            }},
        },

        {
            // 2 assertions per each 20 extra lines
            std::make_tuple(0U, 0U, 20U, 2U),
            {{
                std::make_pair(0U, 0U),        std::make_pair(9U, 0U),
                std::make_pair(10U, 0U),       std::make_pair(11U, 0U),
                std::make_pair(19U, 0U),       std::make_pair(20U, 2U),
                std::make_pair(21U, 2U),       std::make_pair(29U, 2U),
                std::make_pair(30U, 2U),       std::make_pair(31U, 2U),
                std::make_pair(39U, 2U),       std::make_pair(40U, 4U),
                std::make_pair(41U, 4U),       std::make_pair(49U, 4U),
                std::make_pair(50U, 4U),       std::make_pair(51U, 4U),
                std::make_pair(59U, 4U),       std::make_pair(60U, 6U),
                std::make_pair(61U, 6U),       std::make_pair(99U, 8U),
                std::make_pair(100U, 10U),     std::make_pair(101U, 10U),
                std::make_pair(999U, 98U),     std::make_pair(1000U, 100U),
                std::make_pair(1001U, 100U),   std::make_pair(9999U, 998U),
                std::make_pair(10000U, 1000U), std::make_pair(10001U, 1000U),
            }},
        },

        {
            // at least one assertion
            // plus, 2 assertions per each 20 extra lines
            std::make_tuple(0U, 1U, 20U, 2U),
            {{
                std::make_pair(0U, 1U),        std::make_pair(9U, 1U),
                std::make_pair(10U, 1U),       std::make_pair(11U, 1U),
                std::make_pair(19U, 1U),       std::make_pair(20U, 3U),
                std::make_pair(21U, 3U),       std::make_pair(29U, 3U),
                std::make_pair(30U, 3U),       std::make_pair(31U, 3U),
                std::make_pair(39U, 3U),       std::make_pair(40U, 5U),
                std::make_pair(41U, 5U),       std::make_pair(49U, 5U),
                std::make_pair(50U, 5U),       std::make_pair(51U, 5U),
                std::make_pair(59U, 5U),       std::make_pair(60U, 7U),
                std::make_pair(61U, 7U),       std::make_pair(99U, 9U),
                std::make_pair(100U, 11U),     std::make_pair(101U, 11U),
                std::make_pair(999U, 99U),     std::make_pair(1000U, 101U),
                std::make_pair(1001U, 101U),   std::make_pair(9999U, 999U),
                std::make_pair(10000U, 1001U), std::make_pair(10001U, 1001U),
            }},
        },
    };

TEST_P(AssertionCountCheckTest, Test) {
  // if for some huge number of lines it says zero assertions are expected,
  // then these params are NOP

  // this is indeed a magic number
  // any value sufficiently larger than 0 should be fine.

  if (C->expectedAssertionsMin(1000) == 0) {
    EXPECT_TRUE(C->isNOP());
    EXPECT_TRUE(CheckMap.find(GetParam()) == CheckMap.end());
  } else {
    EXPECT_FALSE(C->isNOP());

    const auto DataPoints = CheckMap.find(GetParam());
    ASSERT_TRUE(DataPoints != CheckMap.end());
    // yes, this is an ASSERT. bad things would happen if this EXPECT would fail

    for (const auto &CheckPoint : DataPoints->second)
      EXPECT_EQ(C->expectedAssertionsMin(CheckPoint.first), CheckPoint.second)
          << "   Where Lines: " << CheckPoint.first;
  }
}

} // anonymous namespace
} // namespace test
} // namespace tidy
} // namespace clang
