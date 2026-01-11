#include "PropertyParser.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

// Structure for callback invocations data
struct CallbackData {
    int callCount;
    std::vector<std::string> propertyNames;
    std::vector<std::string> propertyValues;
    std::vector<std::string> propertyMatches;
    std::vector<bool> isValidFlags;

    CallbackData() : callCount(0) {}
};

// Callback for testing
static void testCallback(void* data, const PropertyParser& parser) {
    auto* callbackData = static_cast<CallbackData*>(data);
    callbackData->callCount++;
    callbackData->propertyNames.push_back(parser.getPropertyName());
    callbackData->propertyValues.push_back(parser.getPropertyValue());
    callbackData->propertyMatches.push_back(parser.getPropertyMatch());
    callbackData->isValidFlags.push_back(parser.isValid());
}

// ---------------- Pattern matching ----------------

TEST(PropertyParserTest, ExactMatchPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.MyTest"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.OtherTest"));
}

TEST(PropertyParserTest, WildcardStarPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.subpackage.MyTest", "com.example.*"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.other.MyTest", "com.example.*"));
}

TEST(PropertyParserTest, WildcardQuestionPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("MyTest", "My?est"));
    EXPECT_TRUE(PropertyParser::matchesPattern("MyTest", "??????"));
    EXPECT_FALSE(PropertyParser::matchesPattern("MyTest", "?????"));
}

TEST(PropertyParserTest, ComplexPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.*.My?e??"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.subpackage.MyTest", "com.**.MyTest"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.example.MyTest", "com.*.Other?e??"));
}

TEST(PropertyParserTest, EdgeCasesPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("", ""));
    EXPECT_TRUE(PropertyParser::matchesPattern("", "*"));
    EXPECT_FALSE(PropertyParser::matchesPattern("", "?"));

    EXPECT_TRUE(PropertyParser::matchesPattern("anything", "***"));

    EXPECT_TRUE(PropertyParser::matchesPattern("very.long.package.name.MyTest", "*.MyTest"));
}

TEST(PropertyParserTest, CaseInsensitivePatternMatching) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*", false));
    EXPECT_TRUE(PropertyParser::matchesPattern("COM.EXAMPLE.SUBPACKAGE.MYTEST", "com.*.mytest", false));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.other.MyTest", "com.example.*", false));

    EXPECT_FALSE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"));
}

// ---------------- feedAndParse basic ----------------

TEST(PropertyParserTest, FeedAndParseWithValidProperty) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("name=value\n", 11, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "name");
    EXPECT_EQ(callbackData.propertyValues[0], "value");
    EXPECT_EQ(callbackData.propertyMatches[0], "");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseWithInvalidProperty) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("invalid\n", 8, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "");
    EXPECT_EQ(callbackData.propertyValues[0], "");
    EXPECT_EQ(callbackData.propertyMatches[0], "invalid");
    EXPECT_FALSE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseMultipleProperties) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("a=1\nb=2\nc=3\n", 12, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 3);

    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_TRUE(callbackData.isValidFlags[0]);

    EXPECT_EQ(callbackData.propertyNames[1], "b");
    EXPECT_EQ(callbackData.propertyValues[1], "2");
    EXPECT_TRUE(callbackData.isValidFlags[1]);

    EXPECT_EQ(callbackData.propertyNames[2], "c");
    EXPECT_EQ(callbackData.propertyValues[2], "3");
    EXPECT_TRUE(callbackData.isValidFlags[2]);
}

TEST(PropertyParserTest, FeedAndParseMixedValidAndInvalid) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("valid=value\ninvalid\n", 20, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 2);

    EXPECT_EQ(callbackData.propertyNames[0], "valid");
    EXPECT_EQ(callbackData.propertyValues[0], "value");
    EXPECT_TRUE(callbackData.isValidFlags[0]);

    EXPECT_EQ(callbackData.propertyNames[1], "");
    EXPECT_EQ(callbackData.propertyValues[1], "");
    EXPECT_EQ(callbackData.propertyMatches[1], "invalid");
    EXPECT_FALSE(callbackData.isValidFlags[1]);
}

