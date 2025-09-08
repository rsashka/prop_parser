#include "PropertyParser.h"
#include <algorithm>
#include <cctype>
#include <iostream>

PropertyParser::PropertyParser(bool caseInsensitive) 
    : m_isValid(false), m_caseInsensitive(caseInsensitive) {}

void PropertyParser::feed(const std::vector<char>& data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

void PropertyParser::feed(const char* data, size_t length) {
    m_buffer.insert(m_buffer.end(), data, data + length);
}

bool PropertyParser::findTokenBoundary(size_t& start, size_t& end) {
    // Skip leading CR/LF
    start = 0;
    while (start < m_buffer.size() && (m_buffer[start] == '\n' || m_buffer[start] == '\r')) {
        start++;
    }
    
    if (start >= m_buffer.size()) {
        return false;
    }
    
    // Find end of token (LF or CRLF)
    end = start;
    while (end < m_buffer.size()) {
        if (m_buffer[end] == '\n') {
            break;
        }
        if (m_buffer[end] == '\r' && (end + 1 < m_buffer.size()) && m_buffer[end+1] == '\n') {
            break;
        }
        end++;
    }
    
    return end < m_buffer.size();
}

bool PropertyParser::parseToken(const std::vector<char>& token) {
    // Convert token to string for easier processing
    std::string tokenStr(token.begin(), token.end());
    
    // Find '=' position
    size_t eqPos = tokenStr.find('=');
    if (eqPos == std::string::npos) {
        // No '=' found, store the entire token in m_propertyMatch
        m_propertyMatch = tokenStr;
        
        // Trim whitespace from m_propertyMatch
        size_t start = 0;
        size_t end = m_propertyMatch.size();
        while (start < end && std::isspace(m_propertyMatch[start])) start++;
        while (end > start && std::isspace(m_propertyMatch[end-1])) end--;
        
        if (start < end) {
            m_propertyMatch = m_propertyMatch.substr(start, end - start);
        } else {
            m_propertyMatch.clear();
        }
        
        // Convert property match to lowercase if case insensitive mode is enabled
        if (m_caseInsensitive) {
            std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(), 
                          [](unsigned char c){ return std::tolower(c); });
        }
        
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
    
    m_propertyName = tokenStr.substr(nameStart, nameEnd - nameStart);
    
    // Convert property name to lowercase if case insensitive mode is enabled
    if (m_caseInsensitive) {
        std::transform(m_propertyName.begin(), m_propertyName.end(), m_propertyName.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
    }
    
    // Extract property value (trim only leading/trailing whitespace)
    size_t valueStart = eqPos + 1;
    size_t valueEnd = tokenStr.size();
    
    // Preserve special characters but remove surrounding whitespace
    while (valueStart < valueEnd && std::isspace(tokenStr[valueStart])) valueStart++;
    while (valueEnd > valueStart && std::isspace(tokenStr[valueEnd-1])) valueEnd--;
    
    m_propertyValue = tokenStr.substr(valueStart, valueEnd - valueStart);
    return true;
}

bool PropertyParser::parseNext() {
    m_isValid = false;
    m_propertyName.clear();
    m_propertyValue.clear();
    m_propertyMatch.clear();
    
    size_t tokenStart, tokenEnd;
    if (!findTokenBoundary(tokenStart, tokenEnd)) {
        return false;
    }
    
    // Extract token
    std::vector<char> token(m_buffer.begin() + tokenStart, m_buffer.begin() + tokenEnd);
    
    // Parse the token
    m_isValid = parseToken(token);
    
    // Remove processed data including the delimiter
    size_t removeEnd = tokenEnd;
    if (removeEnd < m_buffer.size() && m_buffer[removeEnd] == '\r') removeEnd++;
    if (removeEnd < m_buffer.size() && m_buffer[removeEnd] == '\n') removeEnd++;
    
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + removeEnd);
    
    return true;
}

bool PropertyParser::isValid() const {
    return m_isValid;
}

const std::string& PropertyParser::getPropertyName() const {
    return m_propertyName;
}

const std::string& PropertyParser::getPropertyValue() const {
    return m_propertyValue;
}

const std::string& PropertyParser::getPropertyMatch() const {
    return m_propertyMatch;
}

void PropertyParser::reset() {
    m_buffer.clear();
    m_propertyName.clear();
    m_propertyValue.clear();
    m_propertyMatch.clear();
    m_isValid = false;
}

bool PropertyParser::matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive) {
    // Create copies for case insensitive comparison
    std::string strCopy = str;
    std::string patternCopy = pattern;
    
    if (!caseSensitive) {
        // Convert both strings to lowercase for comparison
        std::transform(strCopy.begin(), strCopy.end(), strCopy.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        std::transform(patternCopy.begin(), patternCopy.end(), patternCopy.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
    }
    
    size_t strIndex = 0;
    size_t patternIndex = 0;
    size_t starIndex = std::string::npos;
    size_t matchIndex = 0;
    
    while (strIndex < strCopy.length()) {
        // Если символы совпадают или в шаблоне '?'
        if (patternIndex < patternCopy.length() && 
            (patternCopy[patternIndex] == '?' || patternCopy[patternIndex] == strCopy[strIndex])) {
            strIndex++;
            patternIndex++;
        }
        // Если в шаблоне '*' - запоминаем его позицию
        else if (patternIndex < patternCopy.length() && patternCopy[patternIndex] == '*') {
            starIndex = patternIndex;
            matchIndex = strIndex;
            patternIndex++;
        }
        // Если не совпадает и не '*', но был '*' ранее
        else if (starIndex != std::string::npos) {
            patternIndex = starIndex + 1;
            matchIndex++;
            strIndex = matchIndex;
        }
        // Если не совпадает и не было '*' ранее
        else {
            return false;
        }
    }
    
    // Проверяем, что оставшиеся символы в шаблоне - это только '*'
    while (patternIndex < patternCopy.length() && patternCopy[patternIndex] == '*') {
        patternIndex++;
    }
    
    // Совпадение есть, если мы дошли до конца шаблона
    return patternIndex == patternCopy.length();
}
