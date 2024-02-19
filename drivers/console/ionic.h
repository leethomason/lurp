#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace ionic {

enum class Color : uint8_t {
    black,
    gray,
    white,

    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    default = gray
};

struct Format {
    Color color = Color::default;
    uint8_t vPad = 1;
    uint8_t hPad = 1;
    char corner = '+';
    char horizontal = '-';
    char vertical = '|';
};

enum class BreakRules {
    newline,
    doubleNewline
};

class Ionic {
public:
    static void initConsole();
    static std::vector<std::string_view> wordWrap(const std::string_view& text, int width, BreakRules rules);

    void addRow(const std::vector<std::string_view>& row);
};

}  // namespace ionic
