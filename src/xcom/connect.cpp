#include <coreutils/core.hpp>
#include <xcom/connect.hpp>

#include <cassert>

#include <X11/keysymdef.h>
#include <X11/keysym.h>

#include <xcb/xcb_keysyms.h>

#define CXGRABMODE XCB_GRAB_MODE_ASYNC
namespace cx
{

    namespace debug
    {
        void print_modifiers(uint32_t mask)
        {
            const char *MODIFIERS[] = {
                    "Shift", "Lock", "Ctrl", "Alt",
                    "Mod2", "Mod3", "Mod4", "Mod5",
                    "Button1", "Button2", "Button3", "Button4", "Button5"
            };

            fmt::print("Modifier mask: ");
            for (const char **modifier = MODIFIERS ; mask; mask >>= 1, ++modifier) {
                if (mask & 1) {
                    fmt::print(*modifier);
                }
            }
            fmt::print("\n");
        }
    } // namespace debug
    



    inline void setup_redirection_of_map_requests(XCBConn *conn, XCBWindow window) {
    
        auto value_to_set = XCB_CW_EVENT_MASK;
        cx::u32 values[2];
        values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
        // values[0] = ROOT_EVENT_MASK;
        auto attr_req_cookie = xcb_change_window_attributes_checked(conn, window, value_to_set, values);
        if (auto err = xcb_request_check(conn, attr_req_cookie); err) {
            cx::println("Could not set Substructure Redirect for our root window. This window manager will not function");
            std::abort();
        }

        auto root_geometry = xcb_get_geometry(conn, window);
        auto g_reply = xcb_get_geometry_reply(conn, root_geometry, nullptr);
        if (!g_reply) {
            cx::println("Failed to get geometry of root window. Exiting");
            std::abort();
        }
        DBGLOG("Root geometry: Width x Height = {} x {}", g_reply->width, g_reply->height);
    }

    inline void setup_mouse_button_request_handling(XCBConn *conn, XCBWindow window)
    {
        auto mouse_button = 1; // left mouse button, 2 middle, 3 right, MouseModMask is alt + mouse click
        auto PRESS_AND_RELEASE_MASK = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;
        while (mouse_button < 4) {
            auto ck = xcb_grab_button_checked(conn, 0, window, PRESS_AND_RELEASE_MASK, CXGRABMODE, CXGRABMODE, window, XCB_NONE,
                                              mouse_button, XCB_MOD_MASK_ANY);
            if (auto err = xcb_request_check(conn, ck); err) {
                cx::println("Could not set up handling of mouse button clicks for button {}", mouse_button);
            }
            mouse_button++;
        }
    }

    inline void setup_key_press_listening(XCBConn* conn, XCBWindow root) {
        namespace KM = xcb_key_masks;


        if(auto keysyms = xcb_key_symbols_alloc(conn); keysyms) {
            auto f4_keycodes = xcb_key_symbols_get_keycode(keysyms, XK_F4);
            xcb_key_symbols_free(keysyms);
            auto kc_count = 0;
            while(f4_keycodes[kc_count] != XCB_NO_SYMBOL) {
                kc_count++;
            }
            cx::println("Found {} keycodes for F4. First: {}", kc_count, f4_keycodes[0]);
            auto i = 0;
            while(i < kc_count) {
                // Grab Super+Shift + F4
                xcb_grab_key(conn, 1, root, KM::SUPER_SHIFT, f4_keycodes[i], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);  
                ++i;
            }
        } else {
            cx::println("Failed to allocate keysymbol table... perhaps?");
        }

        if(auto keysyms = xcb_key_symbols_alloc(conn); keysyms) {
            auto f4_keycodes = xcb_key_symbols_get_keycode(keysyms, XK_R);
            xcb_key_symbols_free(keysyms);
            auto kc_count = 0;
            while(f4_keycodes[kc_count] != XCB_NO_SYMBOL) {
                kc_count++;
            }
            cx::println("Found {} keycodes for F4. First: {}", kc_count, f4_keycodes[0]);
            auto i = 0;
            while(i < kc_count) {
                // Grab Super+Shift + F4
                xcb_grab_key(conn, 1, root, KM::SUPER_SHIFT, f4_keycodes[i], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);  
                ++i;
            }
        } else {
            cx::println("Failed to allocate keysymbol table... perhaps?");
        }

    }

    void Manager::setup() {
        // Grab top levels
        // auto vcookie = xcb_grab_server(get_conn());
        xcb_grab_server(get_conn());
        setup_root_workspace_container();
        // TODO: handle mapping & reparenting already existing top-level windows
        xcb_ungrab_server(get_conn());
    }

	void Manager::setup_root_workspace_container() {

        // auto win_geom = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), get_root()), nullptr);
        this->focused_ws = new workspace::Workspace{0, "Workspace 1", geom::Geometry{0, 0, 800, 600}};
	}

    // TODO(implement): key grabbing/listening for key combos

