#pragma once
#include <fmt/core.h>
#include <string_view>
#include <cstring>

// These re-defines are just to more clearly state intent
#define local_persist static
#define global static

namespace cx
{

    using uint = unsigned int;
    using u16 = unsigned short;
    using u32 = uint32_t;
    using usize = unsigned long;
    using isize = signed long;
    using f32 = float;
    using f64 = double;

    template <std::size_t N, typename... Args>
    constexpr void println(const char (&format_str)[N], Args... args)
    {
        using fmt_str_t = decltype(format_str);
        fmt::print(std::forward<fmt_str_t>(format_str), std::forward<Args>(args)...);
        fmt::print("\n");
    }
} // namespace cx

#define LOG(message, ...) cx::println(message, __VA_ARGS__)
#define NOLOG()

#ifdef DEBUGGING
#define DBGLOG(message, ...) LOG(message, __VA_ARGS__)
#else
#define DBGLOG(message, ...) NOLOG()
#endif