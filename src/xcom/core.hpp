#pragma once

#include <array>

// N.B! ALL "defines" that redefine XCB calls, are of the "checked" kind

/// Returns std::array<xcb_void_cookie_t, 2>, where i:0 is the frame cookie, and i:1 is the client cookie
#define CONFIG_CX_WINDOW(window, fprops, fvals, cprops, cvals)                                                                                       \
    std::array<xcb_void_cookie_t, 2>                                                                                                                 \
    {                                                                                                                                                \
        xcb_configure_window_checked(get_conn(), window.frame_id, fprops, fvals),                                                                    \
            xcb_configure_window_checked(get_conn(), window.client_id, cprops, cvals)                                                                \
    }