    std::unique_ptr<Manager> Manager::initialize()
    {
        int screen_number;
        XCBScreen *screen = nullptr;
        XCBDrawable root_drawable;
        auto connection = xcb_connect(nullptr, &screen_number);
        if (xcb_connection_has_error(connection)) {
            throw std::runtime_error{"XCB Error: failed trying to connect to X-server"};
        }

        auto screen_iter = xcb_setup_roots_iterator(xcb_get_setup(connection));

        // Find our screen
        for (auto i = 0; i < screen_number; ++i)
            xcb_screen_next(&screen_iter);

        screen = screen_iter.data;

        if (!screen) {
            xcb_disconnect(connection);
            throw std::runtime_error{"XCB Error: Failed to find screen. Disconnecting from X-server."};
        }

        root_drawable = screen->root;
        XCBWindow window = screen->root;

        // Set this in pre-processor variable in CMake, to run this code
        DBGLOG("Screen size {} x {} pixels. Root window: {}", screen->width_in_pixels, screen->height_in_pixels, root_drawable);
        setup_mouse_button_request_handling(connection, window);
        setup_redirection_of_map_requests(connection, window);
        setup_key_press_listening(connection, window);
        // TODO: tell X we reparent windows, because we are the manager
        // TODO: grab keys & set up keysymbols and configs
        return std::unique_ptr<Manager>(new Manager{connection, screen, root_drawable, window});
    }

    // Private constructor called via public interface function Manager::initialize()
    Manager::Manager(XCBConn *connection, XCBScreen *screen, XCBDrawable root_drawable, XCBWindow root_window)
        : x_detail{connection, screen, root_drawable, root_window}, m_running(false), client_to_frame_mapping{}, frame_to_client_mapping{}, focused_ws(nullptr)
    {

    }

    inline auto Manager::get_conn() -> XCBConn* { 
        return x_detail.c; 
    }

    inline auto Manager::get_root() -> XCBWindow { 
        return x_detail.screen->root;
    }

    inline auto Manager::get_screen() -> XCBScreen* { 
        return x_detail.screen; 
    }

	// TODO(Workspace, Tiling): handle where, and what properties the to-be mapped window should have
    auto Manager::handle_map_request(xcb_map_request_event_t *evt) -> void
    {
        local_persist int map_requests{};
        DBGLOG("Handling map request of window with id {}. #{}", evt->window, map_requests++);
        frame_window(evt->window);
        xcb_map_window(get_conn(), evt->window);
    }

    auto Manager::handle_unmap_request(xcb_unmap_window_request_t* event) -> void {
        if(client_to_frame_mapping.count(event->window) == 0) return;
        auto window = focused_ws->find_window(event->window);
        unframe_window(*window);
        focused_ws->unregister_window(&window.value());
    }


