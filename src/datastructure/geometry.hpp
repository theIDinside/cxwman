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
    struct Vector {
        GU x, y;
    };

    struct Position {
        GU x, y;
        friend Position operator+(const Position& lhs, const Vector& rhs);
    };

    Position operator+(const Position& lhs, const Vector& rhs);

    struct Geometry {
        Position pos;
        GU width, height;
        Geometry(GU x, GU y, GU width, GU height) : pos{x, y}, width(width), height(height) {}
        Geometry(Position p, GU width, GU height) : pos(p), width(width), height(height) {}

        friend auto v_split(const Geometry& g, float split_ratio) -> std::pair<Geometry, Geometry>;
        friend auto h_split(const Geometry& g, float split_ratio) -> std::pair<Geometry, Geometry>;

        friend auto v_split_at(const Geometry& g, int x, int border_width_adjust) -> std::pair<Geometry, Geometry>;
        friend auto h_split_at(const Geometry& g, int y, int border_width_adjust) -> std::pair<Geometry, Geometry>;

        [[nodiscard]] constexpr inline GU y() const { return pos.y; }
        [[nodiscard]] constexpr inline GU x() const { return pos.x; }
        /// Returns geometry values used by the X server. Utility function, so we can use structured bindings like so: auto& [x,y,w,h] = xcb_value_list()
        [[nodiscard]] constexpr inline auto xcb_value_list() const -> std::array<GU, 4> { return {pos.x, pos.y, width, height}; }
        [[nodiscard]] constexpr inline auto xcb_value_list_border_adjust(int border_width) const -> std::array<GU, 4> { return {pos.x, pos.y, width - (border_width * 2), height - (border_width * 2)}; }
        static Geometry default_new() { return Geometry{0, 0, 800, 600}; }
        static Geometry window_default() { return Geometry{0, 0, 400, 400}; }
        // This function

        friend bool aabb_collision(const Geometry& p, const Geometry& geometry);
        friend bool is_inside(const Position& p, const Geometry& geometry);
        friend Geometry operator+(const Geometry& lhs, const Position& rhs);
        // Scalar multiplication of the *dimensions*, i.e. width & height, not the anchor/position
        friend Geometry operator*(const Geometry& lhs, int rhs);
    };
    auto v_split(const Geometry& g, float split_ratio = 0.5f) -> std::pair<Geometry, Geometry>;
    auto h_split(const Geometry& g, float split_ratio = 0.5f) -> std::pair<Geometry, Geometry>;
    auto v_split_at(const Geometry& g, int x, int border_width_adjust = 0) -> std::pair<Geometry, Geometry>;
    auto h_split_at(const Geometry& g, int y, int border_width_adjust = 0) -> std::pair<Geometry, Geometry>;

    /// Aligned-axis bounding box collision
    auto is_inside(const Position& p, const Geometry& geometry) -> bool;
    auto aabb_collision(const Geometry& p, const Geometry& geometry) -> bool;
    auto adjacent_to(const Geometry& client, const Geometry& target, int boundary_breadth = 3) -> bool;

    Geometry operator+(const Geometry& lhs, const Position& rhs);
    Geometry operator*(const Geometry& lhs, int rhs);
    Position wrapping_add(const Position& to, const Vector& vector, const Geometry& bounds, int add_on_wrap = 0);
    Position wrapping_sub(const Position& to, const Vector& vector, const Geometry& bounds);
} // namespace cx::geom
