// Library / Application headers
#include <coreutils/core.hpp>
#include <xcom/manager.hpp>

// System headers xcb
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>
// STD System headers
#include <algorithm>
#include <cassert>
#include <memory>

namespace cx
{
    template<typename StrT>
    constexpr auto name_len(StrT str)
    {
        return std::strlen(str);
    }

#define x_replace_str_prop(c, window, atom, string)                                                                                                  \
    xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, atom, XCB_ATOM_STRING, 8, name_len(string), string)

    auto Manager::setup() -> void
    {
        // TODO(implement): grab pre-existing windows, reparent them properly and manage them
        xcb_grab_server(get_conn());
        setup_root_workspace_container();
        xcb_ungrab_server(get_conn());
    }

    auto Manager::setup_root_workspace_container() -> void
    {
        // auto win_geom = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), get_root()), nullptr);
        add_workspace("Workspace 1", 0);
        this->focused_ws = m_workspaces[0].get();
    }

    auto Manager::initialize() -> std::unique_ptr<Manager>
    {
        int screen_number;
        x11::XCBScreen* screen = nullptr;
        x11::XCBDrawable root_drawable;
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
        x11::XCBWindow window = screen->root;

        // Set this in pre-processor variable in CMake, to run this code
        DBGLOG("Screen size {} x {} pixels. Root window: {}", screen->width_in_pixels, screen->height_in_pixels, root_drawable);
        // TODO: remove this call to setup_mouse... completely for root?
        // x11::setup_mouse_button_request_handling(c, window);
        x11::setup_redirection_of_map_requests(c, window);
        x11::setup_key_press_listening(c, window);
        // TODO: grab keys & set up keysymbols and configs
        auto ewmh_window = xcb_generate_id(c);
        xcb_create_window(c, XCB_COPY_FROM_PARENT, ewmh_window, window, -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT,
                          XCB_CW_OVERRIDE_REDIRECT, (uint32_t[]){1});
        auto atoms = cx::x11::get_supported_atoms(c, x11::atom_names);

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
    Manager::Manager(x11::XCBConn* connection, x11::XCBScreen* screen, x11::XCBDrawable root_drawable, x11::XCBWindow root_window,
                     x11::XCBWindow ewmh_window, xcb_key_symbols_t* symbols) noexcept
        : x_detail{connection, screen, root_drawable, root_window, ewmh_window, symbols},
          m_running(false), client_to_frame_mapping{}, frame_to_client_mapping{},
          focused_ws(nullptr), actions{}, m_workspaces{}, event_dispatcher{this}
    {
        // TODO: Set up key-combo-configurations with bindings like this? Or unnecessarily complex?
        actions[27] = [this]() {
            this->focused_ws->rotate_focus_pair();
            this->focused_ws->display_update(get_conn());
        };
    }

    [[nodiscard]] inline auto Manager::get_conn() const -> x11::XCBConn* { return x_detail.c; }

    [[nodiscard]] inline auto Manager::get_root() const -> x11::XCBWindow { return x_detail.screen->root; }

    [[nodiscard]] inline auto Manager::get_screen() const -> x11::XCBScreen* { return x_detail.screen; }

    // TODO(EWMHints): Grab EWM hints & set up supported hints
    auto Manager::handle_map_request(xcb_map_request_event_t* evt) -> void
    {
        frame_window(evt->window);
        xcb_map_window(get_conn(), evt->window);
    }

    // FIXME: When killing clients down to only 1 client, mapping new clients fails.
    auto Manager::handle_unmap_request(xcb_unmap_window_request_t* event) -> void
    {
        // DBGLOG("Handle unmap request for {}", event->window);
        auto window_container = focused_ws->find_window(event->window);

        if(window_container) {
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
            DBGLOG("Handle cfg for frame {} of client {}", frame, e->window);
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
        DBGLOG("Key code {} - KeySym: {}", event->detail, ksym);
        auto cfg = config::KeyConfiguration{ksym, event->state};
        // TODO: Is this the right way to deal with something like this? Too complex? I have no idea.
        // "Safe" event handler. Calls Manager::noop() if key-combo is not registered with a specific member function of Manager
        event_dispatcher.handle(cfg);
    }

    auto Manager::frame_window(x11::XCBWindow window, geom::Geometry geometry, bool created_before_wm) -> void
    {
        namespace xkm = xcb_key_masks;

        std::array<xcb_void_cookie_t, 4> cookies{};
        constexpr auto border_width = 2;
        constexpr auto border_color = 0xff12ab;
        constexpr auto bg_color = 0x010101;

        if(client_to_frame_mapping.count(window)) {
            DBGLOG("Framing an already framed window (id: {}) is unhandled behavior. Returning early from framing function.", window);
            return;
        }

        auto win_attr = xcb_get_window_attributes_reply(get_conn(), xcb_get_window_attributes(get_conn(), window), nullptr);
        auto client_geometry = process_x_geometry(xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), window), nullptr));
        DBGLOG("Client geometry: {},{} -- {}x{}", client_geometry->x(), client_geometry->y(), client_geometry->width, client_geometry->height);
        if(created_before_wm) {
            cx::println("Window was created before WM.");
            if(win_attr->override_redirect || win_attr->map_state != XCB_MAP_STATE_VIEWABLE)
                return;
        }

        // construct frame
        auto frame_id = xcb_generate_id(get_conn());
        uint32_t values[2];
        /* see include/xcb.h for the FRAME_EVENT_MASK */
        uint32_t mask = XCB_CW_BORDER_PIXEL;
        values[0] = border_color;
        mask |= XCB_CW_EVENT_MASK;
        values[1] = (cx::u32)XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;

        cookies[0] = xcb_create_window_checked(get_conn(), 0, frame_id, get_root(), 0, 0, client_geometry->width, client_geometry->height,
                                               border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT, get_screen()->root_visual, mask, values);
        auto rep = xcb_grab_button_checked(get_conn(), 1, window, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE,
                                           XCB_NONE, XCB_BUTTON_INDEX_1, xkm::SUPER_SHIFT);
        if(auto e = xcb_request_check(get_conn(), rep); e) {
            cx::println("Failed button grab on window {}", window);
        } else {
            cx::println("Succeeded in grabbing button on window {}", window);
        }
        auto co = xcb_configure_window_checked(get_conn(), window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]){10, 10});
        if(auto err2 = xcb_request_check(get_conn(), co); err2) {
            cx::println("Failed to set dimensions of new window");
        }
        cookies[1] = xcb_reparent_window_checked(get_conn(), window, frame_id, 1, 1);
        auto tag = x11::get_client_wm_name(get_conn(), window);
        ws::Window win{client_geometry.value_or(geom::Geometry::window_default()), window, frame_id,
                       ws::Tag{tag.value_or("cxw_" + std::to_string(window)), focused_ws->m_id}};
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
    }

    auto Manager::unframe_window(const ws::Window& w) -> void
    {
        xcb_unmap_window(get_conn(), w.frame_id);
        xcb_reparent_window(get_conn(), w.client_id, get_root(), 0, 0);
        xcb_destroy_window(get_conn(), w.frame_id);
        client_to_frame_mapping.erase(w.client_id);
        frame_to_client_mapping.erase(w.frame_id);
    }

    auto Manager::configure_window_geometry(ws::Window window) -> void
    {
        namespace xcm = xcb_config_masks;
        const auto& [x, y, width, height] = window.geometry.xcb_value_list();
        auto frame_properties = xcm::TELEPORT;
        auto child_properties = xcm::RESIZE;
        // cx::uint frame_values[] = {x, y, width, m_tree_height};
        cx::uint child_values[] = {(cx::uint)width, (cx::uint)height};
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
        setup_input_functions();
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
                case XCB_MAP_NOTIFY: {
                    cx::println("Map notify caught");
                    break;
                }
                case XCB_UNMAP_NOTIFY:
                    handle_unmap_request((xcb_unmap_window_request_t*)evt);
                    break;
                case XCB_CONFIGURE_REQUEST: {
                    cx::println("Configure request caught");
                    handle_config_request((xcb_configure_request_event_t*)evt);
                    break;
                }
                case XCB_BUTTON_PRESS: {
                    cx::println("Button press caught");
                    auto e = (xcb_button_press_event_t*)evt;
                    focused_ws->focus_client_with_xid(e->event);
                    break;
                }
                case XCB_BUTTON_RELEASE:
                    break;
                case XCB_KEY_PRESS: {
                    handle_key_press((xcb_key_press_event_t*)evt);
                    break;
                }
                case XCB_MAPPING_NOTIFY: // alerts us if a *key mapping* has been done, NOT a window one
                    cx::println("Mappting notify caught");
                    break;
                case XCB_MOTION_NOTIFY: // We just fall through all these for now, since we don't do anything right now anyway
                    cx::println("Motion notify caught");
                    break;
                case XCB_CLIENT_MESSAGE: // TODO(implement) XCB_CLIENT_MESSAGE:
                    cx::println("Client message caught");
                case XCB_CONFIGURE_NOTIFY: // TODO(implement) XCB_CONFIGURE_NOTIFY
                    cx::println("Configure notify caught");
                    break;
                case XCB_KEY_RELEASE: // TODO(implement)? XCB_KEY_RELEASE
                    cx::println("Key release caught");
                case XCB_EXPOSE:
                    cx::println("Expose caught");
                    break; // TODO(implement) XCB_EXPOSE
                }
            }
        }
    }
    auto Manager::add_workspace(const std::string& workspace_tag, std::size_t screen_number) -> void
    {
        m_workspaces.emplace_back(std::make_unique<ws::Workspace>(m_workspaces.size(), workspace_tag, geom::Geometry{0, 0, 800, 600}));
    }
    auto Manager::setup_input_functions() -> void
    {
        namespace xkm = xcb_key_masks;
        using KC = cx::config::KeyConfiguration;
        using Arg = cx::events::EventArg;
        using cx::events::Dir;
        using ResizeArg = cx::events::ResizeArgument;

        event_dispatcher.register_action(KC{XK_F4, xkm::SUPER_SHIFT}, &Manager::rotate_focused_layout);
        event_dispatcher.register_action(KC{XK_r, xkm::SUPER_SHIFT}, &Manager::rotate_focused_pair);

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER}, &Manager::move_focused, Arg{Dir::LEFT});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER}, &Manager::move_focused, Arg{Dir::RIGHT});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER}, &Manager::move_focused, Arg{Dir::UP});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER}, &Manager::move_focused, Arg{Dir::DOWN});

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::LEFT, 10}});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::RIGHT, 10}});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::UP, 10}});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::DOWN, 10}});

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::RIGHT, 10}});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::LEFT, 10}});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::DOWN, 10}});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::UP, 10}});
    }

    // Manager window/client actions
    auto Manager::rotate_focused_layout() -> void
    {
        focused_ws->rotate_focus_layout();
        focused_ws->display_update(get_conn());
    }

    auto Manager::rotate_focused_pair() -> void
    {
        focused_ws->rotate_focus_pair();
        focused_ws->display_update(get_conn());
    }
    auto Manager::noop() -> void { cx::println("Key combination not yet handled"); }

    auto Manager::move_focused(cx::events::EventArg arg) -> void
    {
        cx::println("Generalized move focused client call made");
        auto cmd_arg = std::get<cx::events::ScreenSpaceDirection>(arg.arg);
        focused_ws->move_focused(cmd_arg);
        focused_ws->display_update(get_conn());
    }
    auto Manager::increase_size_focused(cx::events::EventArg arg) -> void
    {
        auto resize_arg = std::get<cx::events::ResizeArgument>(arg.arg);
        focused_ws->increase_size_focused(resize_arg);
        focused_ws->display_update(get_conn());
    }
    auto Manager::decrease_size_focused(cx::events::EventArg arg) -> void
    {
        auto size_arg = std::get<cx::events::ResizeArgument>(arg.arg);
        focused_ws->decrease_size_focused(size_arg);
        focused_ws->display_update(get_conn());
    }
    auto process_x_geometry(xcb_get_geometry_reply_t* reply) -> std::optional<geom::Geometry>
    {
        if(reply) {
            auto res = geom::Geometry{static_cast<geom::GU>(reply->x), static_cast<geom::GU>(reply->y), static_cast<geom::GU>(reply->width),
                                      static_cast<geom::GU>(reply->height)};
            delete reply;
            return res;
        } else {
            return {};
        }
    }
} // namespace cx
