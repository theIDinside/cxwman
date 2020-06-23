#include <cassert>
#include <datastructure/geometry.hpp>

namespace cx::geom
{

    Position operator+(const Position& lhs, const Vector& rhs) { return Position{lhs.x + rhs.x, lhs.y + rhs.y}; }

    /// This function takes a position add_to and adds the parameter vector to it, and clamps the result to land within
    /// the geometry of bounds (x0, y0, x0+width, y0+height). This is used when we move windows, because if the
    /// result lands outside of the root geometry, we must make sure it wraps around. add_on_wrap is how much
    /// we add to the result when it wraps around the geometry/screen. Default value is 0, but we will use 10 for the most part
    Position wrapping_add(const Position& add_to, const Vector& vector, const Geometry& bounds, int add_on_wrap)
    {
        auto res = Position{0, 0};
        if(add_to.x + vector.x > bounds.x() + bounds.width) {
            res.x = bounds.x() + add_on_wrap;
        } else if(add_to.x + vector.x < bounds.x()) { // means vector.x was a negative number
            res.x = bounds.x() + bounds.width - add_on_wrap;
        } else { // we are within bounds of bounds
            res.x = add_to.x + vector.x;
        }
        if(add_to.y + vector.y > bounds.y() + bounds.height) {
            res.y = bounds.y() + add_on_wrap;
        } else if(add_to.y + vector.y < bounds.y()) {
            res.y = bounds.y() + bounds.height - add_on_wrap;
        } else {
            res.y = add_to.y + vector.y;
        }
        return res;
    }

    auto v_split(const Geometry& g, float split_ratio) -> std::pair<Geometry, Geometry>
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto lwidth = static_cast<GU>(static_cast<float>(g.width) * sp);
        auto rwidth = static_cast<GU>(g.width - lwidth);
        auto rx = static_cast<GU>(g.x() + lwidth);
        return {Geometry{g.x(), g.y(), lwidth, g.height}, Geometry{rx, g.y(), rwidth, g.height}};
    }
    auto h_split(const Geometry& g, float split_ratio) -> std::pair<Geometry, Geometry>
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto theight = static_cast<GU>(static_cast<float>(g.height) * sp);
        auto bheight = static_cast<GU>(g.height - theight);
        auto by = static_cast<GU>(g.y() + theight);

        return {Geometry{g.x(), g.y(), g.width, theight}, Geometry{g.x(), by, g.width, bheight}};
    }

    auto v_split_at(const Geometry& g, int x) -> std::pair<Geometry, Geometry>
    {
        assert(x < g.width && "you can't split at relative x greater than g.width.");
        auto width_left = x;
        auto width_right = g.width - x;
        auto p_left = g.pos;
        auto p_right = g.pos + Vector{x, 0};
        auto height = g.height;
        assert(width_right > 0 && width_left > 0 && "Width have to be > 0");
        return {Geometry{p_left, width_left, height}, Geometry{p_right, width_right, height}};
    }
    auto h_split_at(const Geometry& g, int y) -> std::pair<Geometry, Geometry>
    {
        assert(y < g.height && "You can't split at relative y greater than g.height");
        auto height_top = y;
        auto height_bottom = g.height - y;
        auto p_top = g.pos;
        auto p_bottom = g.pos + Vector{0, y};
        auto width = g.width;
        return {Geometry{p_top, width, height_top}, Geometry{p_bottom, width, height_bottom}};
    }

    // TODO(implement): Use these functions to navigate windows in screen space.
    // Take top-left anchor, look at what window is 10 pixels north by using: if is_inside(anchor, otherWindowGeometry), then swap
    // geometries and remap workspace etc
    auto is_inside(const Position& p, const Geometry& geometry) -> bool
    {
        auto within_x = p.x >= geometry.x() && p.x <= geometry.x() + geometry.width;
        auto within_y = p.y >= geometry.y() && p.y <= geometry.y() + geometry.height;
        return within_x && within_y;
    }
    auto aabb_collision(const Geometry& a, const Geometry& b) -> bool
    {
        auto x_collision = ((a.x() + a.width) > b.x()) && ((b.x() + b.width) > a.x());
        auto y_collision = ((a.y() + a.height) > b.y()) && ((b.y() + b.height) > a.y());
        return x_collision && y_collision;
    }

    Geometry operator+(const Geometry& lhs, const Position& rhs) { return Geometry{lhs.pos.x + rhs.x, lhs.pos.y + rhs.y, lhs.width, lhs.height}; }
    Geometry operator*(const Geometry& lhs, int rhs) { return Geometry{lhs.pos, lhs.width * rhs, lhs.height * rhs}; }

} // namespace cx::geom