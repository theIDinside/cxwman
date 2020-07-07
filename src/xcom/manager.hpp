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

#include "events.hpp"
#include <xcom/constants.hpp>
#include <xcom/core.hpp>
#include <xcom/status_bar.hpp>
#include <xcom/utility/config.hpp>
#include <xcom/workspace.hpp>
#include <xcom/xinit.hpp>

namespace cx
{

    namespace ws = cx::workspace;
    namespace fs = std::filesystem;

    /// This also free's the memory pointed to by reply
    auto process_x_geometry(xcb_connection_t* c, xcb_window_t window) -> std::optional<geom::Geometry>;

    void unmap(const ws::Window& w, xcb_connection_t* c);
    constexpr auto MouseModMask = XCB_MOD_MASK_1;

    // TODO: Use/Not use a map of std::functions as keybindings?
    template<typename Receiver>
    struct KeyEventHandler {
        using FunctionCall = std::pair<typename Receiver::MFPWA, cx::events::EventArg>;
        explicit KeyEventHandler(Receiver* r) : r(r) {}

        void handle(const config::KeyConfiguration& kc)
        {
            if(key_map.count(kc) >= 1) {
                (r->*key_map[kc])();
            } else if(key_map_with_args.count(kc) >= 1) {
                auto& [fn, arg] = key_map_with_args[kc];
                (r->*fn)(arg);
            } else {
                Receiver::noop();
            }
        }

        void register_action(const config::KeyConfiguration& k_cfg, typename Receiver::MFP fnptr) { key_map[k_cfg] = fnptr; }
        void register_action(const config::KeyConfiguration& k_cfg, typename Receiver::MFPWA fnptr, cx::events::EventArg arg)
        {
            key_map_with_args.emplace(k_cfg, FunctionCall{fnptr, arg});
        }

        Receiver* r;
        std::map<config::KeyConfiguration, typename Receiver::MFP> key_map{};
        std::map<config::KeyConfiguration, FunctionCall> key_map_with_args{};
    };

    class Manager
    {
      public:
        using MFP = void (Manager::*)();
        using MFPWA = void (Manager::*)(cx::events::EventArg);
        // Public interface.
        static auto initialize() -> std::unique_ptr<Manager>;
        static auto noop() -> void;
        auto event_loop() -> void;

      private:
        Manager(x11::XCBConn* connection, x11::XCBScreen* screen, x11::XCBDrawable root_drawable, x11::XCBWindow root_window,
                x11::XCBWindow ewmh_window, xcb_key_symbols_t* symbols) noexcept;

        [[nodiscard]] inline constexpr auto get_conn() const -> x11::XCBConn*;
        [[nodiscard]] inline constexpr auto get_root() const -> x11::XCBWindow;
        [[nodiscard]] inline constexpr auto get_screen() const -> x11::XCBScreen*;

        // This is called after we have initialized the WM. The setup handles reframing of already existing windows
        // and also sets up the workspace(s)
        auto setup() -> void;
        auto setup_root_workspace_container() -> void;

        template<typename XCBCookieType, typename ErrHandler>
        auto process_request(XCBCookieType cookie, ws::Window w, ErrHandler fn, bool abort_on_error = true)
        {
            if(auto err = xcb_request_check(x_detail.c, cookie); err) {
                fn(w);
                delete err;
                if(abort_on_error)
                    std::abort();
            }
        }

        // EVENT MANAGING / Handlers
        auto handle_map_request(xcb_map_request_event_t* event) -> void;
        auto handle_unmap_request(xcb_unmap_window_request_t* event) -> void;
        auto handle_config_request(xcb_configure_request_event_t* event) -> void;
        auto handle_key_press(xcb_key_press_event_t* event) -> void;

        // We assume that most windows were not mapped/created before our WM started
        auto frame_window(x11::XCBWindow window, geom::Geometry geometry = geom::Geometry{0, 0, 800, 600}, bool create_before_wm = false) -> void;
        auto unframe_window(const ws::Window& w, bool destroy_client = true) -> void;
        // Makes configure request to X so that it updates window, based on our representation of what it should be
        auto configure_window_geometry(const ws::Window& w) -> void;

        auto add_workspace(const std::string& workspace_tag, std::size_t screen_number = 0) -> void;
        // TODO: implement some rudimentary configuration system that can load key actions from file and/or bind key actions at runtime
        auto load_keymap(const fs::path& file_path);
        auto setup_input_functions() -> void;

        // Client navigation/movement
        void rotate_focused_layout();
        void rotate_focused_pair();

        auto move_focused(cx::events::EventArg arg) -> void;
        auto increase_size_focused(cx::events::EventArg arg) -> void;
        auto decrease_size_focused(cx::events::EventArg arg) -> void;
        auto kill_client(cx::events::EventArg arg) -> void;

        auto change_workspace(std::size_t ws_id) -> void;

        // These are data types that are needed to talk to X. It's none of the logic, that our Window Manager
        // actually needs.
        x11::XInternals x_detail;
        bool m_running;
        std::map<xcb_window_t, xcb_window_t> client_to_frame_mapping;
        std::map<xcb_window_t, xcb_window_t> frame_to_client_mapping;
        ws::Workspace* focused_ws;
        std::vector<std::unique_ptr<ws::Workspace>> m_workspaces;
        std::unique_ptr<ws::StatusBar> status_bar;

        KeyEventHandler<Manager> event_dispatcher;
    };

} // namespace cx
