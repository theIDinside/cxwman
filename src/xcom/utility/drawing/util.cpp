//
// Created by cx on 2020-07-16.
//

#include "util.h"
namespace cx::draw::utils {
    Position align_text_center_of(xcb_query_text_extents_reply_t* text_extents, GU box_width, GU box_height) noexcept {
        auto half_width = text_extents->overall_width / 2;
        auto half_box_width = box_width / 2;
        auto x_pos = half_box_width - half_width;

        auto half_height = text_extents->overall_ascent / 2;
        auto half_box_height = box_height / 2;
        auto y_pos = half_box_height + half_height;
        return Position{x_pos, y_pos};
    }
    Position align_vertical_middle_left_of(xcb_query_text_extents_reply_t* text_extents, GU box_width, GU box_height) noexcept {
        auto half_height = text_extents->overall_ascent / 2;
        auto half_box_height = box_height / 2;
        auto y_pos = half_box_height + half_height;
        return Position{2, y_pos};
    }
}