#pragma once
// System headers
#include <filesystem>
#include <map>
#include <set>

#include <memory>
#include <string_view>

// Third party headers

// Library/Application headers
#include <datastructure/geometry.hpp>

#include <xcom/constants.hpp>
#include <xcom/core.hpp>
#include <xcom/utility/config.hpp>
#include <xcom/workspace.hpp>
#include <xcom/xinit.hpp>

namespace cx
{

    namespace ws = cx::workspace;
    namespace fs = std::filesystem;

    constexpr auto MouseModMask = XCB_MOD_MASK_1;
    class Manager
    {
      public:
        typedef void (Manager::*MFP)();
        // Public interface.
        static std::unique_ptr<Manager> initialize();
        void event_loop();
        // FIXME?: Called when key-press combo is registered by WM, but not actually defined/handle a command
        static void noop();

      private:
        Manager(xinit::XCBConn* connection, xinit::XCBScreen* screen, xinit::XCBDrawable root_drawable, xinit::XCBWindow root_window,
                xinit::XCBWindow ewmh_window, xcb_key_symbols_t* symbols) noexcept;

        [[nodiscard]] inline auto get_conn() const -> xinit::XCBConn*;
        [[nodiscard]] inline auto get_root() const -> xinit::XCBWindow;
        [[nodiscard]] inline auto get_screen() const -> xinit::XCBScreen*;

        // This is called after we have initialized the WM. The setup handles reframing of already existing windows
        // and also sets up the workspace(s)
        auto setup() -> void;
        auto setup_root_workspace_container() -> void;

        template<typename XCBCookieType>
        constexpr auto inspect_request(XCBCookieType cookie)
        {
            return xcb_request_check(x_detail.c, cookie);
        }

        template<typename XCBCookieType, typename ErrHandler>
        auto process_request(XCBCookieType cookie, ws::Window w, ErrHandler fn)
        {
            if(auto err = xcb_request_check(x_detail.c, cookie); err) {
                fn(w);
                delete err;
            }
        }

        // EVENT MANAGING / Handlers
        auto handle_map_request(xcb_map_request_event_t* event) -> void;
        auto handle_unmap_request(xcb_unmap_window_request_t* event) -> void;
        auto handle_config_request(xcb_configure_request_event_t* event) -> void;
        auto handle_key_press(xcb_key_press_event_t* event) -> void;

        // We assume that most windows were not mapped/created before our WM started
        auto frame_window(xinit::XCBWindow window, geom::Geometry geometry = geom::Geometry{0, 0, 800, 600}, bool create_before_wm = false) -> void;
        auto unframe_window(const ws::Window& w) -> void;
        auto configure_window_geometry(ws::Window w) -> void;

        // TODO(implement): Unmaps currently focused workspace, and maps workspace ws
        auto map_workspace(ws::Workspace& ws) -> void;
        auto add_workspace(const std::string& workspace_tag, std::size_t screen_number = 0) -> void;

        auto load_keymap(const fs::path& file_path);
        void setup_input_functions();

        // Client navigation/movement
        void rotate_focused_layout();
        void rotate_focused_pair();
        void move_focused_left();
        void move_focused_right();
        void move_focused_up();
        void move_focused_down();

        // These are data types that are needed to talk to X. It's none of the logic, that our Window Manager
        // actually needs.
        xinit::XInternals x_detail;
        bool m_running;
        std::map<xcb_window_t, xcb_window_t> client_to_frame_mapping;
        std::map<xcb_window_t, xcb_window_t> frame_to_client_mapping;
        std::map<std::size_t, std::function<auto()->void>> actions;
        ws::Workspace* focused_ws;
        std::vector<std::unique_ptr<ws::Workspace>> m_workspaces;
        // TODO: Use/Not use a map of std::functions as keybindings?
        template <typename Receiver>
        struct EventHandler {
            EventHandler() = default;
            void handle(const config::KeyConfiguration& kc, Receiver* receiver) {
                if(key_map.count(kc) >= 1) {
                    (receiver->*key_map[kc])();
                } else {
                    Manager::noop();
                }
            }

            void register_action(const config::KeyConfiguration& k_cfg, typename Receiver::MFP fnptr) {
                key_map[k_cfg] = fnptr;
            }
            std::map<config::KeyConfiguration, typename Receiver::MFP> key_map;
        };

        EventHandler<Manager>  event_dispatcher;

    };

} // namespace cx
