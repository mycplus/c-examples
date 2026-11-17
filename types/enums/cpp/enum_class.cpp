// enum_class: how C++ changes enums. A plain enum works as in C (and the
// enum keyword is optional when declaring a variable); an enum class adds
// scoped names, no implicit conversion to int, and a chosen underlying type.
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>

enum Weekday { monday, tuesday, wednesday };            // plain, as in C

enum class Signal : std::uint8_t { red, amber, green };  // scoped, one byte
enum class Light : std::uint8_t { off, red, green };      // "red" again: no clash

constexpr std::string_view name(Signal s)
{
    switch (s) {
    case Signal::red:   return "red";
    case Signal::amber: return "amber";
    case Signal::green: return "green";
    }
    return "invalid";
}

constexpr Signal next(Signal s)
{
    switch (s) {
    case Signal::red:   return Signal::green;
    case Signal::green: return Signal::amber;
    case Signal::amber: return Signal::red;
    }
    return Signal::red;
}

int main()
{
    Weekday day = tuesday;          // no "enum" keyword needed in C++
    std::cout << "tuesday = " << day << '\n';

    Signal s = Signal::red;
    for (int i = 0; i < 4; ++i) {
        std::cout << name(s) << " (" << static_cast<int>(s) << ")\n";
        s = next(s);
    }

    static_assert(sizeof(Signal) == 1, "the underlying type is std::uint8_t");
    static_assert(std::is_same_v<std::underlying_type_t<Signal>, std::uint8_t>);
    std::cout << "sizeof(Signal) = " << sizeof(Signal) << ", sizeof(Weekday) = " << sizeof(Weekday) << '\n';
    std::cout << "Signal::red = " << static_cast<int>(Signal::red) << ", Light::red = "
              << static_cast<int>(Light::red) << ": same name, two scopes, no clash\n";
}
