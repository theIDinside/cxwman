#include <xcom/window.hpp>

namespace cx::workspace
{

    Tag::Tag(std::string tag) : m_tag(tag) {}

	Window::Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, Tag tag)
        : original_size(std::move(g)), client_id(client), frame_id(frame), m_tag(std::move(tag))
    {
    }
}; // namespace cx::workspace
