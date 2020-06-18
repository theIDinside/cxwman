#include <datastructure/geometry.hpp>

namespace cx::geom
{
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

} // namespace cx::geom