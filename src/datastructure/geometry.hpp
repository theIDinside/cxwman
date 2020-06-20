#pragma once
// System headers
#include <algorithm>
#include <array>
#include <tuple>
// Library/Application headers
#include <coreutils/core.hpp>
namespace cx::geom
{
    // Geometry Unit
    using GU = int;
    struct Position {
        GU x, y;
        friend Position operator+(const Position& lhs, const Position& rhs);
    };

    Position operator+(const Position& lhs, const Position& rhs);

    struct Geometry {
        Position pos;
        GU width, height;
        Geometry(GU x, GU y, GU width, GU height) : pos{x, y}, width(width), height(height) {}
        Geometry(Position p, GU width, GU height) : pos(p), width(width), height(height) {}

        friend std::pair<Geometry, Geometry> v_split(const Geometry& g, float split_ratio);
        friend std::pair<Geometry, Geometry> h_split(const Geometry& g, float split_ratio);

        [[nodiscard]] constexpr inline GU y() const { return pos.y; }
        [[nodiscard]] constexpr inline GU x() const { return pos.x; }
        constexpr inline auto xcb_value_list() -> std::array<GU, 4> { return {pos.x, pos.y, width, height}; }

        static Geometry default_new() { return Geometry{0, 0, 800, 600}; }

        // This function

        friend bool aabb_collision(const Geometry& p, const Geometry& geometry);
        friend bool is_inside(const Position& p, const Geometry& geometry);
        friend Geometry operator+(const Geometry& lhs, const Position& rhs);
        // Scalar multiplication of the *dimensions*, i.e. width & height, not the anchor/position
        friend Geometry operator*(const Geometry& lhs, int rhs);
    };
    std::pair<Geometry, Geometry> v_split(const Geometry& g, float split_ratio = 0.5f);
    std::pair<Geometry, Geometry> h_split(const Geometry& g, float split_ratio = 0.5f);
    /// Aligned-axis bounding box collision
    bool is_inside(const Position& p, const Geometry& geometry);
    bool aabb_collision(const Geometry& p, const Geometry& geometry);
    Geometry operator+(const Geometry& lhs, const Position& rhs);
    Geometry operator*(const Geometry& lhs, int rhs);
    Position wrapping_add(const Position& to, const Position& vector, const Geometry& bounds, int add_on_wrap = 0);
    Position wrapping_sub(const Position& to, const Position& vector, const Geometry& bounds);
} // namespace cx::geom
