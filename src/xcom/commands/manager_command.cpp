//
// Created by cx on 2020-07-08.
//

#include "manager_command.hpp"
#include <cassert>
#include <datastructure/container.hpp>
#include <memory_resource>
#include <numeric>
#include <xcom/constants.hpp>
#include <xcom/core.hpp>
#include <xcom/manager.hpp>
#include <xcom/workspace.hpp>

namespace cx::commands
{
    void cx::commands::FocusWindow::perform(xcb_connection_t* c) const
    {
        auto deactivated_cookie = xcb_change_window_attributes_checked(c, defocused_window.value().frame_id, XCB_CW_BORDER_PIXEL, (int[]){icol});
        // error handling / reporting
        if(auto err = xcb_request_check(c, deactivated_cookie); err) {
            cx::println("Failed to change color of de-focused window");
        }
        auto focus_cookie = xcb_change_window_attributes_checked(c, window.frame_id, XCB_CW_BORDER_PIXEL, (int[]){acol});
        if(auto err = xcb_request_check(c, focus_cookie); err) {
            cx::println("Failed to make focused window colored");
        }

        xcb_flush(c);
    }
    void FocusWindow::request_state(Manager* m)
    {
        auto border_col_cfg = m->get_config().borders;
        icol = border_col_cfg.inactive;
        acol = border_col_cfg.active;
    }
    void FocusWindow::set_defocused(ws::Window w) { defocused_window = w; }
    void ChangeWorkspace::perform(xcb_connection_t* c) const {}
    void ConfigureWindows::perform(xcb_connection_t* c) const
    {
        namespace xcm = xcb_config_masks;
        auto configure_window_geometry = [c](auto& window) {
            const auto& [x, y, width, height] = window.geometry.xcb_value_list_border_adjust(1);
            auto frame_properties = xcm::TELEPORT;
            auto child_properties = xcm::RESIZE;
            // cx::uint frame_values[] = {x, y, width, m_tree_height};
            cx::uint child_values[] = {(cx::uint)width, (cx::uint)height};
            // TODO: Fix so that borders show up on the right side and bottom side of windows.
            cx::uint frame_vals[]{(cx::uint)x, (cx::uint)y, (cx::uint)width, (cx::uint)height};

            std::array<xcb_void_cookie_t, 2> cookies{xcb_configure_window_checked(c, window.frame_id, frame_properties, frame_vals),
                                                     xcb_configure_window_checked(c, window.client_id, child_properties, child_values)};
            for(const auto& cookie : cookies) {
                if(auto err = xcb_request_check(c, cookie); err) {
                    DBGLOG("Failed to configure item {}. Error code: {}", err->resource_id, err->error_code);
                }
            }
        };

        if(existing_window) {
            configure_window_geometry(existing_window.value());
            configure_window_geometry(window);
        } else {
            configure_window_geometry(window);
        }
        xcb_flush(c);
    }
    void ConfigureWindows::request_state(Manager* m) {}
    void KillClient::perform(xcb_connection_t* c) const {}
    void UpdateWindows::perform(xcb_connection_t* c) const
    {
        namespace xcm = xcb_config_masks;
        auto configure_window_geometry = [c](auto& window) {
            const auto& [x, y, width, height] = window.geometry.xcb_value_list_border_adjust(1);
            auto frame_properties = xcm::TELEPORT;
            auto child_properties = xcm::RESIZE;
            // cx::uint frame_values[] = {x, y, width, m_tree_height};
            cx::uint child_values[] = {(cx::uint)width, (cx::uint)height};
            // TODO: Fix so that borders show up on the right side and bottom side of windows.
            cx::uint frame_vals[]{(cx::uint)x, (cx::uint)y, (cx::uint)width, (cx::uint)height};

            std::array<xcb_void_cookie_t, 2> cookies{xcb_configure_window_checked(c, window.frame_id, frame_properties, frame_vals),
                                                     xcb_configure_window_checked(c, window.client_id, child_properties, child_values)};
            for(const auto& cookie : cookies) {
                if(auto err = xcb_request_check(c, cookie); err) {
                    DBGLOG("Failed to configure item {}. Error code: {}", err->resource_id, err->error_code);
                }
            }
        };

        for(const auto& window : windows)
            configure_window_geometry(window);
    }
    UpdateWindows::UpdateWindows(const std::vector<ws::ContainerTree*>& nodes) noexcept : ManagerCommand{"Display update windows"}, windows{}
    {
#ifdef DEBUGGING
        for(const auto node : nodes) {
            assert(node->is_window() && "Each node passed to UpdateWindows must be of window 'type'");
        }
#endif
        std::transform(std::begin(nodes), std::end(nodes), std::back_inserter(windows), [](auto t) { return t->client.value(); });
    }
    void UpdateWindows::request_state(Manager* m) {}
    void MoveWindow::perform(xcb_connection_t* c) const
    {
        using Dir = geom::ScreenSpaceDirection;
        using Vec = cx::geom::Vector;
        events::Pos target_space{0, 0};
        const auto& bounds = workspace->m_root->geometry;
        auto window_result = workspace->find_window(window.client_id);
        if(window_result) {
            switch(direction) {
            case Dir::UP:
                target_space = geom::wrapping_add(middle_of_side(window.geometry, direction), Vec::axis_aligned(direction, 10), bounds, 10);
                break;
            case Dir::DOWN:
                target_space = geom::wrapping_add(middle_of_side(window.geometry, direction), Vec::axis_aligned(direction, 10), bounds, 10);
                break;
            case Dir::LEFT:
                target_space = geom::wrapping_add(middle_of_side(window.geometry, direction), Vec::axis_aligned(direction, 10), bounds, 10);
                break;
            case Dir::RIGHT:
                target_space = geom::wrapping_add(middle_of_side(window.geometry, direction), Vec::axis_aligned(direction, 10), bounds, 10);
                break;
            }
            if(!geom::is_inside(target_space, window.geometry)) {
                auto target_client = tree_in_order_find(
                    workspace->m_root, [&](auto& tree) { return geom::is_inside(target_space, tree->geometry) && tree->is_window(); });
                if(target_client) {
                    auto window_node = window_result.value();
                    move_client(window_node, *target_client);
                    auto mapper = [c](auto& window) {
                        // auto window = window_opt;
                        namespace xcm = cx::xcb_config_masks;
                        const auto& [x, y, width, height] = window.geometry.xcb_value_list();
                        auto frame_properties = xcm::TELEPORT;
                        auto child_properties = xcm::RESIZE;
                        cx::uint frame_values[] = {(cx::uint)x, (cx::uint)y, (cx::uint)width, (cx::uint)height};
                        cx::uint child_values[] = {(cx::uint)width, (cx::uint)height};
                        auto cookies =
                            std::array<xcb_void_cookie_t, 2>{xcb_configure_window_checked(c, window.frame_id, frame_properties, frame_values),
                                                             xcb_configure_window_checked(c, window.client_id, child_properties, child_values)};
                        for(const auto& cookie : cookies) {
                            if(auto err = xcb_request_check(c, cookie); err) {
                                DBGLOG("Failed to configure item {}. Error code: {}", err->resource_id, err->error_code);
                            }
                        }
                    };
                    in_order_window_map(workspace->m_root, mapper);
                } else {
                    DBGLOG("Could not find a suitable window to swap with. Position: ({},{})", target_space.x, target_space.y);
                }
            } else {
                DBGLOG("Target position is inside source geometry. No move {}", "");
            }
        }
    }
    void MoveWindow::request_state(Manager* m) {}
    auto parse_string(std::string_view str) -> std::optional<std::unique_ptr<ManagerCommand>>
    {
        std::optional<std::string> cmd;
        std::vector<std::string> params;

        while(!str.empty()) {
            auto part = str.find_first_of(' ');
            if(!cmd)
                cmd = std::make_optional(str.substr(0, part));
            else
                params.emplace_back(str.substr(0, part));
            str.remove_prefix(part);
        }

        return std::optional<std::unique_ptr<ManagerCommand>>();
    }
} // namespace cx::commands
