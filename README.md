# Property Parser

Это библиотека для парсинга свойств в формате `ключ=значение`.

## Описание

Библиотека предоставляет класс `PropertyParser` для разбора строк в формате свойств. Каждое свойство записывается в виде `ключ=значение`, где:
- `ключ` - имя свойства
- `значение` - значение свойства

Если при парсинге входных данных отсутствует разделитель ключ-значение (знак равенства), то такая строка сохраняется в поле `propertyMatch`, которое можно получить с помощью метода `getPropertyMatch()`.

Поддерживаются различные форматы окончания строк: LF (\n) и CRLF (\r\n).

## Использование

```cpp
#include "PropertyParser.h"

PropertyParser parser;
// Парсинг валидного свойства
parser.feed("name=value\n", 11);
if (parser.parseNext()) {
    if (parser.isValid()) {
        std::string name = parser.getPropertyName();
        std::string value = parser.getPropertyValue();
        // Обработка свойства
    }
}

// Парсинг строки без разделителя
parser.feed("invalid_string\n", 14);
if (parser.parseNext()) {
    if (!parser.isValid()) {
        std::string match = parser.getPropertyMatch();
        // Обработка строки без разделителя
    }
}
```

## Методы класса PropertyParser

- `PropertyParser()` - Конструктор
- `void feed(const std::vector<char>& data)` - Передача данных для парсинга
- `void feed(const char* data, size_t length)` - Передача данных для парсинга
- `bool parseNext()` - Парсинг следующего токена
- `bool isValid() const` - Проверка валидности последнего разобранного свойства
- `const std::string& getPropertyName() const` - Получение имени свойства
- `const std::string& getPropertyValue() const` - Получение значения свойства
- `const std::string& getPropertyMatch() const` - Получение строки, не содержащей разделитель ключ-значение
- `void reset()` - Сброс состояния парсера
- `static bool matchesPattern(const std::string& str, const std::string& pattern)` - Проверка соответствия строки шаблону

## Шаблоны

Метод `matchesPattern` позволяет проверять соответствие строки шаблону в формате, аналогичном используемому в GWT:
- `*` - соответствует любому количеству символов (включая ноль)
- `?` - соответствует ровно одному символу

Примеры:
- `"com.example.*"` - соответствует всем строкам, начинающимся с "com.example."
- `"*Test"` - соответствует всем строкам, заканчивающимся на "Test"
- `"My?est"` - соответствует строке "MyTest"

## Сборка

Для сборки проекта используется CMake:

```bash
mkdir build
cd build
cmake ..
make
```

## Тестирование

Проект включает набор unit-тестов, реализованных с использованием Google Test:

```bash
cd build
./PropertyParserTests
