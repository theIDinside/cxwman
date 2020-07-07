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
        cx::geom::Geometry original_size; /// The size of the window when not maximized
        cx::geom::Geometry geometry;      /// current dimensions & position
        xcb_window_t client_id;           /// the id of the client application window
        xcb_window_t frame_id;            /// id of our frame, that holds client application window
        Tag m_tag;

        void set_geometry(geom::Geometry g) noexcept;
        friend bool operator==(const Window& lhs, const Window& rhs) { return lhs.client_id == rhs.client_id && lhs.frame_id == rhs.frame_id; }
    };
}; // namespace cx::workspace
