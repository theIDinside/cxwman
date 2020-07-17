//
// Created by cx on 2020-06-19.
//

#pragma once
#include <algorithm>
#include <memory>
#include <string_view>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <optional>
#include <xcom/constants.hpp>


// System X11
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include "raii.hpp"



#define CXGRABMODE XCB_GRAB_MODE_ASYNC

namespace cx {
    using u32 = std::uint32_t;
}

namespace cx::x11
{

    namespace debug
    {
        [[maybe_unused]] void print_modifiers(std::uint32_t mask);
    }

    static constexpr std::string_view atom_names[]{"_NET_SUPPORTED",
                                                   "_NET_SUPPORTING_WM_CHECK",
                                                   "_NET_WM_NAME",
                                                   "_NET_WM_VISIBLE_NAME",
                                                   "_NET_WM_MOVERESIZE",
                                                   "_NET_WM_STATE_STICKY",
                                                   "_NET_WM_STATE_FULLSCREEN",
                                                   "_NET_WM_STATE_DEMANDS_ATTENTION",
                                                   "_NET_WM_STATE_MODAL",
                                                   "_NET_WM_STATE_HIDDEN",
                                                   "_NET_WM_STATE_FOCUSED",
                                                   "_NET_WM_STATE",
                                                   "_NET_WM_WINDOW_TYPE",
                                                   "_NET_WM_WINDOW_TYPE_NORMAL",
                                                   "_NET_WM_WINDOW_TYPE_DOCK",
                                                   "_NET_WM_WINDOW_TYPE_DIALOG",
                                                   "_NET_WM_WINDOW_TYPE_UTILITY",
                                                   "_NET_WM_WINDOW_TYPE_TOOLBAR",
                                                   "_NET_WM_WINDOW_TYPE_SPLASH",
                                                   "_NET_WM_WINDOW_TYPE_MENU",
                                                   "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
                                                   "_NET_WM_WINDOW_TYPE_POPUP_MENU",
                                                   "_NET_WM_WINDOW_TYPE_TOOLTIP",
                                                   "_NET_WM_WINDOW_TYPE_NOTIFICATION",
                                                   "_NET_WM_DESKTOP",
                                                   "_NET_WM_STRUT_PARTIAL",
                                                   "_NET_CLIENT_LIST",
                                                   "_NET_CLIENT_LIST_STACKING",
                                                   "_NET_CURRENT_DESKTOP",
                                                   "_NET_NUMBER_OF_DESKTOPS",
                                                   "_NET_DESKTOP_NAMES",
                                                   "_NET_DESKTOP_VIEWPORT",
                                                   "_NET_ACTIVE_WINDOW",
                                                   "_NET_CLOSE_WINDOW",
                                                   "_NET_MOVERESIZE_WINDOW"};

    using XCBConn = xcb_connection_t;
    using XCBScreen = xcb_screen_t;
    using XCBDrawable = xcb_drawable_t;
    using XCBWindow = xcb_window_t;

    struct XInternals {
        XInternals(XCBConn* c, XCBScreen* scr, XCBDrawable rd, XCBWindow w, XCBWindow ewmh, xcb_key_symbols_t* symbols, int fd)
            : c(c), screen(scr), root_drawable(rd), root_window(w), ewmh_window(ewmh), keysyms(symbols), xcb_file_descriptor(fd)
        {
        }
        ~XInternals() { free(keysyms); }
        XCBConn* c;
        XCBScreen* screen;
        XCBDrawable root_drawable;
        XCBWindow root_window;
        XCBWindow ewmh_window;
        xcb_key_symbols_t* keysyms;
        int xcb_file_descriptor;
    };

    // Yanked from the define in i3, to be used for our root window as well
    constexpr auto ROOT_EVENT_MASK =
        (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION |
         XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW);

    constexpr auto FRAME_EVENT_MASK =
        (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE |
         XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW);

    // TODO(simon): These are utility functions, that serve *only* as wrappers,
    // so they become a mental model of what needs to be done, to set up a simplistic window manager

    /// This setups of what X calls SubstructureRedirection. This essentially means,
    // that all map requests will get sent to "us" (the window manager) so that we get
    // to specify how we handle them, for example, in a tiling window manager example,
    // we would check how we layout "our" windows, and then find a suitable place for the
    // soon-to-be mapped window. If we can't find one, in a tiling wm, we split a sub-space somewhere,
    // where a window exists, and let the new window take half that space, for example
    void setup_redirection_of_map_requests(XCBConn* conn, XCBWindow window);

    // This setups up, so that we tell the X-server, that we want to be notified of Mouse Button
    // events. This way, we override what needs to be done, so that we can hi-jack button presses in client windows
    void setup_mouse_button_request_handling(XCBConn* conn, XCBWindow window);

    void setup_key_press_listening(XCBConn* conn, XCBWindow root);

    // TODO(implement): Get all 35 atoms that i3 support, and filter out the ones we probably won't need
    template<typename StrView, std::size_t N>
    auto get_supported_atoms(XCBConn* c, const StrView (&names)[N] = atom_names) -> std::array<xcb_atom_t, N>
    {
        std::array<xcb_atom_t, N> atoms{};
        std::transform(std::begin(names), std::begin(names) + N, std::begin(atoms), [c](auto str) {
            auto cookie = xcb_intern_atom(c, 0, str.size(), str.data());
            cx::x11::X11Resource resource = xcb_intern_atom_reply(c, cookie, nullptr);
            return resource->atom;
        });
        return atoms;
    }

    auto get_client_wm_name(XCBConn* c, xcb_window_t window) -> std::optional<std::string>;
    // NOTE: On linux type xlsfonts to list X font names, that can be used as font_name
    auto get_font_gc(XCBConn* c, XCBWindow window, u32 fg_color, u32 bg_color, std::string_view font_name) -> std::optional<xcb_gcontext_t>;
} // namespace cx::x11