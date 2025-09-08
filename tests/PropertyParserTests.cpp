#include "PropertyParser.h"
#include <gtest/gtest.h>
#include <vector>

TEST(PropertyParserTest, BasicParsing) {
    PropertyParser parser;
    parser.feed("name=value\n", 11);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "name");
    EXPECT_EQ(parser.getPropertyValue(), "value");
}

TEST(PropertyParserTest, CRLFTerminator) {
    PropertyParser parser;
    parser.feed("key=value\r\n", 12);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "key");
    EXPECT_EQ(parser.getPropertyValue(), "value");
}

TEST(PropertyParserTest, EmptyValue) {
    PropertyParser parser;
    parser.feed("empty=\n", 7);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "empty");
    EXPECT_EQ(parser.getPropertyValue(), "");
}

TEST(PropertyParserTest, MissingEquals) {
    PropertyParser parser;
    parser.feed("invalid\n", 8);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
}

// Новые тесты для propertyMatch
TEST(PropertyParserTest, PropertyMatchBasic) {
    PropertyParser parser;
    parser.feed("invalid\n", 8);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), "invalid");
}

TEST(PropertyParserTest, PropertyMatchWithWhitespace) {
    PropertyParser parser;
    parser.feed("  invalid  \n", 12);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), "invalid");
}

TEST(PropertyParserTest, PropertyMatchEmpty) {
    PropertyParser parser;
    parser.feed("\n", 1);
    EXPECT_FALSE(parser.parseNext()); // Empty line should not be parsed
}

TEST(PropertyParserTest, PropertyMatchOnlyWhitespace) {
    PropertyParser parser;
    parser.feed("  \n", 3);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), ""); // Only whitespace should result in empty match
}

TEST(PropertyParserTest, PropertyMatchWithCR) {
    PropertyParser parser;
    parser.feed("invalid\r\n", 9);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), "invalid");
}

TEST(PropertyParserTest, PropertyMatchMixedWithValidProperties) {
    PropertyParser parser;
    parser.feed("valid=value\n", 12);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "valid");
    EXPECT_EQ(parser.getPropertyValue(), "value");
    EXPECT_EQ(parser.getPropertyMatch(), ""); // Should be empty for valid properties
    
    parser.feed("invalid\n", 8);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), "invalid");
    // Property name and value should be empty for invalid tokens
    EXPECT_EQ(parser.getPropertyName(), "");
    EXPECT_EQ(parser.getPropertyValue(), "");
}

TEST(PropertyParserTest, WhitespaceHandling) {
    PropertyParser parser;
    parser.feed("  name  =  value  \n", 19);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "name");
    EXPECT_EQ(parser.getPropertyValue(), "value");
}

TEST(PropertyParserTest, MultipleProperties) {
    PropertyParser parser;
    parser.feed("a=1\nb=2\nc=3\n", 12);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "a");
    EXPECT_EQ(parser.getPropertyValue(), "1");
    
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "b");
    EXPECT_EQ(parser.getPropertyValue(), "2");
    
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "c");
    EXPECT_EQ(parser.getPropertyValue(), "3");
}

TEST(PropertyParserTest, ChunkedInput) {
    PropertyParser parser;
    parser.feed("na", 2);
    parser.feed("me=val", 6);
    parser.feed("ue\n", 3);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "name");
    EXPECT_EQ(parser.getPropertyValue(), "value");
}

TEST(PropertyParserTest, SpecialCharacters) {
    PropertyParser parser;
    std::string input = "special=!@#$%^&*()_+-=[]{};':\",./<>?\n";
    parser.feed(input.data(), input.size());
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "special");
    EXPECT_EQ(parser.getPropertyValue(), "!@#$%^&*()_+-=[]{};':\",./<>?");
}

TEST(PropertyParserTest, ResetFunctionality) {
    PropertyParser parser;
    parser.feed("first=value\n", 12);
    parser.parseNext();
    parser.reset();
    
    parser.feed("second=value\n", 13);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "second");
}

TEST(PropertyParserTest, PartialToken) {
    PropertyParser parser;
    parser.feed("partial=token", 13);
    EXPECT_FALSE(parser.parseNext());  // No terminator
    
    parser.feed("\n", 1);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "partial");
    EXPECT_EQ(parser.getPropertyValue(), "token");
}

TEST(PropertyParserTest, MixedLineEndings) {
    PropertyParser parser;
    parser.feed("a=1\r\nb=2\nc=3\r\n", 15);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "a");
    
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "b");
    
    EXPECT_TRUE(parser.parseNext());
    EXPECT_EQ(parser.getPropertyName(), "c");
}

TEST(PropertyParserTest, EmptyName) {
    PropertyParser parser;
    parser.feed("=value\n", 7);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
}

// Pattern matching tests
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
    // Пустая строка и шаблон
    EXPECT_TRUE(PropertyParser::matchesPattern("", ""));
    EXPECT_TRUE(PropertyParser::matchesPattern("", "*"));
    EXPECT_FALSE(PropertyParser::matchesPattern("", "?"));
    
    // Только звездочки
    EXPECT_TRUE(PropertyParser::matchesPattern("anything", "***"));
    
    // Совпадение с пустым шаблоном
    EXPECT_TRUE(PropertyParser::matchesPattern("", ""));
    
    // Длинная строка
    EXPECT_TRUE(PropertyParser::matchesPattern("very.long.package.name.MyTest", "*.MyTest"));
}

// Case insensitive tests
TEST(PropertyParserTest, CaseInsensitiveParsing) {
    PropertyParser parser(true); // case insensitive mode
    parser.feed("Name=Value\n", 11);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "name"); // Should be lowercase
    EXPECT_EQ(parser.getPropertyValue(), "Value"); // Value should remain unchanged
}

TEST(PropertyParserTest, CaseSensitiveParsing) {
    PropertyParser parser(false); // case sensitive mode (default)
    parser.feed("Name=Value\n", 11);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "Name"); // Should remain as is
    EXPECT_EQ(parser.getPropertyValue(), "Value");
}

TEST(PropertyParserTest, CaseInsensitivePatternMatching) {
    // Test case insensitive pattern matching
    EXPECT_TRUE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*", false));
    EXPECT_TRUE(PropertyParser::matchesPattern("COM.EXAMPLE.SUBPACKAGE.MYTEST", "com.*.mytest", false));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.other.MyTest", "com.example.*", false));
    
    // Test case sensitive pattern matching (default behavior)
    EXPECT_FALSE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"));
}

TEST(PropertyParserTest, MixedCaseProperties) {
    PropertyParser parser(true); // case insensitive mode
    parser.feed("MixedCaseName=MixedCaseValue\n", 29);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_TRUE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "mixedcasename"); // Should be lowercase
    EXPECT_EQ(parser.getPropertyValue(), "MixedCaseValue"); // Value should remain unchanged
}

TEST(PropertyParserTest, CaseInsensitivePropertyMatch) {
    PropertyParser parser(true); // case insensitive mode
    parser.feed("InvalidString\n", 14);
    EXPECT_TRUE(parser.parseNext());
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyMatch(), "invalidstring"); // Should be lowercase
}
