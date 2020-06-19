#pragma once

#include <coreutils/core.hpp>

#include <functional>
#include <map>
#include <xcb/xproto.h>

namespace cx::config
{
    struct Binding {
        std::size_t id;
    };

    struct KeyConfiguration {
        xcb_keysym_t symbol;
        cx::u16 modifier;
        std::function<auto()->void> action;
        friend bool operator<(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
    };
    bool operator<(const KeyConfiguration& lhs, const KeyConfiguration& rhs);

    struct KeyMap {
        std::map<KeyConfiguration, std::function<auto()->void>> key_map;
    };
} // namespace cx::config
