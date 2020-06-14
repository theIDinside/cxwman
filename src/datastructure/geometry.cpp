#include <datastructure/geometry.hpp>

namespace cx::geom
{
    std::pair<Geometry, Geometry> vsplit(const Geometry& g, float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto lwidth = static_cast<GU>(static_cast<float>(g.width) * sp);
        auto rwidth = static_cast<GU>(g.width - lwidth);
        auto rx = static_cast<GU>(g.x() + lwidth);
        return {Geometry{g.x(), g.y(), lwidth, g.height}, Geometry{rx, g.y(), rwidth, g.height}};
    }
    std::pair<Geometry, Geometry> hsplit(const Geometry& g, float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto theight = static_cast<GU>(static_cast<float>(g.height) * sp);
        auto bheight = static_cast<GU>(g.height - theight);
        auto by = static_cast<GU>(g.y() + theight);

        return {Geometry{g.x(), g.y(), g.width, theight}, Geometry{g.x(), by, g.width, bheight}};
    }

} // namespace cx::geom
