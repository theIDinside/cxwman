//
// Created by cx on 2020-06-28.
//
#include "drawing.hpp"

namespace cx::x11::draw
{

    auto get_gc(XCBWindow window, const DrawAttribute& draw_attr) -> std::optional<xcb_gcontext_t>
    {
        auto gc = xcb_generate_id(x11Info.c);
        auto mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_LINE_WIDTH;
        uint32_t value_list[]{static_cast<uint32_t>(draw_attr.fg_color), static_cast<uint32_t>(draw_attr.bg_color), draw_attr.pen_width};
        auto cookie = xcb_create_gc_checked(x11::x11Info.c, gc, window, mask, value_list);
        if(auto err = xcb_request_check(x11Info.c, cookie); err) {
            cx::println("Failed to create graphics context for window {}. Error code: {}", window, err->error_code);
            return {};
        }
        return std::make_optional(gc);
    }

    auto get_font_gc(const DrawAttribute& da) -> std::optional<xcb_gcontext_t>
    {
        cx::println("No we are crashing in xinit.cpp");
        const auto& c = x11Info.c;
        auto font = xcb_generate_id(c);
        auto font_cookie = xcb_open_font_checked(c, font, da.font.value_or("rk14").length(), da.font.value_or("rk14").data());

        if(auto err = xcb_request_check(c, font_cookie); err) {
            cx::println("Could not open font. Error code: {}", err->error_code);
            return {};
        }
        auto gfx_context = xcb_generate_id(c);
        auto mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_LINE_WIDTH | XCB_GC_FONT;
        uint32_t v_list[]{static_cast<uint32_t>(da.fg_color), static_cast<uint32_t>(da.bg_color), da.pen_width, font};
        auto gc_cookie = xcb_create_gc_checked(c, gfx_context, da.surface, mask, v_list);
        if(auto err = xcb_request_check(c, gc_cookie); err) {
            cx::println("Could not create graphics context. Error code {}", err->error_code);
            return {};
        }
        font_cookie = xcb_close_font_checked(c, font);
        if(auto err = xcb_request_check(c, font_cookie); err) {
            cx::println("Could not close font. Error code: {}", err->error_code);
            return {};
        }
        return std::make_optional(gfx_context);
    }

    auto draw_text(const DrawAttribute& da, const std::string_view& text, TextAlign align)
    {
        auto& c = x11::x11Info.c;
        // draw rectangle
        xcb_rectangle_t xcb_rectangle[1] = {geom::xcb_rectangle(da.clip_region)};
        auto gc = get_font_gc(da).value();

        auto cookie_fInfo = xcb_query_text_extents(c, gc, text.length(), reinterpret_cast<const xcb_char2b_t*>(text.data()));
        auto font_info = xcb_query_text_extents_reply(c, cookie_fInfo, nullptr);
        auto ascent = 0;
        auto txt_img_width = 0;
        if(font_info) {
            ascent = font_info->font_ascent;
            txt_img_width = font_info->overall_width;
            cx::println("Font ascent found. Value: {}. Text image width: {}", ascent, txt_img_width);
        }
        free(font_info);
        const auto& g = da.clip_region;
        auto cookie_text = xcb_image_text_8_checked(c, text.length(), da.surface, gc, g.x() + (g.width - txt_img_width) / 2,
                                                    g.y() + ascent + (g.height - ascent) / 2, text.data());
        if(auto err = xcb_request_check(c, cookie_text); err) {
            cx::println("Failed to draw text");
        }
    }
    
    DrawAttribute::DrawAttribute(Color foreground, Color background, cx::uint pen_line_width, xcb_drawable_t x_surface_id, geom::Geometry draw_region,
                                 std::optional<std::string_view> font)
        : fg_color(foreground), bg_color(background), pen_width(pen_line_width), surface(x_surface_id), clip_region(draw_region), font(font)
    {
    }
} // namespace cx::x11::draw