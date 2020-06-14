#pragma once
// System headers
#include <map>
#include <memory>
#include <xcb/xcb.h>

// Third party headers

// Library/Application headers
#include <datastructure/geometry.hpp>
#include <xcom/workspace.hpp>

#include <array>

// N.B! ALL "defines" that redefine XCB calls, are of the "checked" kind

/// Returns std::array<xcb_void_cookie_t, 2>, where i:0 is the frame cookie, and i:1 is the client cookie
#define CONFIG_CX_WINDOW(window, fprops, fvals, cprops, cvals) \
std::array<xcb_void_cookie_t, 2>{xcb_configure_window_checked(get_conn(), window.frame_id, fprops, fvals),\
xcb_configure_window_checked(get_conn(), window.client_id, cprops, cvals)}\


namespace cx
{

    namespace xcb_config_masks {
        constexpr auto CXWIN_X         = XCB_CONFIG_WINDOW_X;
        constexpr auto CXWIN_Y         = XCB_CONFIG_WINDOW_Y;
        constexpr auto CXWIN_WIDTH     = XCB_CONFIG_WINDOW_WIDTH;
        constexpr auto CXWIN_HEIGHT    = XCB_CONFIG_WINDOW_HEIGHT;
        constexpr auto CXWIN_STACK     = XCB_CONFIG_WINDOW_STACK_MODE;

        constexpr auto TELEPORT        = CXWIN_X | CXWIN_Y | CXWIN_WIDTH | CXWIN_HEIGHT;
        constexpr auto MOVE            = CXWIN_X | CXWIN_Y;
        constexpr auto RESIZE          = CXWIN_WIDTH | CXWIN_HEIGHT;
    };

    


    namespace ws = cx::workspace;

    using XCBConn = xcb_connection_t;
    using XCBScreen = xcb_screen_t;
    using XCBDrawable = xcb_drawable_t;
    using XCBWindow = xcb_window_t;

    // Yanked from the define in i3, to be used for our root window as well
    constexpr auto ROOT_EVENT_MASK =
        (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
         XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW);

    constexpr auto FRAME_EVENT_MASK = (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
                                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                                       XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW);


    // TODO(simon): These are utility functions, that serve *only* as wrappers,
    // so they become a mental model of what needs to be done, to set up a simplistic window manager

    /// This setups of what X calls SubstructureRedirection. This essentially means,
    // that all map requests will get sent to "us" (the window manager) so that we get
    // to specify how we handle them, for example, in a tiling window manager example,
    // we would check how we layout "our" windows, and then find a suitable place for the
    // soon-to-be mapped window. If we can't find one, in a tiling wm, we split a sub-space somewhere,
    // where a window exists, and let the new window take half that space, for example
    inline void setup_redirection_of_map_requests(XCBConn *conn, XCBWindow window);

    // This setups up, so that we tell the X-server, that we want to be notified of Mouse Button
    // events. This way, we override what needs to be done, so that we can hi-jack button presses in client windows
    inline void setup_mouse_button_request_handling(XCBConn *conn, XCBWindow window);

    constexpr auto MouseModMask = XCB_MOD_MASK_1;
    class Manager
    {
      public:
        // Public interface.
        static std::unique_ptr<Manager> initialize();
        void event_loop();

      private:
        Manager(XCBConn *connection, XCBScreen *screen, XCBDrawable root_drawable, XCBWindow root_window);

        inline auto get_conn()      -> XCBConn *;
        inline auto get_root()      -> XCBWindow;
        inline auto get_screen()    -> XCBScreen*;

        // This is called after we have initialized the WM. The setup handles reframing of already existing windows
        // and also sets up the workspace(s)
        auto setup() -> void;
    	auto setup_root_workspace_container() -> void;

        template <typename XCBCookieType>
        constexpr auto inspect_request(XCBCookieType cookie)
        {
            return xcb_request_check(x_detail.c, cookie);
        }

        template <typename XCBCookieType, typename ErrHandler>
        auto process_request(XCBCookieType cookie, ws::Window w, ErrHandler fn)
        {
            if(auto err = xcb_request_check(x_detail.c, cookie); err) {
                fn(w);
                delete err;
            }
        }

        // EVENT MANAGING
        auto handle_map_request(xcb_map_request_event_t *event) -> void;
        auto handle_config_request(xcb_configure_request_event_t* event) -> void;

        // We assume that most windows were not mapped/created before our WM started
        auto frame_window(XCBWindow window, geom::Geometry geometry = geom::Geometry{0, 0, 800, 600}, bool create_before_wm = false) -> void;

        auto configure_window_geometry(ws::Window w) -> void;

        // Unmaps currently focused workspace, and maps workspace ws
        auto map_workspace(ws::Workspace &ws) -> void;

        // TODO(remove/fix): this is data we will in the future receive from the Workspace type
        auto get_free_top_left() -> geom::Position;

        // These are data types that are needed to talk to X. It's none of the logic, that our Window Manager
        // actually needs.
        struct XInternals {
            XInternals(XCBConn *c, XCBScreen *scr, XCBDrawable rd, XCBWindow w) : c(c), screen(scr), root_drawable(rd), root_window(w) {}
            ~XInternals() {}
            XCBConn *c;
            XCBScreen *screen;
            XCBDrawable root_drawable;
            XCBWindow root_window;
        } x_detail;

        bool m_running;
        std::map<xcb_window_t, xcb_window_t> client_to_frame_mapping;
        std::map<xcb_window_t, xcb_window_t> frame_to_client_mapping;
        ws::Workspace *focused_ws;
    };
} // namespace cx
