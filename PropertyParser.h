#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#include <cstddef>
#include <string>
#include <vector>

// Forward declaration for callback function
class PropertyParser;

// Callback function type: takes a void pointer and a reference to the parser object
typedef void (*PropertyParserCallback)(void*, const PropertyParser&);

class PropertyParser {
public:
    // Legacy/extended constructor that allows controlling internal buffer size
    PropertyParser(size_t maxBufferSize, bool caseInsensitive = false);

    // Feed data to the parser and immediately try to parse with callback
    void feedAndParse(const char* data, size_t length, PropertyParserCallback callback = nullptr,
                      void* callbackData = nullptr);

    // Parse next token from internal buffer. Returns true if a token was consumed.
    bool parseNext();

    // Get parsing results
    bool isValid() const;
    const std::string& getPropertyName() const;
    const std::string& getPropertyValue() const;

    // If a token does not contain '=', it is stored here.
    const std::string& getPropertyMatch() const;

    // Clear parser state
    void reset();

    // Pattern matching functionality
    static bool matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive = true);

    // Find a single property by name in a raw buffer.
    // Returns true if found and sets valueBegin to the address of the first character of the value
    // in the original buffer (not trimmed / not unescaped).
    static bool findPropertyValue(const char* data, size_t length, const std::string& name,
                                  const char*& valueBegin, bool caseSensitive = true);

private:
    std::vector<char> m_buffer;

    std::string m_propertyName;
    std::string m_propertyValue;
    std::string m_propertyMatch;

    bool m_isValid{false};
    bool m_caseInsensitive{false};

    // Extract next token from buffer according to README rules.
    // Returns true if a token boundary was found or buffer is full; false if need more data.
    bool extractNextToken(std::string& token);

    bool parseToken(const std::string& token);

    static bool equalsName(const std::string& a, const std::string& b, bool caseSensitive);
};

#endif // PROPERTY_PARSER_H
