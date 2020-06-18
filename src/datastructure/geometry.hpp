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

        friend std::pair<Geometry, Geometry> v_split(const Geometry& g, float split_ratio);
        friend std::pair<Geometry, Geometry> h_split(const Geometry& g, float split_ratio);

        [[nodiscard]] constexpr inline GU y() const { return pos.y; }
        [[nodiscard]] constexpr inline GU x() const { return pos.x; }
        constexpr inline auto xcb_value_list() -> std::array<GU, 4> { return {pos.x, pos.y, width, height}; }

        static Geometry default_new() {
            return Geometry{0, 0, 800, 600};
        }

        friend bool aabb_collision(const Geometry& p, const Geometry& geometry);
        friend bool is_inside(const Position& p, const Geometry& geometry);
    };
    std::pair<Geometry, Geometry> v_split(const Geometry& g, float split_ratio = 0.5f);
    std::pair<Geometry, Geometry> h_split(const Geometry& g, float split_ratio = 0.5f);
    /// Aligned-axis bounding box collision
    bool is_inside(const Position& p, const Geometry& geometry);
    bool aabb_collision(const Geometry& p, const Geometry& geometry);
} // namespace cx::geom
