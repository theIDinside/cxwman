#pragma once

#include <functional>
#include <map>

namespace cx::config
{
    struct Binding {
        std::size_t id;
    };

    struct KeyConfiguration {
        std::function<auto()->void> action;
    };
} // namespace cx::config
