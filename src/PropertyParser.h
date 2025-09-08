#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#include <vector>
#include <string>
#include <cstddef>

class PropertyParser {
public:
    PropertyParser();
    
    // Feed data to the parser
    void feed(const std::vector<char>& data);
    void feed(const char* data, size_t length);
    
    // Parse next token
    bool parseNext();
    
    // Get parsing results
    bool isValid() const;
    const std::string& getPropertyName() const;
    const std::string& getPropertyValue() const;
    
    // Clear parser state
    void reset();
    
    // Pattern matching functionality
    static bool matchesPattern(const std::string& str, const std::string& pattern);

private:
    std::vector<char> buffer_;
    std::string propertyName_;
    std::string propertyValue_;
    bool isValid_;
    
    bool findTokenBoundary(size_t& start, size_t& end);
    bool parseToken(const std::vector<char>& token);
};

#endif // PROPERTY_PARSER_H
