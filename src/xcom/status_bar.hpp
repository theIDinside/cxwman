//
// Created by cx on 2020-06-26.
//

#pragma once

#include "xinit.hpp"
#include <datastructure/geometry.hpp>
#include <map>
#include <xcb/xproto.h>

namespace cx::workspace
{

    enum class Color : uint32_t { Black = 0x000000, Blue = 0x0000ff, Green = 0x00ff00, Red = 0xff0000, White = 0xffffff };

    enum class ItemState { INACTIVE = 0, ACTIVE = 1 };

    struct WorkspaceBox {
        cx::uint workspace_id;
        xcb_drawable_t button_id;
        geom::Geometry dimension;
        ItemState state;
        xcb_gcontext_t draw_props;
        WorkspaceBox(cx::uint ws_id, xcb_drawable_t xid, geom::Geometry geometry);
        void draw(xcb_connection_t* c);

        template <typename Cb>
        auto signal(Cb cb) {
            cb(workspace_id);
        }
    };

    class StatusBar
    {
      private:
        cx::geom::Geometry geometry;
        std::map<xcb_window_t, std::unique_ptr<WorkspaceBox>> items;
        std::size_t active_workspace;
      public:

        // one-time initialize the graphics contexts
        // application window id
        xcb_window_t drawable;
        xcb_gcontext_t gc_inactive;
        xcb_gcontext_t gc_active;
        xcb_connection_t* c;
        xcb_window_t active_workspace_button;
        StatusBar(xcb_connection_t* c, xcb_window_t assigned_id, geom::Geometry assigned_geometry, std::vector<std::unique_ptr<WorkspaceBox>> boxes, xcb_gcontext_t active,
                  xcb_gcontext_t inactive);
        void draw(std::size_t active_workspace = 0);
        void set_active(std::size_t item);
        void update();
        void add_workspace();
        bool has_child(xcb_window_t window);

        template <typename CallBack>
        void clicked_workspace(xcb_window_t item, CallBack cb) {
            if(this->items.count(item) && item != active_workspace_button) {
                std::array<xcb_void_cookie_t, 4> cookies{};
                items[item]->draw_props = gc_active;
                items[active_workspace_button]->draw_props = gc_inactive;

                auto bg_mask = XCB_CW_BACK_PIXEL;
                auto color = 0x00ff00;

                cookies[0] = xcb_change_window_attributes_checked(c, item, bg_mask, (int[]){color});
                cookies[1] = xcb_clear_area_checked(c, 1, item, 0, 0, 30, 30);
                cookies[2] = xcb_change_window_attributes_checked(c, active_workspace_button, bg_mask, (int[]){0x0000ff});
                cookies[3] = xcb_clear_area_checked(c, 1, active_workspace_button, 0, 0, 30, 30);

                for(const auto& cookie : cookies) {
                    if(auto err = xcb_request_check(c, cookie); err) {
                        cx::println("Failed to change attributes of window. Error code: {}", err->error_code);
                    }
                }
                active_workspace_button = item;
                cb(items[item]->workspace_id);
            }
        }
    };
    using SysBar = std::unique_ptr<StatusBar>;
    using WBox = std::unique_ptr<WorkspaceBox>;

    /// Talks to X-server and creates the required x server resources, returns a well-formed StatusBar object
    SysBar make_system_bar(xcb_connection_t* c, xcb_screen_t* screen, std::size_t workspace_count, geom::Geometry sys_bar_geometry);

} // namespace cx::workspace