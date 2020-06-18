#include <utility>
#include <xcom/window.hpp>

namespace cx::workspace
{

    Tag::Tag(std::string tag, std::size_t ws_id) noexcept : m_tag(std::move(tag)), m_ws_id(ws_id) {}

    Window::Window() noexcept : original_size(0, 0, 0, 0), geometry(0, 0, 0, 0), client_id(0), frame_id(0), m_tag{} {}

    Window::Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, const Tag &tag) noexcept
        : original_size(g), geometry(g), client_id(client), frame_id(frame), m_tag(tag)
    {
    }

    void Window::set_geometry(geom::Geometry g) noexcept { this->geometry = g; }
}; // namespace cx::workspace
