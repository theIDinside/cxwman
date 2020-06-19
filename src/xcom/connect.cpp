#include <coreutils/core.hpp>
#include <xcom/connect.hpp>

#include <cassert>

#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>

#include <algorithm>
#include <memory>

#define CXGRABMODE XCB_GRAB_MODE_ASYNC
namespace cx
{
    template<typename StrT>
    constexpr auto name_len(StrT str)
    {
        return std::strlen(str);
    }

#define x_replace_str_prop(c, window, atom, string)                                                                                                  \
    xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, atom, XCB_ATOM_STRING, 8, name_len(string), string)

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
    // TODO(implement): Get all 35 atoms that i3 support, and filter out the ones we probably won't need
    template<typename StrView, std::size_t N>
    auto get_supported_atoms(XCBConn* c, const StrView (&names)[N]) -> std::array<xcb_atom_t, N>
    {
        std::array<xcb_atom_t, N> atoms{};
        std::transform(std::begin(names), std::begin(names) + N, std::begin(atoms), [c](auto str) {
            auto cookie = xcb_intern_atom(c, 0, str.size(), str.data());
            return xcb_intern_atom_reply(c, cookie, nullptr)->atom;
        });
        return atoms;
    }
    auto get_wm_name(XCBConn* c, xcb_window_t window) -> std::optional<std::string>
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

    void Manager::setup()
    {
        // TODO(implement): grab pre-existing windows, reparent them properly and manage them
        xcb_grab_server(get_conn());
        setup_root_workspace_container();
        xcb_ungrab_server(get_conn());
    }

    void Manager::setup_root_workspace_container()
    {
        // auto win_geom = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), get_root()), nullptr);
        add_workspace("Workspace 1", 0);
        this->focused_ws = m_workspaces[0].get();
    }

    // Fixme: Currently pressing on client menus and specific buttons doesn't work. This has got to have to be because we set some flags
    // wrong, (or possibly not at all). Perhaps look to i3 for help?

    std::unique_ptr<Manager> Manager::initialize()
    {
        int screen_number;
        XCBScreen* screen = nullptr;
        XCBDrawable root_drawable;
        auto c = xcb_connect(nullptr, &screen_number);
        if(xcb_connection_has_error(c)) {
            throw std::runtime_error{"XCB Error: failed trying to connect to X-server"};
        }

        auto screen_iter = xcb_setup_roots_iterator(xcb_get_setup(c));

        // Find our screen
        for(auto i = 0; i < screen_number; ++i)
            xcb_screen_next(&screen_iter);

        screen = screen_iter.data;

        if(!screen) {
            xcb_disconnect(c);
            throw std::runtime_error{"XCB Error: Failed to find screen. Disconnecting from X-server."};
        }

        root_drawable = screen->root;
        XCBWindow window = screen->root;

        // Set this in pre-processor variable in CMake, to run this code
        DBGLOG("Screen size {} x {} pixels. Root window: {}", screen->width_in_pixels, screen->height_in_pixels, root_drawable);
        setup_mouse_button_request_handling(c, window);
        setup_redirection_of_map_requests(c, window);
        setup_key_press_listening(c, window);
        // TODO: grab keys & set up keysymbols and configs
        auto ewmh_window = xcb_generate_id(c);
        xcb_create_window(c, XCB_COPY_FROM_PARENT, ewmh_window, window, -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT,
                          XCB_CW_OVERRIDE_REDIRECT, (uint32_t[]){1});
        auto atoms = get_supported_atoms(c, atom_names);

        auto support_check_cookie = xcb_intern_atom(c, 0, name_len("_NET_SUPPORTING_WM_CHECK"), "_NET_SUPPORTING_WM_CHECK");
        auto wm_name_cookie = xcb_intern_atom(c, 0, name_len("_NET_WM_NAME"), "_NET_WM_NAME");

        auto support_r = xcb_intern_atom_reply(c, support_check_cookie, nullptr);
        auto wm_name_r = xcb_intern_atom_reply(c, wm_name_cookie, nullptr);

        auto get_atom = [c](auto atom_name) -> xcb_atom_t {
            auto cookie = xcb_intern_atom(c, 0, strlen(atom_name), atom_name);
            /* ... do other work here if possible ... */
            if(auto reply = xcb_intern_atom_reply(c, cookie, nullptr); reply) {
                DBGLOG("The {} atom has ID {}", atom_name, reply->atom);
                auto atom = reply->atom;
                free(reply);
                return atom;
            } else {
                DBGLOG("Failed to get atom for: {}. In debug mode we crash here.", atom_name);
                exit(-1);
            }
        };
        auto a_wm = get_atom("_NET_WM_NAME");
        auto a_supp = get_atom("_NET_SUPPORTING_WM_CHECK");

        std::vector<xcb_void_cookie_t> cookies{};

        cookies.push_back(xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, ewmh_window, a_supp, XCB_ATOM_WINDOW, 32, 1, &ewmh_window));
        cookies.push_back(x_replace_str_prop(c, ewmh_window, a_wm, "CXWMAN"));
        cookies.push_back(xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, a_supp, XCB_ATOM_WINDOW, 32, 1, &ewmh_window));
        cookies.push_back(x_replace_str_prop(c, window, a_wm, "CXWMAN"));

        // TODO(implement): Set the supported atoms by calling change prop with _NET_SUPPORTED as the... property, XCB_ATOM_ATOM as the
        // type, and then the atoms as the data
        auto err_found = false;
        for(const auto& cookie : cookies) {
            if(auto err = xcb_request_check(c, cookie); err) {
                cx::println("Failed to change property of EWMH Window or Root window");
                err_found = true;
            }
        }
