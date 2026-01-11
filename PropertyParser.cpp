#include "PropertyParser.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace {

inline bool isSpaceOrTab(char c) { return c == ' ' || c == '\t'; }

inline char toLowerAscii(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

static std::string toLowerCopy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return out;
}

static bool equalsNameImpl(const std::string& a, const std::string& b, bool caseSensitive) {
    if (caseSensitive) {
        return a == b;
    }
    return toLowerCopy(a) == toLowerCopy(b);
}

} // namespace

PropertyParser::PropertyParser(size_t maxBufferSize, bool caseInsensitive)
    : m_buffer(maxBufferSize), m_isValid(false), m_caseInsensitive(caseInsensitive) {
    m_buffer.clear(); // keep capacity, start empty
}

bool PropertyParser::equalsName(const std::string& a, const std::string& b, bool caseSensitive) {
    return equalsNameImpl(a, b, caseSensitive);
}

void PropertyParser::feedAndParse(const char* data, size_t length, PropertyParserCallback callback, void* callbackData) {
    auto clearCurrentResult = [this]() {
        m_isValid = false;
        m_propertyName.clear();
        m_propertyValue.clear();
        m_propertyMatch.clear();
    };

    size_t processed = 0;
    while (processed < length) {
        const size_t availableSpace = m_buffer.capacity() - m_buffer.size();
        const size_t toProcess = std::min(length - processed, availableSpace);

        m_buffer.insert(m_buffer.end(), data + processed, data + processed + toProcess);
        processed += toProcess;

        // If we have a chance to parse (delimiter seen in appended chunk) or buffer full or end of input.
        bool mayHaveToken = (processed >= length) || (m_buffer.size() >= m_buffer.capacity());
        if (!mayHaveToken) {
            for (size_t i = m_buffer.size() - toProcess; i < m_buffer.size(); ++i) {
                if (m_buffer[i] == '\n' || m_buffer[i] == ';') {
                    mayHaveToken = true;
                    break;
                }
            }
        }

        if (!mayHaveToken) {
            continue;
        }

        while (true) {
            if (!parseNext()) {
                break;
            }

            if (callback && (m_isValid || !m_propertyMatch.empty())) {
                callback(callbackData, *this);
            }

            // Do not keep last result after feedAndParse() iteration.
            clearCurrentResult();
        }
    }
}

