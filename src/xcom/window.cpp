#include <xcom/window.hpp>

namespace cx::workspace
{

    Tag::Tag(std::string tag) : m_tag(tag) {}

    Window::Window() : original_size(0, 0, 0, 0), geometry(0, 0, 0, 0), client_id(0), frame_id(0), m_tag{} {}

    Window::Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, Tag tag)
        : original_size(g), geometry(g), client_id(client), frame_id(frame), m_tag(std::move(tag))
    {
    }

    void Window::set_geometry(geom::Geometry g) {
        this->geometry = g;
    }
}; // namespace cx::workspace
