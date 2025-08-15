#include "PropertyParser.h"
#include <algorithm>
#include <cctype>

PropertyParser::PropertyParser() : isValid_(false) {}

void PropertyParser::feed(const std::vector<char>& data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void PropertyParser::feed(const char* data, size_t length) {
    buffer_.insert(buffer_.end(), data, data + length);
}

bool PropertyParser::findTokenBoundary(size_t& start, size_t& end) {
    // Skip leading CR/LF
    start = 0;
    while (start < buffer_.size() && (buffer_[start] == '\n' || buffer_[start] == '\r')) {
        start++;
    }
    
    if (start >= buffer_.size()) {
        return false;
    }
    
    // Find end of token (LF or CRLF)
    end = start;
    while (end < buffer_.size()) {
        if (buffer_[end] == '\n') {
            break;
        }
        if (buffer_[end] == '\r' && (end + 1 < buffer_.size()) && buffer_[end+1] == '\n') {
            break;
        }
        end++;
    }
    
    return end < buffer_.size();
}

bool PropertyParser::parseToken(const std::vector<char>& token) {
    // Convert token to string for easier processing
    std::string tokenStr(token.begin(), token.end());
    
    // Find '=' position
    size_t eqPos = tokenStr.find('=');
    if (eqPos == std::string::npos) {
        return false;
    }
    
    // Extract property name (trim whitespace)
    size_t nameStart = 0;
    size_t nameEnd = eqPos;
    while (nameStart < nameEnd && std::isspace(tokenStr[nameStart])) nameStart++;
    while (nameEnd > nameStart && std::isspace(tokenStr[nameEnd-1])) nameEnd--;
    
    if (nameStart == nameEnd) {
        return false;
    }
    
    propertyName_ = tokenStr.substr(nameStart, nameEnd - nameStart);
    
    // Extract property value (trim only leading/trailing whitespace)
    size_t valueStart = eqPos + 1;
    size_t valueEnd = tokenStr.size();
    
    // Preserve special characters but remove surrounding whitespace
    while (valueStart < valueEnd && std::isspace(tokenStr[valueStart])) valueStart++;
    while (valueEnd > valueStart && std::isspace(tokenStr[valueEnd-1])) valueEnd--;
    
    propertyValue_ = tokenStr.substr(valueStart, valueEnd - valueStart);
    return true;
}

bool PropertyParser::parseNext() {
    isValid_ = false;
    propertyName_.clear();
    propertyValue_.clear();
    
    size_t tokenStart, tokenEnd;
    if (!findTokenBoundary(tokenStart, tokenEnd)) {
        return false;
    }
    
    // Extract token
    std::vector<char> token(buffer_.begin() + tokenStart, buffer_.begin() + tokenEnd);
    
    // Parse the token
    isValid_ = parseToken(token);
    
    // Remove processed data including the delimiter
    size_t removeEnd = tokenEnd;
    if (removeEnd < buffer_.size() && buffer_[removeEnd] == '\r') removeEnd++;
    if (removeEnd < buffer_.size() && buffer_[removeEnd] == '\n') removeEnd++;
    
    buffer_.erase(buffer_.begin(), buffer_.begin() + removeEnd);
    
    return true;
}

bool PropertyParser::isValid() const {
    return isValid_;
}

const std::string& PropertyParser::getPropertyName() const {
    return propertyName_;
}

const std::string& PropertyParser::getPropertyValue() const {
    return propertyValue_;
}

void PropertyParser::reset() {
    buffer_.clear();
    propertyName_.clear();
    propertyValue_.clear();
    isValid_ = false;
}