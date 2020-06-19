//
// Created by cx on 2020-06-19.
//
#include <coreutils/core.hpp>
#include <xcom/xinit.hpp>

namespace cx::xinit
{

    namespace debug
    {
        [[maybe_unused]] void print_modifiers(std::uint32_t mask)
        {
            constexpr const char* MODIFIERS[] = {"Shift", "Lock",    "Ctrl",    "Alt",     "Mod2",    "Mod3",   "Mod4",
                                                 "Mod5",  "Button1", "Button2", "Button3", "Button4", "Button5"};

            for(auto modifier = MODIFIERS; mask; mask >>= 1U, ++modifier) {
                if(mask & 1U) {
                    fmt::print("Modifier mask: {} - Value: {}\t", *modifier, mask);
                }
            }
            fmt::print("\n");
        }
    } // namespace debug

    void setup_redirection_of_map_requests(XCBConn* conn, XCBWindow window)
    {

        auto value_to_set = XCB_CW_EVENT_MASK;
        cx::u32 values[2];
        values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
        // values[0] = ROOT_EVENT_MASK;
        auto attr_req_cookie = xcb_change_window_attributes_checked(conn, window, value_to_set, values);
        if(auto err = xcb_request_check(conn, attr_req_cookie); err) {
            cx::println("Could not set Substructure Redirect for our root window. This window manager will not function");
            std::abort();
        }

        auto root_geometry = xcb_get_geometry(conn, window);
        auto g_reply = xcb_get_geometry_reply(conn, root_geometry, nullptr);
        if(!g_reply) {
            cx::println("Failed to get geometry of root window. Exiting");
            std::abort();
        }
        DBGLOG("Root geometry: Width x Height = {} x {}", g_reply->width, g_reply->height);
    }
    void setup_mouse_button_request_handling(XCBConn* conn, XCBWindow window)
    {
        auto mouse_button = 1; // left mouse button, 2 middle, 3 right, MouseModMask is alt + mouse click

        auto PRESS_AND_RELEASE_MASK = (cx::u32)XCB_EVENT_MASK_BUTTON_PRESS | (cx::u32)XCB_EVENT_MASK_BUTTON_RELEASE |
                                      (cx::u32)XCB_EVENT_MASK_ENTER_WINDOW | (cx::u32)XCB_EVENT_MASK_LEAVE_WINDOW;
        while(mouse_button < 4) {
            auto ck = xcb_grab_button_checked(conn, 0, window, PRESS_AND_RELEASE_MASK, CXGRABMODE, CXGRABMODE, window, XCB_NONE, mouse_button,
                                              XCB_MOD_MASK_ANY);
            if(auto err = xcb_request_check(conn, ck); err) {
                cx::println("Could not set up handling of mouse button clicks for button {}", mouse_button);
            }
            mouse_button++;
        }
    }

    constexpr auto get_key_mod_bindings()
    {
        namespace KM = xcb_key_masks;
        return make_array(std::make_pair(KM::SUPER_SHIFT, XK_F4), std::make_pair(KM::SUPER_SHIFT, XK_R), std::make_pair(KM::SUPER_SHIFT, XK_Left),
                          std::make_pair(KM::SUPER_SHIFT, XK_Right), std::make_pair(KM::SUPER_SHIFT, XK_Up),
                          std::make_pair(KM::SUPER_SHIFT, XK_Down));
    }

    void setup_key_press_listening(XCBConn* conn, XCBWindow root)
    {
        namespace KM = xcb_key_masks;
        auto keysyms_data = xcb_key_symbols_alloc(conn);
        constexpr auto bindings = get_key_mod_bindings();

        std::for_each(std::begin(bindings), std::end(bindings), [&](auto& binding) {
            auto& [modifier, keysym] = binding;
            auto key_codes = xcb_key_symbols_get_keycode(keysyms_data, keysym);
            if(key_codes) {
                auto pos = 0;
                while(key_codes[pos] != XCB_NO_SYMBOL) {
                    xcb_grab_key(conn, 1, root, modifier, key_codes[pos], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
                    pos++;
                }
            }
            free(key_codes);
        });
        free(keysyms_data);
    }

    auto get_client_wm_name(XCBConn* c, xcb_window_t window) -> std::optional<std::string>
    {
        auto prop_cookie = xcb_get_property(c, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 45);
        if(auto prop_reply = xcb_get_property_reply(c, prop_cookie, nullptr); prop_reply) {
            auto str_length = xcb_get_property_value_length(prop_reply);
            if(str_length == 0) {
                DBGLOG("Failed to get WM_NAME due to length being == {}", str_length);
                free(prop_reply);
                return {};
            } else {
                auto start = (const char*)xcb_get_property_value(prop_reply);
                std::string_view s{start, (std::size_t)str_length};
                DBGLOG("WM_NAME Length: {} Data: [{}]", str_length, s);
                auto result = std::string{s};
                free(prop_reply);
                return result;
            }
        } else {
            DBGLOG("Failed to request WM_NAME for window {}", window);
            free(prop_reply);
            return {};
        }
    }
} // namespace cx::xinit