TEST(PropertyParserTest, FeedAndParseWithNoCallback) {
    PropertyParser parser(1024, false);

    parser.feedAndParse("name=value\n", 11);

    // after feedAndParse() parser should not keep last result
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "");
    EXPECT_EQ(parser.getPropertyValue(), "");
    EXPECT_EQ(parser.getPropertyMatch(), "");
}

TEST(PropertyParserTest, FeedAndParseEmptyData) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("", 0, testCallback, &callbackData);

    EXPECT_EQ(callbackData.callCount, 0);
}

TEST(PropertyParserTest, FeedAndParsePartialData) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    parser.feedAndParse("partial=token", 13, testCallback, &callbackData);
    EXPECT_EQ(callbackData.callCount, 0);

    parser.feedAndParse("\n", 1, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "partial");
    EXPECT_EQ(callbackData.propertyValues[0], "token");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseCaseInsensitive) {
    CallbackData callbackData;
    PropertyParser parser(1024, true);

    parser.feedAndParse("Name=Value\n", 11, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "name");   // lower-cased key
    EXPECT_EQ(callbackData.propertyValues[0], "Value"); // value untouched (except quotes handling)
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseCaseInsensitivePropertyMatch) {
    CallbackData callbackData;
    PropertyParser parser(1024, true);

    parser.feedAndParse("InvalidString\n", 14, testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "");
    EXPECT_EQ(callbackData.propertyMatches[0], "invalidstring");
    EXPECT_FALSE(callbackData.isValidFlags[0]);
}

// ---------------- README features: separators ----------------

TEST(PropertyParserTest, SeparatorSemicolon) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "a=1;b=2;c=3;";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 3);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_EQ(callbackData.propertyNames[1], "b");
    EXPECT_EQ(callbackData.propertyValues[1], "2");
    EXPECT_EQ(callbackData.propertyNames[2], "c");
    EXPECT_EQ(callbackData.propertyValues[2], "3");
}

TEST(PropertyParserTest, SeparatorCRLF) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "a=1\r\nb=2\r\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 2);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_EQ(callbackData.propertyNames[1], "b");
    EXPECT_EQ(callbackData.propertyValues[1], "2");
}

// ---------------- README features: whitespace + comments ----------------

TEST(PropertyParserTest, IgnoresSpacesTabsAndLineComments) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    // spaces/tabs ignored, '#' starts a line comment
    const char* src = " \t name \t=\t value \t # comment here\r\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "name");
    EXPECT_EQ(callbackData.propertyValues[0], "value");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, IgnoresBlockComments) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "a/*ignored*/=/*ignored*/1\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

// ---------------- README features: line continuation ----------------

TEST(PropertyParserTest, BackslashLineContinuationLF) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "a=hel\\\nlo\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "hello");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, BackslashLineContinuationCRLF) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "a=hel\\\r\nlo\r\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "hello");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

// ---------------- README features: quoted strings + escaping ----------------

TEST(PropertyParserTest, QuotedStringValue) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "s=\"hello world\"\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "s");
    EXPECT_EQ(callbackData.propertyValues[0], "hello world");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, QuotedStringEscapedQuotes) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "s=\"hello \\\"world\\\"\"\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "s");
    EXPECT_EQ(callbackData.propertyValues[0], "hello \"world\"");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, UnclosedQuotedStringIsInvalid) {
    CallbackData callbackData;
    PropertyParser parser(1024, false);

    const char* src = "s=\"hello\n";
    parser.feedAndParse(src, std::strlen(src), testCallback, &callbackData);

    ASSERT_EQ(callbackData.callCount, 1);
    EXPECT_FALSE(callbackData.isValidFlags[0]);
    EXPECT_EQ(callbackData.propertyNames[0], "");
    EXPECT_EQ(callbackData.propertyMatches[0], "s=\"hello");
}

