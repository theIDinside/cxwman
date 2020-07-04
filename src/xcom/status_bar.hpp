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
        std::optional<int> clicked_workspace(xcb_window_t item);
    };
    using SysBar = std::unique_ptr<StatusBar>;
    using WBox = std::unique_ptr<WorkspaceBox>;

    /// Talks to X-server and creates the required x server resources, returns a well-formed StatusBar object
    SysBar make_system_bar(xcb_connection_t* c, xcb_screen_t* screen, std::size_t workspace_count, geom::Geometry sys_bar_geometry);

} // namespace cx::workspace