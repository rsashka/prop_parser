#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#include <vector>
#include <string>
#include <cstddef>

class PropertyParser {
public:
    PropertyParser(bool caseInsensitive = false);
    
    // Feed data to the parser
    void feed(const std::vector<char>& data);
    void feed(const char* data, size_t length);
    
    // Parse next token
    bool parseNext();
    
    // Get parsing results
    bool isValid() const;
    const std::string& getPropertyName() const;
    const std::string& getPropertyValue() const;
    const std::string& getPropertyMatch() const;
    
    // Clear parser state
    void reset();
    
    // Pattern matching functionality
    static bool matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive = true);

private:
    std::vector<char> m_buffer;
    std::string m_propertyName;
    std::string m_propertyValue;
    std::string m_propertyMatch;
    bool m_isValid;
    bool m_caseInsensitive;
    
    bool findTokenBoundary(size_t& start, size_t& end);
    bool parseToken(const std::vector<char>& token);
};

#endif // PROPERTY_PARSER_H