    auto Manager::handle_config_request(xcb_configure_request_event_t* e) -> void {
        cx::println("\tcfg request by: {} sibling: {}", e->window, e->sibling);
        xcb_generic_error_t *err;
        uint32_t values[7], mask = 0, i = 0;
        if(client_to_frame_mapping.count(e->parent) == 1) {
            xcb_window_t frame = client_to_frame_mapping[e->window];
            cx::println("Handle cfg for frame {} of client {}", frame, e->window);
            if (e->value_mask & XCB_CONFIG_WINDOW_X) {
                mask |= XCB_CONFIG_WINDOW_X;
                values[i++] = e->x;
            }
            if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
                mask |= XCB_CONFIG_WINDOW_Y;
                // Modify y to be within limits
                values[i++] = e->y;
            }
            if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_WIDTH;
                values[i++] = e->width;
            }
            if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                mask |= XCB_CONFIG_WINDOW_HEIGHT;
                values[i++] = e->height;
            }
            if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
                values[i++] = e->border_width;
            }
            err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), frame, mask, values));
            if (err) {
                cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
            }
        }
        mask = 0;
        i = 0;
        if (e->value_mask & XCB_CONFIG_WINDOW_X) {
            mask |= XCB_CONFIG_WINDOW_X;
            values[i++] = e->x;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
            mask |= XCB_CONFIG_WINDOW_Y;
            values[i++] = e->y;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_WIDTH;
            values[i++] = e->width;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
            values[i++] = e->height;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
            values[i++] = e->border_width;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
            mask |= XCB_CONFIG_WINDOW_SIBLING;
            values[i++] = e->sibling;
        }
        if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
            mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            values[i++] = e->stack_mode;
        }
        err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), e->window, mask, values));
        if (err) {
            cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
        }
    }


	// TODO(Workspace, Tiling): Add params, that are passed from handle_map_request, then map window accordingly
    auto Manager::frame_window(XCBWindow window, geom::Geometry geometry, bool created_before_wm) -> void
    {
        std::array<xcb_void_cookie_t, 4> cookies;

        constexpr auto border_width = 3;
        constexpr auto border_color = 0xff12ab;
        constexpr auto bg_color = 0x010101;

        if (client_to_frame_mapping.count(window)) {
            DBGLOG("Framing an already framed window (id: {}) is unhandled behavior. Returning early from framing function.", window);
            return;
        }
        
        auto win_attr = xcb_get_window_attributes_reply(get_conn(), xcb_get_window_attributes(get_conn(), window), nullptr);
        auto client_geometry = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), window), nullptr);
        if (created_before_wm) {
            cx::println("Window was created before WM.");
            if (win_attr->override_redirect || win_attr->map_state != XCB_MAP_STATE_VIEWABLE)
                return;
        }

        // construct frame
        auto frame_id = xcb_generate_id(get_conn());

        // TODO(implement): Where the frame window gets placed, is decided by us.
        // Where that is, depends on how we split subspaces, layout strategies etc. For now, we just put it at 0, 0
        auto pos_x = 0;
        auto pos_y = 0;

        uint32_t mask = 0;
        uint32_t values[2];
        /* see include/xcb.h for the FRAME_EVENT_MASK */
        mask = XCB_CW_BORDER_PIXEL;
        values[0] = border_color;
        mask |= XCB_CW_EVENT_MASK;
        values[1] = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS | 
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | 
        XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;

        cookies[0] = xcb_create_window_checked(get_conn(), 0, 
        frame_id, get_root(), pos_x, pos_y, client_geometry->width,
                                                 client_geometry->height, border_width, 
                                                 XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                 get_screen()->root_visual, mask, values);

        cookies[1] = xcb_reparent_window_checked(get_conn(), window, frame_id, 0, 0);
        
        auto x = static_cast<geom::GU>(client_geometry->x);
        auto y = static_cast<geom::GU>(client_geometry->y);
        auto w = static_cast<geom::GU>(client_geometry->width);
        auto h = static_cast<geom::GU>(client_geometry->height);

        ws::Window win{geom::Geometry{x, y, w, h}, window, frame_id, ws::Tag{"cx window_"}};
        cookies[2] = xcb_map_window_checked(get_conn(), frame_id);
        cookies[3] = xcb_map_subwindows_checked(get_conn(), frame_id);

        process_request(cookies[0], win, [](auto w) {
            cx::println("Failed to create X window/frame");
        });
        process_request(cookies[1], win, [](auto w) {
            cx::println("Reparenting window {} to frame {} failed", w.client_id, w.frame_id);	
        });
        process_request(cookies[2], win, [](auto w) {
            cx::println("Failed to map frame {}", w.frame_id);
        });
        process_request(cookies[3], win, [](auto w) {
            cx::println("Failed to map subwindows of frame {} -> {}", w.frame_id, w.client_id);	
        });
        if(!focused_ws)
        {
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

    auto Manager::unframe_window(ws::Window w) -> void {

    }

    auto Manager::configure_window_geometry(ws::Window window) -> void {
        namespace xcm = xcb_config_masks;
        const auto& [x, y, width, height] = window.geometry.xcb_value_list();
        auto frame_properties = xcm::TELEPORT;
        auto child_properties = xcm::RESIZE;
        cx::uint frame_values[] = { x, y, width, height };
        cx::uint child_values[] = { width, height };

        auto cookies = CONFIG_CX_WINDOW(window, frame_properties, frame_values, child_properties, child_values);
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
        // TODO: handle events & dispatch them
        auto map_requests = 0;
        while (m_running) {
            auto evt = xcb_wait_for_event(get_conn());
            if (evt == nullptr) {
                // do something else

                // TODO: complete hack. It is for formatting purposes *only*. clang-format slugs it out the rest
                // to far otherwise
                continue;
            } else {
                switch (evt->response_type /*& ~0x80 = 127 = 0b01111111*/) {
                    case XCB_MAP_REQUEST: {
                        handle_map_request((xcb_map_request_event_t *)(evt));
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
                        /*  TODO: implement focusing of client via clicking or some key-combination
                            auto managed_frame = e->child;
                            TODO: this will look through all clients in focused_ws, and focus the workspace's focus pointer on that client
                            focus_client(managed_frame);
                        */
                        cx::println("Button pressed for window child {}, root: {} event: {}", e->child, e->root, e->event);
                        break;
                    }
                    case XCB_BUTTON_RELEASE:
                        cx::println("Button released");
                        break;
                    case XCB_KEY_PRESS: {
                        auto e = (xcb_key_press_event_t*)evt;
                        debug::print_modifiers(e->state);
                        fmt::print("Key pressed: {}", e->detail);
                        if(e->detail == 70) { // Beautiful hack and slash. Winkey + Shift + F4
                            focused_ws->rotate_focus_layout();
                            focused_ws->display_update(get_conn());
                        } else if(e->detail == 27) {// Beautiful hack and slash. Winkey + Shift + R
                            focused_ws->rotate_focus_pair();
                            focused_ws->display_update(get_conn());
                        }
                        break;
                    }
                    case XCB_KEY_RELEASE:
                        cx::println("Key released");
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
} // namespace cx