bool PropertyParser::extractNextToken(std::string& token) {
    token.clear();

    if (m_buffer.empty()) {
        return false;
    }

    // Skip leading CR/LF and separators.
    size_t i = 0;
    while (i < m_buffer.size() && (m_buffer[i] == '\n' || m_buffer[i] == '\r' || m_buffer[i] == ';')) {
        ++i;
    }
    if (i >= m_buffer.size()) {
        // Buffer has only separators; consume all.
        m_buffer.clear();
        return false;
    }

    bool inQuotes = false;
    bool escape = false;

    bool inLineComment = false;  // '#'
    bool inBlockComment = false; // /* ... */

    bool sawDelimiter = false;
    size_t endIndex = i;

    // Build token while tracking endIndex (where delimiter starts or buffer ends).
    for (; endIndex < m_buffer.size(); ++endIndex) {
        const char c = m_buffer[endIndex];
        const char next = (endIndex + 1 < m_buffer.size()) ? m_buffer[endIndex + 1] : '\0';
        const char next2 = (endIndex + 2 < m_buffer.size()) ? m_buffer[endIndex + 2] : '\0';

        // Handle CRLF as newline delimiter.
        const bool isCRLF = (c == '\r' && next == '\n');
        const bool isBackslashCRLF = (c == '\\' && next == '\r' && next2 == '\n');

        if (inLineComment) {
            if (c == '\n' || isCRLF) {
                inLineComment = false;
                // This newline is a delimiter. Stop token.
                sawDelimiter = true;
                break;
            }
            // Ignore comment chars.
            continue;
        }

        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                ++endIndex; // consume '/'
            }
            continue;
        }

        if (!inQuotes) {
            // Start comments (ignored)
            if (c == '#') {
                inLineComment = true;
                continue;
            }
            if (c == '/' && next == '*') {
                inBlockComment = true;
                ++endIndex; // consume '*'
                continue;
            }

            // Line continuation: backslash immediately before newline (LF or CRLF)
            if (c == '\\') {
                if (next == '\n') {
                    ++endIndex; // consume '\n'
                    continue;   // continue token
                }
                if (isBackslashCRLF) {
                    endIndex += 2; // consume '\r' + '\n'
                    continue;
                }
                // Otherwise treat as ordinary char in unquoted value
            }

            // Whitespace ignored outside quotes
            if (isSpaceOrTab(c)) {
                continue;
            }

            // Delimiters outside quotes/comments
            if (c == ';') {
                sawDelimiter = true;
                break;
            }
            if (c == '\n' || isCRLF) {
                sawDelimiter = true;
                break;
            }

            // Quoted string start (value may start with quotes)
            if (c == '"') {
                inQuotes = true;
                token.push_back(c);
                continue;
            }

            token.push_back(c);
            continue;
        }

        // inQuotes
        // Newline always terminates token; if quote wasn't closed, parseToken() will mark it invalid.
        if (c == '\n' || isCRLF) {
            sawDelimiter = true;
            break;
        }

        if (escape) {
            token.push_back(c);
            escape = false;
            continue;
        }

        if (c == '\\') {
            // Escape inside string (for escaped quotes etc.)
            token.push_back(c);
            escape = true;
            continue;
        }

        if (c == '"') {
            token.push_back(c);
            inQuotes = false;
            continue;
        }

        token.push_back(c);
    }

    // Determine how many bytes to consume from buffer:
    // from 0..endIndex plus delimiter bytes (if present) plus leading skipped part (i).
    size_t removeEnd = endIndex;
    if (sawDelimiter) {
        // Consume delimiter bytes too
        if (removeEnd < m_buffer.size() && m_buffer[removeEnd] == '\r') {
            ++removeEnd;
        }
        if (removeEnd < m_buffer.size() && m_buffer[removeEnd] == '\n') {
            ++removeEnd;
        }
        if (endIndex < m_buffer.size() && m_buffer[endIndex] == ';') {
            ++removeEnd;
        }
    } else {
        // No delimiter found.
        // If buffer is full - treat buffer as a token (consume all).
        if (m_buffer.size() >= m_buffer.capacity()) {
            removeEnd = m_buffer.size();
        } else {
            // Need more data for complete token
            return false;
        }
    }

    // Remove leading skipped bytes too (separators at start)
    // If we started at i > 0, we need to also drop that prefix.
    // We built token from position i..endIndex.
    const size_t toErase = (removeEnd > 0) ? removeEnd : 0;
    if (toErase >= m_buffer.size()) {
        m_buffer.clear();
    } else {
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + toErase);
    }

    return true;
}

