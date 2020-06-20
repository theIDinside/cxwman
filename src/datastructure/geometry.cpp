#include <datastructure/geometry.hpp>

namespace cx::geom
{

    Position operator+(const Position& lhs, const Position& rhs) {
        return Position{lhs.x + rhs.x, lhs.y + rhs.y};
    }

    /// This function takes a position add_to and adds a vector add_from to it, and clamps the result to land within
    /// the geometry of bounds (x0, y0, x0+width, y0+height). This is used when we move windows, because if the
    /// result lands outside of the root geometry, we must make sure it wraps around. add_on_wrap is how much
    /// we add to the result when it wraps around the geometry/screen. Default value is 0, but we will use 10 for the most part
    Position wrapping_add(const Position& add_to, const Position& add_from, const Geometry& bounds, int add_on_wrap) {
        auto res = Position{0,0};
        if(add_to.x + add_from.x > bounds.x() + bounds.width) {
            res.x = bounds.x() + add_on_wrap;
        } else if(add_to.x + add_from.x < bounds.x()) { // means add_from.x was a negative number
            res.x = bounds.x() + bounds.width - add_on_wrap;
        } else { // we are within bounds of bounds
            res.x = add_to.x + add_from.x;
        }
        if(add_to.y + add_from.y > bounds.y() + bounds.height) {
            res.y = bounds.y() + add_on_wrap;
        } else if(add_to.y + add_from.y < bounds.y()) {
            res.y = bounds.y() + bounds.height - add_on_wrap;
        } else {
            res.y = add_to.y + add_from.y;
        }
        return res;
    }


    std::pair<Geometry, Geometry> v_split(const Geometry& g, float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto lwidth = static_cast<GU>(static_cast<float>(g.width) * sp);
        auto rwidth = static_cast<GU>(g.width - lwidth);
        auto rx = static_cast<GU>(g.x() + lwidth);
        return {Geometry{g.x(), g.y(), lwidth, g.height}, Geometry{rx, g.y(), rwidth, g.height}};
    }
    std::pair<Geometry, Geometry> h_split(const Geometry& g, float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto theight = static_cast<GU>(static_cast<float>(g.height) * sp);
        auto bheight = static_cast<GU>(g.height - theight);
        auto by = static_cast<GU>(g.y() + theight);

        return {Geometry{g.x(), g.y(), g.width, theight}, Geometry{g.x(), by, g.width, bheight}};
    }

    // TODO(implement): Use these functions to navigate windows in screen space.
    // Take top-left anchor, look at what window is 10 pixels north by using: if is_inside(anchor, otherWindowGeometry), then swap
    // geometries and remap workspace etc
    bool is_inside(const Position& p, const Geometry& geometry)
    {
        auto within_x = p.x >= geometry.x() && p.x <= geometry.x() + geometry.width;
        auto within_y = p.y >= geometry.y() && p.y <= geometry.y() + geometry.height;
        return within_x && within_y;
    }
    bool aabb_collision(const Geometry& a, const Geometry& b)
    {
        auto x_collision = ((a.x() + a.width >= b.x()) && (b.x() + b.width >= a.x()));
        auto y_collision = ((a.y() + a.height >= b.y()) && (b.y() + b.height >= a.y()));
        return x_collision && y_collision;
    }

    Geometry operator+(const Geometry& lhs, const Position& rhs) {
        return Geometry{lhs.pos.x + rhs.x, lhs.pos.y + rhs.y, lhs.width, lhs.height};
    }

    Geometry operator*(const Geometry& lhs, int rhs) {
        return Geometry{lhs.pos, lhs.width * rhs, lhs.height * rhs};
    }

} // namespace cx::geom