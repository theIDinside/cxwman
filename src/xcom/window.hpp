#pragma once
#include <datastructure/geometry.hpp>
#include <string>
#include <xcb/xcb.h>

namespace cx::workspace
{

    struct Tag {
        Tag() noexcept = default;
        Tag(std::string tag, std::size_t ws_id) noexcept;
        ~Tag() noexcept = default;
        std::string m_tag{};
        std::size_t m_ws_id{};
    };

    // This is just pure data. No object oriented "behavior" will be defined or handled in this struct. Fuck OOP
    struct Window {

        Window() noexcept;
        Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, const Tag& tag) noexcept;

        // The size of the window when not maximized
        cx::geom::Geometry original_size;
        cx::geom::Geometry geometry;
        // application window id
        xcb_window_t client_id;
        // the frame id, of which we reparent app window id to, thus controlling the app window
        xcb_window_t frame_id;
        Tag m_tag;

        void set_geometry(geom::Geometry g) noexcept;
    };
}; // namespace cx::workspace
