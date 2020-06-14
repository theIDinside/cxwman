#pragma once
#include <fmt/core.h>


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
    template <typename... Args>
    void println(const char *format_string, Args... args)
    {
        using fmt_str_t = decltype(format_string);
        fmt::print(std::forward<fmt_str_t>(format_string), std::forward<Args>(args)...);
        fmt::print("\n");
    }
} // namespace cx

#define LOG(message, ...) cx::println(message, __VA_ARGS__)

#ifdef DEBUGGING
#define DBGLOG(message, ...) LOG(message, __VA_ARGS__)
#else
#define DBGLOG(message, ...)
#endif