bool PropertyParser::parseToken(const std::string& token) {
    m_isValid = false;
    m_propertyName.clear();
    m_propertyValue.clear();
    m_propertyMatch.clear();

    if (token.empty()) {
        return false;
    }

    // Validate quoted strings: if token contains quotes, they must be balanced (we keep quotes in token).
    // If tokenizer ended while still in quotes, it would have included opening quote without closing quote.
    // Detect: odd number of unescaped quotes.
    {
        bool esc = false;
        int quoteCount = 0;
        for (char c : token) {
            if (esc) {
                esc = false;
                continue;
            }
            if (c == '\\') {
                esc = true;
                continue;
            }
            if (c == '"') {
                ++quoteCount;
            }
        }
        if ((quoteCount % 2) != 0) {
            // Malformed token: unclosed string
            m_propertyMatch = token;
            if (m_caseInsensitive) {
                std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
            }
            return false;
        }
    }

    const size_t eqPos = token.find('=');
    if (eqPos == std::string::npos) {
        m_propertyMatch = token;
        if (m_caseInsensitive) {
            std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
        }
        return false;
    }

    if (eqPos == 0) {
        // Empty name
        m_propertyMatch = token;
        if (m_caseInsensitive) {
            std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
        }
        return false;
    }

    m_propertyName = token.substr(0, eqPos);
    m_propertyValue = token.substr(eqPos + 1);

    if (m_propertyName.empty()) {
        m_propertyMatch = token;
        if (m_caseInsensitive) {
            std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
        }
        return false;
    }

    if (m_caseInsensitive) {
        std::transform(m_propertyName.begin(), m_propertyName.end(), m_propertyName.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
    }

    // If value is quoted string - unescape \" and \\ and remove outer quotes.
    if (m_propertyValue.size() >= 2 && m_propertyValue.front() == '"' && m_propertyValue.back() == '"') {
        std::string unescaped;
        unescaped.reserve(m_propertyValue.size());

        bool esc = false;
        for (size_t i = 1; i + 1 < m_propertyValue.size(); ++i) {
            const char c = m_propertyValue[i];
            if (esc) {
                unescaped.push_back(c);
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else {
                unescaped.push_back(c);
            }
        }

        if (esc) {
            // Trailing backslash inside quotes -> malformed
            m_propertyMatch = token;
            if (m_caseInsensitive) {
                std::transform(m_propertyMatch.begin(), m_propertyMatch.end(), m_propertyMatch.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
            }
            m_propertyName.clear();
            m_propertyValue.clear();
            return false;
        }

        m_propertyValue = unescaped;
    }

    m_isValid = true;
    return true;
}

bool PropertyParser::parseNext() {
    // Reset result
    m_isValid = false;
    m_propertyName.clear();
    m_propertyValue.clear();
    m_propertyMatch.clear();

    std::string token;
    if (!extractNextToken(token)) {
        return false; // no complete token
    }

    (void)parseToken(token);
    return true; // token consumed even if invalid
}

bool PropertyParser::isValid() const { return m_isValid; }

const std::string& PropertyParser::getPropertyName() const { return m_propertyName; }

const std::string& PropertyParser::getPropertyValue() const { return m_propertyValue; }

const std::string& PropertyParser::getPropertyMatch() const { return m_propertyMatch; }

void PropertyParser::reset() {
    m_buffer.clear();
    m_propertyName.clear();
    m_propertyValue.clear();
    m_propertyMatch.clear();
    m_isValid = false;
}

bool PropertyParser::matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive) {
    std::string strCopy = str;
    std::string patternCopy = pattern;

    if (!caseSensitive) {
        std::transform(strCopy.begin(), strCopy.end(), strCopy.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        std::transform(patternCopy.begin(), patternCopy.end(), patternCopy.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
    }

    size_t strIndex = 0;
    size_t patternIndex = 0;
    size_t starIndex = std::string::npos;
    size_t matchIndex = 0;

    while (strIndex < strCopy.length()) {
        if (patternIndex < patternCopy.length() &&
            (patternCopy[patternIndex] == '?' || patternCopy[patternIndex] == strCopy[strIndex])) {
            strIndex++;
            patternIndex++;
        } else if (patternIndex < patternCopy.length() && patternCopy[patternIndex] == '*') {
            starIndex = patternIndex;
            matchIndex = strIndex;
            patternIndex++;
        } else if (starIndex != std::string::npos) {
            patternIndex = starIndex + 1;
            matchIndex++;
            strIndex = matchIndex;
        } else {
            return false;
        }
    }

    while (patternIndex < patternCopy.length() && patternCopy[patternIndex] == '*') {
        patternIndex++;
    }

    return patternIndex == patternCopy.length();
}

bool PropertyParser::findPropertyValue(const char* data, size_t length, const std::string& name,
                                      const char*& valueBegin, bool caseSensitive) {
    valueBegin = nullptr;
    if (!data || length == 0) {
        return false;
    }

    auto isDelimiter = [](char c) { return c == '\n' || c == ';'; };

    bool inQuotes = false;
    bool escape = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    size_t tokenStart = 0;

    auto skipSeparators = [&](size_t& pos) {
        while (pos < length && (data[pos] == '\n' || data[pos] == '\r' || data[pos] == ';')) {
            ++pos;
        }
    };

    size_t pos = 0;
    while (pos < length) {
        skipSeparators(pos);
        if (pos >= length) {
            break;
        }
        tokenStart = pos;

        // Find token end (similar rules as tokenizer, but we want raw pointer to value start).
        inQuotes = false;
        escape = false;
        inLineComment = false;
        inBlockComment = false;

        size_t tokenEnd = pos;
        for (; tokenEnd < length; ++tokenEnd) {
            const char c = data[tokenEnd];
            const char next = (tokenEnd + 1 < length) ? data[tokenEnd + 1] : '\0';
            const bool isCRLF = (c == '\r' && next == '\n');

            if (inLineComment) {
                if (c == '\n' || isCRLF) {
                    break;
                }
                continue;
            }
            if (inBlockComment) {
                if (c == '*' && next == '/') {
                    inBlockComment = false;
                    ++tokenEnd;
                }
                continue;
            }

            if (!inQuotes) {
                if (c == '#') {
                    inLineComment = true;
                    continue;
                }
                if (c == '/' && next == '*') {
                    inBlockComment = true;
                    ++tokenEnd;
                    continue;
                }
                if (c == '\\') {
                    if (next == '\n') {
                        ++tokenEnd;
                        continue;
                    }
                    if (isCRLF) {
                        tokenEnd += 1;
                        continue;
                    }
                }
                if (isDelimiter(c) || isCRLF || c == ';') {
                    break;
                }
                if (c == '"') {
                    inQuotes = true;
                }
                continue;
            } else {
                if (escape) {
                    escape = false;
                    continue;
                }
                if (c == '\\') {
                    escape = true;
                    continue;
                }
                if (c == '"') {
                    inQuotes = false;
                    continue;
                }
            }
        }

        // Now we have [tokenStart, tokenEnd) in original buffer (delimiter at tokenEnd or end).
        // Find '=' and compare names (ignoring spaces/tabs and comments is hard here; we approximate by skipping spaces/tabs).
        // We'll scan from tokenStart to tokenEnd to find '=' outside quotes/comments.
        bool localInQuotes = false;
        bool localEscape = false;
        bool localLineComment = false;
        bool localBlockComment = false;

        size_t eqPos = std::string::npos;

        for (size_t i = tokenStart; i < tokenEnd; ++i) {
            const char c = data[i];
            const char next = (i + 1 < tokenEnd) ? data[i + 1] : '\0';

            if (localLineComment) {
                break;
            }
            if (localBlockComment) {
                if (c == '*' && next == '/') {
                    localBlockComment = false;
                    ++i;
                }
                continue;
            }

            if (!localInQuotes) {
                if (c == '#') {
                    localLineComment = true;
                    break;
                }
                if (c == '/' && next == '*') {
                    localBlockComment = true;
                    ++i;
                    continue;
                }
                if (c == '"') {
                    localInQuotes = true;
                    continue;
                }
                if (c == '=') {
                    eqPos = i;
                    break;
                }
            } else {
                if (localEscape) {
                    localEscape = false;
                    continue;
                }
                if (c == '\\') {
                    localEscape = true;
                    continue;
                }
                if (c == '"') {
                    localInQuotes = false;
                    continue;
                }
            }
        }

        if (eqPos != std::string::npos && eqPos > tokenStart) {
            // Extract name portion: [tokenStart, eqPos)
            std::string key;
            for (size_t i = tokenStart; i < eqPos; ++i) {
                const char c = data[i];
                if (isSpaceOrTab(c)) {
                    continue;
                }
                if (c == '/' && (i + 1 < eqPos) && data[i + 1] == '*') {
                    // stop on comment start inside key
                    break;
                }
                if (c == '#') {
                    break;
                }
                key.push_back(c);
            }

            if (equalsNameImpl(key, name, caseSensitive)) {
                // value begin is first non-space/tab after '='
                size_t vb = eqPos + 1;
                while (vb < tokenEnd && isSpaceOrTab(data[vb])) {
                    ++vb;
                }
                valueBegin = data + vb;
                return true;
            }
        }

        // Move pos to delimiter / next token start
        pos = tokenEnd;
        // Consume CRLF as delimiter
        if (pos < length && data[pos] == '\r' && (pos + 1 < length) && data[pos + 1] == '\n') {
            pos += 2;
        } else if (pos < length && (data[pos] == '\n' || data[pos] == ';')) {
            pos += 1;
        }
    }

    return false;
}
