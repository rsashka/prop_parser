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

// Определение callback-функции для обработки результатов парсинга
void parseCallback(void* data, const PropertyParser& parser) {
    if (parser.isValid()) {
        std::cout << "Найдено свойство: имя='" << parser.getPropertyName() 
                  << "', значение='" << parser.getPropertyValue() << "'" << std::endl;
    } else {
        std::cout << "Некорректная строка: '" << parser.getPropertyMatch() << "'" << std::endl;
    }
}

// Создание парсера с регистрозависимым сравнением (по умолчанию)
PropertyParser parser(false);

// Парсинг валидного свойства с использованием callback-функции
parser.feedAndParse("name=value\n", 11, parseCallback, nullptr);

// Парсинг строки без разделителя с использованием callback-функции
parser.feedAndParse("invalid_string\n", 15, parseCallback, nullptr);

// Создание парсера с регистронезависимым сравнением
PropertyParser caseInsensitiveParser(true);

// Парсинг свойства с регистронезависимым сравнением
caseInsensitiveParser.feedAndParse("Name=Value\n", 11, parseCallback, nullptr);
```

### Пример использования сопоставления по шаблону

```cpp
#include "PropertyParser.h"

// Регистрозависимое сопоставление (по умолчанию)
bool match1 = PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"); // true
bool match2 = PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*"); // false

// Регистронезависимое сопоставление
bool match3 = PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*", false); // true
bool match4 = PropertyParser::matchesPattern("COM.EXAMPLE.MYTEST", "com.example.*", false); // true
```

## Методы класса PropertyParser

- `PropertyParser(bool caseInsensitive = false)` - Конструктор с возможностью установки режима регистронезависимого сравнения
- `void feedAndParse(const char* data, size_t length, PropertyParserCallback callback = nullptr, void* callbackData = nullptr)` - Передача данных для парсинга и немедленная обработка с вызовом callback-функции
- `bool parseNext()` - Парсинг следующего токена (для внутреннего использования)
- `bool isValid() const` - Проверка валидности последнего разобранного свойства
- `const std::string& getPropertyName() const` - Получение имени свойства
- `const std::string& getPropertyValue() const` - Получение значения свойства
- `const std::string& getPropertyMatch() const` - Получение строки, не содержащей разделитель ключ-значение
- `void reset()` - Сброс состояния парсера
- `static bool matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive = true)` - Проверка соответствия строки шаблону с возможностью установки режима чувствительности к регистру

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
