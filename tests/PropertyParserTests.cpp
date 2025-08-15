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