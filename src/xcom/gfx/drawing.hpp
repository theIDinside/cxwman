//
// Created by cx on 2020-06-28.
//

#pragma once

#include <xcom/xinit.hpp>

namespace cx::x11::gfx {

    enum class Color : uint32_t {
        Black = 0x000000,
        Blue = 0x0000ff,
        Green = 0x00ff00,
        Red = 0xff0000,
        White = 0xffffff// and more to come
    };

    struct Font {
        std::string_view name;
    };

    enum TextAlign {
        Center
    };

    struct DrawAttribute {
        DrawAttribute(Color foreground, Color background, cx::uint pen_line_width, xcb_drawable_t x_surface_id, geom::Geometry draw_region, std::optional<std::string_view> font = {});
        DrawAttribute(const DrawAttribute&) = default;
        ~DrawAttribute() = default;

        Color fg_color;
        Color bg_color;
        cx::uint pen_width;
        xcb_drawable_t surface;
        geom::Geometry clip_region; // always relative to surface's geometry. So (0, 1) means 1 pixel down, from top left corner of surface
        std::optional<std::string_view> font;
    };

    auto get_font_gc(const DrawAttribute& da) -> std::optional<xcb_gcontext_t>;
    auto draw_text(const DrawAttribute& draw_attributes, const std::string_view& text, TextAlign alignment);
}