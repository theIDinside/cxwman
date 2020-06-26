//
// Created by cx on 2020-06-26.
//

#pragma once

#include <datastructure/geometry.hpp>
#include <xcb/xproto.h>

namespace cx::workspace {

    class StatusBar
    {
        cx::geom::Geometry geometry;
        // application window id
        xcb_window_t client_id;
    };
}