// ---------------- static finder ----------------

TEST(PropertyParserTest, FindPropertyValueCaseSensitive) {
    const char* valueBegin = nullptr;
    const char* src = "a=1\nb=2\n";
    ASSERT_TRUE(PropertyParser::findPropertyValue(src, std::strlen(src), "b", valueBegin, true));
    ASSERT_NE(valueBegin, nullptr);
    EXPECT_EQ(std::string(valueBegin, 1), "2");
}

TEST(PropertyParserTest, FindPropertyValueCaseInsensitive) {
    const char* valueBegin = nullptr;
    const char* src = "Name=Value\n";
    ASSERT_TRUE(PropertyParser::findPropertyValue(src, std::strlen(src), "name", valueBegin, false));
    ASSERT_NE(valueBegin, nullptr);
    EXPECT_EQ(std::string(valueBegin, 5), "Value");
}

TEST(PropertyParserTest, FindPropertyValueNotFound) {
    const char* valueBegin = reinterpret_cast<const char*>(0x1);
    const char* src = "a=1\n";
    EXPECT_FALSE(PropertyParser::findPropertyValue(src, std::strlen(src), "missing", valueBegin, true));
    EXPECT_EQ(valueBegin, nullptr);
}

// ---------------- Big input / small buffer ----------------

TEST(PropertyParserTest, FeedAndParseLargeData) {
    CallbackData callbackData;
    PropertyParser parser(10, false);

    std::string largeData = "a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\nh=8\ni=9\nj=10\nk=11\nl=12\nm=13\nn=14\no=15\n";

    parser.feedAndParse(largeData.c_str(), largeData.length(), testCallback, &callbackData);

    EXPECT_EQ(callbackData.callCount, 15);
    EXPECT_EQ(callbackData.propertyNames.size(), 15);
    EXPECT_TRUE(callbackData.isValidFlags[0]);
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
}

TEST(PropertyParserTest, FeedAndParseLargeInvalidData) {
    CallbackData callbackData;
    PropertyParser parser(10, false);

    std::string largeInvalidData =
        "invalid1\ninvalid2\ninvalid3\ninvalid4\ninvalid5\ninvalid6\ninvalid7\ninvalid8\ninvalid9\ninvalid10\ninvalid11\ninvalid12\ninvalid13\ninvalid14\ninvalid15\n";

    parser.feedAndParse(largeInvalidData.c_str(), largeInvalidData.length(), testCallback, &callbackData);

    EXPECT_EQ(callbackData.callCount, 15);
    for (int i = 0; i < 15; i++) {
        EXPECT_FALSE(callbackData.isValidFlags[i]);
        EXPECT_EQ(callbackData.propertyNames[i], "");
        EXPECT_EQ(callbackData.propertyValues[i], "");
    }
    EXPECT_EQ(callbackData.propertyMatches[0], "invalid1");
    EXPECT_EQ(callbackData.propertyMatches[1], "invalid2");
    EXPECT_EQ(callbackData.propertyMatches[2], "invalid3");
}

// With the new tokenizer rules, a long line without delimiter may be emitted in multiple chunks (buffer-full tokens).
// We only assert that it gets processed and the first chunk matches the buffer size behaviour.
TEST(PropertyParserTest, FeedAndParseSingleLargeInvalidLine) {
    CallbackData callbackData;
    PropertyParser parser(10, false);

    std::string singleLargeInvalidLine =
        "this_is_a_very_long_invalid_line_that_exceeds_the_buffer_size_and_should_be_processed_correctly\n";

    parser.feedAndParse(singleLargeInvalidLine.c_str(), singleLargeInvalidLine.length(), testCallback, &callbackData);

    ASSERT_GE(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyMatches[0], "this_is_a_");
    EXPECT_FALSE(callbackData.isValidFlags[0]);
}
