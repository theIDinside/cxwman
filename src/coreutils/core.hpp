#pragma once
#include <fmt/core.h>

namespace cx
{

    using uint = unsigned int;
    using u16 = unsigned short;
    using u32 = uint32_t;
    using usize = unsigned long;
    using isize = signed long;
    using f32 = float;
    using f64 = double;
    template <typename... Args>
    void println(const char *format_string, Args... args)
    {
        using fmt_str_t = decltype(format_string);
        fmt::print(std::forward<fmt_str_t>(format_string), std::forward<Args>(args)...);
        fmt::print("\n");
    }
} // namespace cx
