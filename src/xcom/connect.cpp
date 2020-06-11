#include <coreutils/core.hpp>
#include <xcom/connect.hpp>

#define CXGRABMODE XCB_GRAB_MODE_ASYNC
namespace cx
{
    inline void setup_redirection_of_map_requests(XCBConn *conn, XCBWindow window)
    {
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
		if(!g_reply) {
			cx::println("Failed to get geometry of root window. Exiting");
			std::abort();
		}
		cx::println("Root geometry: Width x Height = {} x {}", g_reply->width, g_reply->height);

		// free(g_reply);

    }

    inline void setup_mouse_button_request_handling(XCBConn *conn, XCBWindow window)
    {
        auto mouse_button = 1; // left mouse button, 2 middle, 3 right
        auto PRESS_AND_RELEASE_MASK = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE;
        while (mouse_button < 4) {
            auto ck = xcb_grab_button_checked(conn, 0, window, PRESS_AND_RELEASE_MASK, CXGRABMODE, CXGRABMODE, window, XCB_NONE,
                                              mouse_button, MouseModMask);
            if (auto err = xcb_request_check(conn, ck); err) {
                cx::println("Could not set up handling of mouse button clicks for button {}", mouse_button);
            }
            mouse_button++;
        }
    }

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
#ifdef DEBUGGING
        cx::println("Screen size {} x {} pixels. Root window: {}", screen->width_in_pixels, screen->height_in_pixels, root_drawable);
#endif

        setup_mouse_button_request_handling(connection, window);
        setup_redirection_of_map_requests(connection, window);

        // TODO: tell X we reparent windows, because we are the manager
        // TODO: grab keys & set up keysymbols and configs


        return std::unique_ptr<Manager>(new Manager{connection, screen, root_drawable, window});
    }

    // Private constructor called via public interface function Manager::initialize()
    Manager::Manager(XCBConn *connection, XCBScreen *screen, XCBDrawable root_drawable, XCBWindow root_window)
        : x_detail{connection, screen, root_drawable, root_window}, m_running(false), client_to_frame_mapping{}, focused_ws(nullptr)
    {
    }

    inline XCBConn *Manager::get_conn() { return x_detail.c; }
    inline XCBWindow Manager::get_root() { return x_detail.root_window; }
	inline XCBScreen* Manager::get_screen() { return x_detail.screen; } 
    void Manager::map_workspace(workspace::Workspace &ws) {}

    geom::Position Manager::get_free_top_left() { return geom::Position{0, 0}; }

	void Manager::handle_map_request(xcb_map_request_event_t* evt) {
		#ifdef DEBUGGING
		cx::println("Handling map request of window with id {}", evt->window);
		#endif
		
		frame_window(evt->window);
		auto map_cookie = xcb_map_window_checked(get_conn(), evt->window);
		if(auto err = inspect_request(map_cookie); err) {
			cx::println("Failed to map window {}", evt->window);
		}

	}

    void Manager::frame_window(XCBWindow window, bool created_before_wm)
    {
        constexpr auto border_width = 3;
		constexpr auto border_color = 0xff0000;
        constexpr auto bg_color = 0x010101;

        if (client_to_frame_mapping.count(window)) {
#ifdef DEBUGGING
            cx::println("Framing an already framed window is unhandled behavior. Returning early from framing function.");
#endif
            return;
        }
        auto attribute_cookie = xcb_get_window_attributes(get_conn(), window);
        auto win_attr = xcb_get_window_attributes_reply(get_conn(), attribute_cookie, nullptr);
        auto win_geom = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), window), nullptr);
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
		cx::println("Properties of the client window {}: Position: ({}, {}). Client Dimensions: ({}x{}). Frame id: {}", window, pos_x, pos_y, win_geom->width, win_geom->height, frame_id);
		uint32_t mask = 0;
		uint32_t values[2];
		/* see include/xcb.h for the FRAME_EVENT_MASK */
		mask |= XCB_CW_BORDER_PIXEL;
		values[0] = border_color;

		mask |= XCB_CW_EVENT_MASK;
		values[1] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

        auto wcookie = xcb_create_window_checked(get_conn(), XCB_COPY_FROM_PARENT, frame_id, get_root(), pos_x, pos_y, win_geom->width, win_geom->height,
                          border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT, /*x_detail.screen->root_visual*/ XCB_COPY_FROM_PARENT, mask, values);
		auto creation_error = xcb_request_check(get_conn(), wcookie);
		if(creation_error != nullptr) {
			cx::println("Failed to create X window/frame");
		}
		xcb_reparent_window(get_conn(), window, frame_id, 0, 0);
		xcb_map_window(get_conn(), frame_id);
    }

    void Manager::run_loop()
    {

		// Grab top levels
		// auto vcookie = xcb_grab_server(get_conn());

		xcb_grab_server(get_conn());
		//TODO: handle mapping & reparenting already existing top-level windows
		xcb_ungrab_server(get_conn());
        this->m_running = true;
        // TODO: handle events & dispatch them
        while (m_running) {
            auto evt = xcb_wait_for_event(get_conn());
            if (evt == nullptr) {
                // do something else

                // TODO: complete hack. It is for formatting purposes *only*. clang-format slugs it out the rest
                // to far otherwise
                continue;
            } else {
				switch (evt->response_type & ~0x80) {
				case XCB_MAP_REQUEST:
					// find space for our "new" (or just unmapped-to-be-mapped) window
					// If no space: split a window in half, and let the newly to-be share with the old already
					// mapped window
					handle_map_request((xcb_map_request_event_t*)(evt));
					break;
				case XCB_MAPPING_NOTIFY: // alerts us if a *key mapping* has been done, NOT a window one
					break;
				case XCB_MOTION_NOTIFY:
					break;
				case XCB_CONFIGURE_NOTIFY:
					break;
				case XCB_CONFIGURE_REQUEST: {
					auto e = (xcb_configure_request_event_t*)evt;
					auto mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
					int values[] = { e->x, e->y, e->width, e->height };
					auto cfg_cookie = xcb_configure_window_checked(get_conn(), e->window, mask, values);
					if(auto err = xcb_request_check(get_conn(), cfg_cookie); err) {
						cx::println("Failed to configure window {}", e->window);
					}
					break;
				}
				case XCB_CLIENT_MESSAGE:
					break;
				case XCB_BUTTON_PRESS:
					cx::println("Button pressed");
					break;
				case XCB_BUTTON_RELEASE:
					cx::println("Button released");
					break;
				case XCB_KEY_PRESS:
					break;
				case XCB_KEY_RELEASE:
					break;
				}
			}
        }
    }
} // namespace cx
