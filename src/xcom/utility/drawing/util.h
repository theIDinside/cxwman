//
// Created by cx on 2020-07-16.
//

#pragma once

#include <datastructure/geometry.hpp>
#include <xcb/xproto.h>
namespace cx::draw::utils
{
    using cx::geom::Position;
    using cx::geom::GU;
    [[nodiscard]] Position align_text_center_of(xcb_query_text_extents_reply_t* text_extents, GU box_width, GU box_height) noexcept;
    [[nodiscard]] Position align_vertical_middle_left_of(xcb_query_text_extents_reply_t* text_extents, GU box_width, GU box_height) noexcept;
} // namespace cx::draw::utils