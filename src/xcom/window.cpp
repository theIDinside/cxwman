#include <utility>
#include <xcom/utility/drawing/util.h>
#include <xcom/utility/xinit.hpp>
#include <xcom/window.hpp>

namespace cx::workspace
{

    Tag::Tag(std::string tag, std::size_t ws_id) noexcept : m_tag(std::move(tag)), m_ws_id(ws_id) {}

    Window::Window() noexcept : original_size(0, 0, 0, 0), geometry(0, 0, 0, 0), client_id(0), frame_id(0), m_tag{} {}

    Window::Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, const Tag& tag, cx::cfg::Configuration cfg) noexcept
        : original_size(g), geometry(g), client_id(client), frame_id(frame), m_tag(tag), configuration(cfg)
    {
    }

    void Window::set_geometry(geom::Geometry g) noexcept { this->geometry = g; }
    void Window::draw_title(xcb_connection_t* c, const std::optional<std::string>& new_title) {
        m_tag.m_tag = new_title.value_or(m_tag.m_tag);
        auto font_gc = x11::get_font_gc(c, frame_id, 0x000000, (u32)configuration.frame_background_color, "7x13");
        auto text_extents_cookie = xcb_query_text_extents(c, font_gc.value(), m_tag.m_tag.length(),
                                                          reinterpret_cast<const xcb_char2b_t*>(m_tag.m_tag.c_str()));
        cx::x11::X11Resource text_extents = xcb_query_text_extents_reply(c, text_extents_cookie, nullptr);
        auto [x_pos, y_pos] = cx::draw::utils::align_vertical_middle_left_of(text_extents, geometry.width, 16);
        xcb_clear_area(c, 1, frame_id, 0, 0, geometry.width, this->configuration.frame_title_height);
        xcb_image_text_8(c, m_tag.m_tag.length(), frame_id, font_gc.value(), x_pos, y_pos, m_tag.m_tag.c_str());
        xcb_flush(c);
    }
}; // namespace cx::workspace
