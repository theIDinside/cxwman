#pragma once
// System headers
#include <array>
#include <algorithm>
#include <tuple>
// Library/Application headers
#include <coreutils/core.hpp>
namespace cx::geom
{
    // Geometry Unit
    using GU = cx::u32;
    struct Position {
        GU x, y;
    };

    struct Geometry {
        Position pos;
        GU width, height;
        Geometry(GU x, GU y, GU width, GU height) : pos{x, y}, width(width), height(height) {}
        Geometry(Position p, GU width, GU height) : pos(std::move(p)), width(width), height(height) {}

        friend std::pair<Geometry, Geometry> vsplit(const Geometry& g, float split_ratio);
        friend std::pair<Geometry, Geometry> hsplit(const Geometry& g, float split_ratio);

        constexpr inline GU y() const { return pos.y; }
        constexpr inline GU x() const { return pos.x; }
        constexpr inline auto xcb_value_list() -> const std::array<GU, 4> { return {pos.x, pos.y, width, height}; }

        static Geometry default_new() {
            return Geometry{0, 0, 800, 600};
        }
    };
    std::pair<Geometry, Geometry> vsplit(const Geometry& g, float split_ratio = 0.5f);
    std::pair<Geometry, Geometry> hsplit(const Geometry& g, float split_ratio = 0.5f);
} // namespace cx::geom
