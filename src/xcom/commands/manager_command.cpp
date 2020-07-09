//
// Created by cx on 2020-07-08.
//

#include "manager_command.hpp"
#include <cassert>
#include <datastructure/container.hpp>
#include <xcom/constants.hpp>
#include <xcom/core.hpp>

namespace cx::commands
{
    void cx::commands::FocusWindow::perform(xcb_connection_t* c) const
    {
        auto deactivated_cookie = xcb_change_window_attributes_checked(c, this->defocused_window.frame_id, XCB_CW_BORDER_PIXEL, (int[]){icol});
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
} // namespace cx::commands