#ifdef DEBUGGING
        if(!err_found)
            DBGLOG("Succesfully changed properties SUPPORTING_WM_CHECK and WM_NAME of ewmh window {} and root window {}", ewmh_window, window);
#endif
        cookies.clear();
        cookies.push_back(xcb_map_window_checked(c, ewmh_window));
        cookies.push_back(xcb_configure_window_checked(c, ewmh_window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){XCB_STACK_MODE_BELOW}));
        err_found = false;
        for(const auto& cookie : cookies) {
            if(auto err = xcb_request_check(c, cookie); err) {
                cx::println("Failed to map/configure ewmh window");
                err_found = true;
            }
        }
        auto symbols = xcb_key_symbols_alloc(c);
        return std::unique_ptr<Manager>(new Manager{c, screen, root_drawable, window, ewmh_window, symbols});
    }

    // Private constructor called via public interface function Manager::initialize()
    Manager::Manager(XCBConn* connection, XCBScreen* screen, XCBDrawable root_drawable, XCBWindow root_window, XCBWindow ewmh_window,
                     xcb_key_symbols_t* symbols) noexcept
        : x_detail{connection, screen, root_drawable, root_window, ewmh_window, symbols},
          m_running(false), client_to_frame_mapping{}, frame_to_client_mapping{}, focused_ws(nullptr), actions{}, m_workspaces{}, m_key_bindings{}
    {
        // TODO: Set up key-combo-configurations with bindings like this? Or unnecessarily complex?
        actions[27] = [this]() {
            this->focused_ws->rotate_focus_pair();
            this->focused_ws->display_update(get_conn());
        };
    }

    [[nodiscard]] inline auto Manager::get_conn() const -> XCBConn* { return x_detail.c; }

    [[nodiscard]] inline auto Manager::get_root() const -> XCBWindow { return x_detail.screen->root; }

    [[nodiscard]] inline auto Manager::get_screen() const -> XCBScreen* { return x_detail.screen; }

    // TODO(EWMHints): Grab EWM hints & set up supported hints
    auto Manager::handle_map_request(xcb_map_request_event_t* evt) -> void
    {
        local_persist int map_requests{};
        // DBGLOG("Handling map request of window with id {}. #{}", evt->window, map_requests++);
        frame_window(evt->window);
        xcb_map_window(get_conn(), evt->window);
    }

    auto Manager::handle_unmap_request(xcb_unmap_window_request_t* event) -> void
    {
        // DBGLOG("Handle unmap request for {}", event->window);
        auto window_container = focused_ws->find_window(event->window);

        if(window_container) {
            cx::println("Found window to unmap. Not yet implemented!");
            // unframe_window(*window);
            auto window = *window_container.value()->client;
            focused_ws->unregister_window(*window_container);
            unframe_window(window);
            focused_ws->display_update(get_conn());
        }
    }

    auto Manager::handle_config_request(xcb_configure_request_event_t* e) -> void
    {
        cx::println("\tcfg request by: {} sibling: {}", e->window, e->sibling);
        xcb_generic_error_t* err;
        uint32_t values[7], mask = 0, i = 0;
        if(client_to_frame_mapping.count(e->parent) == 1) {
            xcb_window_t frame = client_to_frame_mapping[e->window];
            cx::println("Handle cfg for frame {} of client {}", frame, e->window);
            if(e->value_mask & XCB_CONFIG_WINDOW_X) {
                mask |= XCB_CONFIG_WINDOW_X;
                values[i++] = e->x;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
                mask |= XCB_CONFIG_WINDOW_Y;
                // Modify y to be within limits
                values[i++] = e->y;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_WIDTH;
                values[i++] = e->width;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                mask |= XCB_CONFIG_WINDOW_HEIGHT;
                values[i++] = e->height;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
                values[i++] = e->border_width;
            }
            err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), frame, mask, values));
            if(err) {
                cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
            }
        }
        mask = 0;
        i = 0;
        if(e->value_mask & XCB_CONFIG_WINDOW_X) {
            mask |= XCB_CONFIG_WINDOW_X;
            values[i++] = e->x;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
            mask |= XCB_CONFIG_WINDOW_Y;
            values[i++] = e->y;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_WIDTH;
            values[i++] = e->width;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
            values[i++] = e->height;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
            values[i++] = e->border_width;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
            mask |= XCB_CONFIG_WINDOW_SIBLING;
            values[i++] = e->sibling;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
            mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            values[i++] = e->stack_mode;
        }
        err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), e->window, mask, values));
        if(err) {
            cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
        }
    }

    auto Manager::handle_key_press(xcb_key_press_event_t* event) -> void
    {
        namespace xkm = xcb_key_masks;
        auto ksym = xcb_key_press_lookup_keysym(x_detail.keysyms, event, 0);
        // debug::print_modifiers(event->state);
        fmt::print("Key code {} - KeySym: {}?\n", event->detail, ksym);
        if(ksym == XK_F4 && event->state == xkm::SUPER_SHIFT) {
            focused_ws->rotate_focus_layout();
            focused_ws->display_update(get_conn());
        } else if(ksym == XK_r && event->state == xkm::SUPER_SHIFT) {
            // This is really odd. When state is SUPER+SHIFT + key, it doesn't register as XK_R but XK_r
            actions[event->detail]();
        } else if(ksym == XK_Left) {
            focused_ws->move_focused_left();
            focused_ws->display_update(get_conn());
        } else if(ksym == XK_Right) {
            focused_ws->move_focused_right();
            focused_ws->display_update(get_conn());
        }
    }
    // Fixme: Make sure client specific functionality isn't lost after re-parenting (such as client menu's should continue working)
    // Fixme: Does not showing client popup/dropdown menus have to do with not mapping their windows? (test basic_wm and see)
    auto Manager::frame_window(XCBWindow window, geom::Geometry geometry, bool created_before_wm) -> void
    {
        std::array<xcb_void_cookie_t, 4> cookies{};
        constexpr auto border_width = 3;
        constexpr auto border_color = 0xff12ab;
        constexpr auto bg_color = 0x010101;

        if(client_to_frame_mapping.count(window)) {
            DBGLOG("Framing an already framed window (id: {}) is unhandled behavior. Returning early from framing function.", window);
            return;
        }

        auto win_attr = xcb_get_window_attributes_reply(get_conn(), xcb_get_window_attributes(get_conn(), window), nullptr);
        auto client_geometry = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), window), nullptr);
        if(created_before_wm) {
            cx::println("Window was created before WM.");
            if(win_attr->override_redirect || win_attr->map_state != XCB_MAP_STATE_VIEWABLE)
                return;
        }

        // construct frame
        auto frame_id = xcb_generate_id(get_conn());

        // Where that is, depends on how we split subspaces, layout strategies etc. For now, we just put it at 0, 0

        uint32_t mask = 0;
        uint32_t values[2];
        /* see include/xcb.h for the FRAME_EVENT_MASK */
        mask = XCB_CW_BORDER_PIXEL;
        values[0] = border_color;
        mask |= XCB_CW_EVENT_MASK;
        // FIXME(Client menus): Should work, but currently doesn't. Perhaps some flag wrong set, or not set at all?
        values[1] = (cx::u32)XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;

        cookies[0] = xcb_create_window_checked(get_conn(), 0, frame_id, get_root(), 0, 0, client_geometry->width, client_geometry->height,
                                               border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT, get_screen()->root_visual, mask, values);

        cookies[1] = xcb_reparent_window_checked(get_conn(), window, frame_id, 0, 0);

        auto x = static_cast<geom::GU>(client_geometry->x);
        auto y = static_cast<geom::GU>(client_geometry->y);
        auto w = static_cast<geom::GU>(client_geometry->width);
        auto h = static_cast<geom::GU>(client_geometry->height);
        auto tag = get_wm_name(get_conn(), window);
        ws::Window win{geom::Geometry{x, y, w, h}, window, frame_id, ws::Tag{tag.value_or("cxw_" + std::to_string(window)), focused_ws->m_id}};
        cookies[2] = xcb_map_window_checked(get_conn(), frame_id);
        cookies[3] = xcb_map_subwindows_checked(get_conn(), frame_id);

        process_request(cookies[0], win, [](auto w) { cx::println("Failed to create X window/frame"); });
        process_request(cookies[1], win, [](auto w) { cx::println("Re-parenting window {} to frame {} failed", w.client_id, w.frame_id); });
        process_request(cookies[2], win, [](auto w) { cx::println("Failed to map frame {}", w.frame_id); });
        process_request(cookies[3], win, [](auto w) { cx::println("Failed to map sub-windows of frame {} -> {}", w.frame_id, w.client_id); });
        if(!focused_ws) {
            DBGLOG("No workspace container was created. {}!", "Error");
        }

        auto layout_attributes = focused_ws->register_window(win);
        if(layout_attributes) {
            auto split_cfg = layout_attributes.value();
            if(split_cfg.existing_window && split_cfg.new_window) {
                configure_window_geometry(split_cfg.existing_window.value());
                configure_window_geometry(split_cfg.new_window.value());
            } else if(!split_cfg.existing_window && split_cfg.new_window) {
                configure_window_geometry(split_cfg.new_window.value());
            }
        } else {
            cx::println("FOUND NO LAYOUT ATTRIBUTES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        }
        client_to_frame_mapping[window] = frame_id;
        frame_to_client_mapping[frame_id] = window;
        delete win_attr;
        delete client_geometry;
    }

    auto Manager::unframe_window(const ws::Window& w) -> void
    {
        xcb_unmap_window(get_conn(), w.frame_id);
        xcb_reparent_window(get_conn(), w.client_id, get_root(), 0, 0);
        xcb_destroy_window(get_conn(), w.frame_id);
    }

    auto Manager::configure_window_geometry(ws::Window window) -> void
    {
        namespace xcm = xcb_config_masks;
        const auto& [x, y, width, height] = window.geometry.xcb_value_list();
        auto frame_properties = xcm::TELEPORT;
        auto child_properties = xcm::RESIZE;
        // cx::uint frame_values[] = {x, y, width, height};
        cx::uint child_values[] = {width, height};
        auto cookies = CONFIG_CX_WINDOW(window, frame_properties, window.geometry.xcb_value_list().data(), child_properties, child_values);
        for(const auto& cookie : cookies) {
            if(auto err = xcb_request_check(get_conn(), cookie); err) {
                DBGLOG("Failed to configure item {}. Error code: {}", err->resource_id, err->error_code);
            }
        }
    }

    // The event loop
    auto Manager::event_loop() -> void
    {
        setup();
        this->m_running = true;
        while(m_running) {
            auto evt = xcb_wait_for_event(get_conn());
            if(evt == nullptr) {
                // do something else
                // TODO: complete hack. It is for formatting purposes *only*. clang-format slugs it out the rest
                // to far otherwise
                continue;
            } else {
                switch(evt->response_type /*& ~0x80 = 127 = 0b01111111*/) {
                case XCB_MAP_REQUEST: {
                    handle_map_request((xcb_map_request_event_t*)(evt));
                    break;
                }
                case XCB_UNMAP_NOTIFY:
                    handle_unmap_request((xcb_unmap_window_request_t*)evt);
                    break;
                case XCB_MAPPING_NOTIFY: // alerts us if a *key mapping* has been done, NOT a window one
                    break;
                case XCB_MOTION_NOTIFY: {
                    auto e = (xcb_motion_notify_event_t*)evt;
                    cx::println("Motion notify event");
                    break;
                }
                case XCB_CONFIGURE_NOTIFY: {
                    break;
                }
                case XCB_CONFIGURE_REQUEST: {
                    handle_config_request((xcb_configure_request_event_t*)evt);
                    break;
                }
                case XCB_CLIENT_MESSAGE:
                    break;
                case XCB_BUTTON_PRESS: {
                    auto e = (xcb_button_press_event_t*)evt;
                    // TODO: implement focusing of client via clicking or some key-combination
                    // TODO: this will look through all clients in focused_ws, and focus the workspace's focus pointer on that client
                    focused_ws->focus_client(e->child);
                    break;
                }
                case XCB_BUTTON_RELEASE:
                    break;
                case XCB_KEY_PRESS: {
                    handle_key_press((xcb_key_press_event_t*)evt);
                    break;
                }
                case XCB_KEY_RELEASE:
                    break;
                case XCB_EXPOSE: {
                    auto e = (xcb_expose_event_t*)evt;
                    cx::println("Expose event for {} caught", e->window);
                    break;
                }
                }
            }
        }
    }
    auto Manager::add_workspace(const std::string& workspace_tag, std::size_t screen_number) -> void
    {
        m_workspaces.emplace_back(std::make_unique<ws::Workspace>(m_workspaces.size(), workspace_tag, geom::Geometry{0, 0, 800, 600}));
    }

} // namespace cx
