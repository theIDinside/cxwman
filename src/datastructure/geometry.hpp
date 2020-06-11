#pragma once
// System headers
#include <algorithm>
#include <tuple>
// Library/Application headers
#include <coreutils/core.hpp>
namespace cx::geom
{

    // Geometry Unit
    using GU = cx::u16;
    struct Position {
        GU x, y;
    };

    struct Geometry {
        Position pos;
        GU width, height;
        Geometry(GU x, GU y, GU width, GU height) : pos{x, y}, width(width), height(height) {}
        Geometry(Position p, GU width, GU height) : pos(std::move(p)), width(width), height(height) {}

        std::pair<Geometry, Geometry> vsplit(float split_ratio);
        std::pair<Geometry, Geometry> hsplit(float split_ratio);

        constexpr inline GU y() const { return pos.y; }
        constexpr inline GU x() const { return pos.x; }
    };

} // namespace cx::geom
