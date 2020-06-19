#pragma once

#include <coreutils/core.hpp>

#include <algorithm>
#include <functional>
#include <map>

#include <xcb/xproto.h>

namespace cx::config
{
    struct KeyConfiguration {
        KeyConfiguration(xcb_keysym_t symbol, cx::u16 modifier);
        xcb_keysym_t symbol;
        cx::u16 modifier;
        friend bool operator<(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
        friend bool operator==(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
        friend bool operator>(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
    };
    bool operator<(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
    bool operator==(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
    bool operator>(const KeyConfiguration& lhs, const KeyConfiguration& rhs);
} // namespace cx::config
